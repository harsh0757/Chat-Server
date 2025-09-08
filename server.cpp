#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <csignal>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <mutex>
#include <algorithm>
#include "thread_pool.h"
#include "logger.h"
#include "protocol.h"

static bool running = true;

struct Client
{
    int fd;
    std::string nick;
    std::string buffer;
};

std::unordered_map<int, Client> clients;
std::mutex clients_mutex;

// Convert string to lowercase
std::string toLower(const std::string &s)
{
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

void broadcast(const std::string &msg, int exclude_fd = -1)
{
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto &kv : clients)
    {
        if (kv.first == exclude_fd)
            continue;
        std::string out = Protocol::encode(msg);
        send(kv.first, out.data(), out.size(), 0);
    }
}

void handleMessage(int fd, const std::string &msg) {
    std::string trimmed = msg;
    trimmed.erase(0, trimmed.find_first_not_of(" \n\r\t"));
    trimmed.erase(trimmed.find_last_not_of(" \n\r\t")+1); // trim both ends

    if (trimmed.empty()) return; // ignore empty messages

    if (trimmed.size() >= 5 && toLower(trimmed.substr(0,4)) == "nick") {
        std::string name = trimmed.substr(5);
        name.erase(0, name.find_first_not_of(" \n\r\t"));
        name.erase(name.find_last_not_of(" \n\r\t")+1);

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[fd].nick = name;
        }
        Logger::info("Client " + std::to_string(fd) + " set nick to " + name);

    } else if (trimmed[0] == '@') {
        // Private message
        size_t spacePos = trimmed.find(' ');
        std::string target, text;
        if (spacePos != std::string::npos) {
            target = trimmed.substr(1, spacePos - 1); // skip '@'
            text = trimmed.substr(spacePos + 1);
        } else {
            target = trimmed.substr(1); // only nickname
            text = "";
        }

        // Trim whitespace
        target.erase(0, target.find_first_not_of(" \n\r\t"));
        target.erase(target.find_last_not_of(" \n\r\t")+1);
        text.erase(0, text.find_first_not_of(" \n\r\t"));
        text.erase(text.find_last_not_of(" \n\r\t")+1);

        std::lock_guard<std::mutex> lock(clients_mutex);
        bool found = false;
        for (auto &kv : clients) {
            if (!kv.second.nick.empty() && toLower(kv.second.nick) == toLower(target)) {
                std::string out = Protocol::encode("[PM] " + clients[fd].nick + ": " + text);
                send(kv.first, out.data(), out.size(), 0);
                Logger::pm("PM from " + clients[fd].nick + " to " + kv.second.nick + ": " + text);
                found = true;
                break;
            }
        }
        if (!found) {
            std::string out = Protocol::encode("[Server] Unknown user: " + target);
            send(fd, out.data(), out.size(), 0);
        }

    } else if (toLower(trimmed) == "list") {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string names = "Clients:";
        for (auto &kv : clients) names += " " + kv.second.nick;
        std::string out = Protocol::encode(names);
        send(fd, out.data(), out.size(), 0);
        Logger::info("Client " + clients[fd].nick + " requested LIST");

    } else {
        std::string nickname = clients[fd].nick.empty() ? "unknown user" : clients[fd].nick;
        Logger::user(nickname + ": " + trimmed);
        broadcast(nickname + ": " + trimmed, fd);
    }
}

void signalHandler(int) { running = false; }

int main()
{
    signal(SIGINT, signalHandler);

    // Read config.json (simple whitespace-separated values)
    std::ifstream f("../config.txt");
    if (!f.is_open())
    {
        Logger::error("Failed to read config.json");
        return 1;
    }

    std::string host;
    int port = 8080, max_clients = 50, threads = 4;
    f >> host >> port >> max_clients >> threads;
    Logger::info("Server config - Host: " + host + ", Port: " + std::to_string(port) +
                 ", Max Clients: " + std::to_string(max_clients) + ", Threads: " + std::to_string(threads));

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        Logger::error("Bind failed");
        return 1;
    }
    if (listen(server_fd, max_clients) < 0)
    {
        Logger::error("Listen failed");
        return 1;
    }

    Logger::info("Server started on port " + std::to_string(port));

    ThreadPool pool(threads);
    std::vector<pollfd> fds;
    fds.push_back({server_fd, POLLIN, 0});

    while (running)
    {
        poll(fds.data(), fds.size(), 1000);
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == server_fd)
                {
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd >= 0)
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        clients[client_fd] = {client_fd, "unknown user", ""};
                        fds.push_back({client_fd, POLLIN, 0});
                        Logger::info("New client: " + std::to_string(client_fd));
                    }
                }
                else
                {
                    char buf[512];
                    ssize_t n = recv(fds[i].fd, buf, sizeof(buf), 0);
                    if (n <= 0)
                    {
                        close(fds[i].fd);
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        Logger::info("Client disconnected: " + clients[fds[i].fd].nick);
                        clients.erase(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        i--;
                    }
                    else
                    {
                        clients[fds[i].fd].buffer.append(buf, n);

                        // Decode messages
                        auto msgs = Protocol::decode(clients[fds[i].fd].buffer);

                        // If no delimiter but buffer is non-empty, treat it as a message
                        if (msgs.empty() && !clients[fds[i].fd].buffer.empty())
                        {
                            msgs.push_back(clients[fds[i].fd].buffer);
                            clients[fds[i].fd].buffer.clear();
                        }

                        for (auto &m : msgs)
                        {
                            pool.enqueue(handleMessage, fds[i].fd, m);
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    Logger::info("Server stopped");
    return 0;
}
