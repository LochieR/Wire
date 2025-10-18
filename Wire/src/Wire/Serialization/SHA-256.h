#pragma once

#include <array>
#include <string>
#include <vector>

namespace wire {

    std::string generateSHA256String(const std::vector<uint8_t>& data);
    std::string generateSHA256String(std::string_view input);

    std::array<uint32_t, 8> generateSHA256(const std::vector<uint8_t>& data);
    std::array<uint32_t, 8> generateSHA256(std::string_view input);

}
