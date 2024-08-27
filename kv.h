#include <cstdint>
#include <span>
#include <string>

struct KeyValuePair {
    const std::string key;
    const uint8_t value;

    inline std::span<const std::byte> key_bytes() const {
        return std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(key.data()),
            key.size()
        );
    }
};

KeyValuePair get_random_kvp();

std::string sha256(const std::string& input);
