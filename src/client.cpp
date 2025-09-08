#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include "protocol.h"
#include "logger.h"

std::mutex cout_mutex;

void receiveMessages(int sock) {
    char buf[512];
    while (true) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
        std::string msg(buf, n);
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << msg << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        Logger::error("Usage: client <host> <port>");
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("Connect failed");
        return 1;
    }

    std::thread recvThread(receiveMessages, sock);
    recvThread.detach();

    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line.empty()) continue;
        if (line == "QUIT") break;
        std::string encoded = Protocol::encode(line + "\n");
        send(sock, encoded.data(), encoded.size(), 0);
    }

    close(sock);
    return 0;
}
