#pragma once
#include <string>
#include <vector>

class Protocol {
public:
    static std::string encode(const std::string& msg);
    static std::vector<std::string> decode(std::string& buffer);
};
