#include <filesystem>
#include <iostream>
#include <vector>

#include <mdbx.h++>

#include "kv.h"
#include "mdbx.h"
//
// Overload the left shift operator to print std::span<const std::byte>
std::ostream& operator<<(std::ostream& os, const std::span<const std::byte>& bytes) {
    os << "0x";
    for (const auto& byte : bytes) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return os;
}

int main() {
    std::filesystem::path data_path;

    try {
        data_path = std::filesystem::current_path() / "data";
        if (!std::filesystem::exists(data_path)) {
            if (!std::filesystem::create_directory(data_path)) {
                throw std::runtime_error("Failed to create directory: " + data_path.string());
            }
            std::cout << "Created directory: " << data_path << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    MDBXWrapper wrap_mdbx(data_path);
    std::vector<KeyValuePair> pairs;

    // Generate 10 random key-value pairs
    for (int i = 0; i < 10; i++) {
       pairs.push_back(get_random_kvp());
    }

    // Loop through and print the key-value pairs
    for (const auto& pair : pairs) {
        wrap_mdbx.Write(pair.key, pair.value);
        std::cout << "Key: " << pair.key << ", Value: " << std::to_string(pair.value) << std::endl;
    }

    return 0;
}
