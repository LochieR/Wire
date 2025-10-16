#pragma once

#include <string>
#include <vector>

namespace wire {

	std::string generateSHA256(const std::vector<uint8_t>& data);
	std::string generateSHA256(std::string_view input);

}
