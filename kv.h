#include <string>

using KeyValuePair = std::pair<std::string, double>;
KeyValuePair get_random_kvp();

std::string sha256(const std::string& input);
