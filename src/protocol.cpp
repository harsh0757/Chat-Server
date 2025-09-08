#include "protocol.h"
#include <cstring>
#include <cstdint>

std::string Protocol::encode(const std::string& msg) {
    uint32_t len = msg.size();
    std::string out(sizeof(len), 0);
    std::memcpy(&out[0], &len, sizeof(len));
    out += msg;
    return out;
}

std::vector<std::string> Protocol::decode(std::string& buffer) {
    std::vector<std::string> msgs;
    while (buffer.size() >= sizeof(uint32_t)) {
        uint32_t len = 0;
        std::memcpy(&len, &buffer[0], sizeof(len));
        if (buffer.size() < sizeof(len) + sizeof(uint32_t)) break;
        msgs.push_back(buffer.substr(sizeof(uint32_t), len));
        buffer.erase(0, sizeof(uint32_t) + len);
    }
    return msgs;
}
