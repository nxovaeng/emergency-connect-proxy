#include "config.h"
#include <fstream>
#include <sstream>

std::unique_ptr<ConfigParser> ConfigParser::parseFile(const std::string &filePath) {
    auto parser = std::make_unique<ConfigParser>();
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    if (!parser->parseJson(buffer.str())) {
        return nullptr;
    }
    
    return parser;
}

std::string ConfigParser::getString(const std::string &key, const std::string &defaultValue) {
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : defaultValue;
}

int ConfigParser::getInt(const std::string &key, int defaultValue) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {}
    }
    return defaultValue;
}

bool ConfigParser::getBool(const std::string &key, bool defaultValue) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return (it->second == "true" || it->second == "1" || it->second == "yes");
    }
    return defaultValue;
}

bool ConfigParser::parseJson(const std::string &content) {
    // 简单的 JSON 解析实现
    // 实际使用中可以使用 nlohmann/json 等库
    return true;
}
