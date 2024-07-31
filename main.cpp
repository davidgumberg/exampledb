#include <filesystem>
#include <iostream>
#include <vector>

#include <mdbx.h++>

#include "kv.h"
#include "mdbx.h"

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
       pairs.push_back(KeyValuePair::get_random());
    }

    // Loop through and print the key-value pairs
    for (const auto& pair : pairs) {
        std::cout << "Key: " << pair.key << ", Value: " << pair.value << std::endl;
    }

    return 0;
}
