#include <cstdint>
#include <span>
#include <string>

struct KeyValuePair {
    const std::span<const std::byte> key;
    const uint8_t value;
};

KeyValuePair get_random_kvp();

std::string sha256(const std::string& input);
