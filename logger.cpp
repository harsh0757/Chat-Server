#include "logger.h"
#include <iostream>
#include <mutex>
#include <ctime>

namespace {
    std::mutex cout_mutex;

    std::string getTimestamp() {
        time_t t = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        return std::string(buf);
    }

    std::string getColor(Logger::Level lvl) {
        switch(lvl) {
            case Logger::Level::INFO:  return "\033[1;34m"; // Blue
            case Logger::Level::ERROR: return "\033[1;31m"; // Red
            case Logger::Level::PM:    return "\033[1;32m"; // Green
            case Logger::Level::USER:  return "\033[1;33m"; // Yellow
        }
        return "\033[0m";
    }
}

namespace Logger {
    void log(Level lvl, const std::string &msg) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << getColor(lvl) << "[" << getTimestamp() << "] ";
        switch(lvl) {
            case Level::INFO:  std::cout << "[INFO] "; break;
            case Level::ERROR: std::cout << "[ERROR] "; break;
            case Level::PM:    std::cout << "[PM] "; break;
            case Level::USER:  std::cout << "[USER] "; break;
        }
        std::cout << msg << "\033[0m\n";
    }

    void info(const std::string &msg)  { log(Level::INFO, msg); }
    void error(const std::string &msg) { log(Level::ERROR, msg); }
    void pm(const std::string &msg)    { log(Level::PM, msg); }
    void user(const std::string &msg)  { log(Level::USER, msg); }
}
