#include "string_utils.h"
#include <algorithm>
#include <cctype>

std::vector<std::string> StringUtils::split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : str) {
        if (c == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string StringUtils::trim(const std::string &str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

std::string StringUtils::toLower(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string StringUtils::toUpper(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}
