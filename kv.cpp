#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <random>
#include <span>
#include <sstream>
#include <string>

#include "kv.h"

/* Generate a random std::string key, and double value. */
KeyValuePair get_random_kvp() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    std::string input = "input" + std::to_string(dis(gen));
    std::string key = sha256(input);
    uint8_t value = std::floor(dis(gen) * 100);
    
    const std::span<const std::byte> _key = std::as_bytes(std::span(key));

    return KeyValuePair{_key, value};
}

/* Return the sha256 of a given string */
std::string sha256(const std::string& input) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == nullptr) {
        return "";
    }

    if(EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    if(EVP_DigestUpdate(context, input.c_str(), input.length()) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if(EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}
