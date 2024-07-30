#include <iostream>
#include <vector>

#include <mdbx.h++>

#include "kv.h"

int main() {
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
