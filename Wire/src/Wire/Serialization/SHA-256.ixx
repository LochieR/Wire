module;

#include <string>
#include <vector>

export module wire.serialization:sha256;

namespace wire {

	export std::string generateSHA256(const std::vector<uint8_t>& data);
	export std::string generateSHA256(std::string_view input);

}
