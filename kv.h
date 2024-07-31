#include <string>

using KeyValuePair = std::pair<std::string, double>;
KeyValuePair get_random_kv();

std::string sha256(const std::string& input);
