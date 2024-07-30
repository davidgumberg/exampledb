#include <string>

class KeyValuePair {
public:
    std::string key;
    double value;

    KeyValuePair(const std::string& k, double v) : key(k), value(v) {}

    static KeyValuePair get_random();
};

std::string sha256(const std::string& input);
