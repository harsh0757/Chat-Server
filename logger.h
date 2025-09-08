#pragma once
#include <string>

namespace Logger {
    enum class Level { INFO, ERROR, PM, USER };

    void log(Level lvl, const std::string &msg);
    void info(const std::string &msg);
    void error(const std::string &msg);
    void pm(const std::string &msg);
    void user(const std::string &msg);
}
