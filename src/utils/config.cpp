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

bool ConfigParser::hasKey(const std::string &key) const {
    return data_.find(key) != data_.end();
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
    // 提取简单 JSON 键值对，无需笨重依赖
    size_t pos = 0;
    while (pos < content.size()) {
        size_t keyQuoteStart = content.find('"', pos);
        if (keyQuoteStart == std::string::npos) break;
        size_t keyQuoteEnd = content.find('"', keyQuoteStart + 1);
        if (keyQuoteEnd == std::string::npos) break;

        std::string key = content.substr(keyQuoteStart + 1, keyQuoteEnd - keyQuoteStart - 1);
        size_t colon = content.find(':', keyQuoteEnd + 1);
        if (colon == std::string::npos) break;

        size_t valStart = content.find_first_not_of(" \t\r\n", colon + 1);
        if (valStart == std::string::npos) break;

        if (content[valStart] == '"') {
            size_t valEnd = content.find('"', valStart + 1);
            if (valEnd != std::string::npos) {
                data_[key] = content.substr(valStart + 1, valEnd - valStart - 1);
                pos = valEnd + 1;
            } else {
                break;
            }
        } else if (content[valStart] == '{' || content[valStart] == '[') {
            // 跳过嵌套对象/数组的起始符，继续在内部提取叶子字段
            pos = colon + 1;
        } else {
            size_t valEnd = content.find_first_of(",}\r\n", valStart);
            if (valEnd == std::string::npos) valEnd = content.size();
            std::string val = content.substr(valStart, valEnd - valStart);
            while (!val.empty() && (val.back() == ' ' || val.back() == '\t')) val.pop_back();
            data_[key] = val;
            pos = valEnd;
        }
    }
    return true;
}
