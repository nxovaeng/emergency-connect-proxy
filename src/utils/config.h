#pragma once

#include <string>
#include <map>
#include <memory>

class ConfigParser {
public:
    // 解析配置文件
    static std::unique_ptr<ConfigParser> parseFile(const std::string &filePath);
    
    // 获取值
    bool hasKey(const std::string &key) const;
    std::string getString(const std::string &key, const std::string &defaultValue = "");
    int getInt(const std::string &key, int defaultValue = 0);
    bool getBool(const std::string &key, bool defaultValue = false);
    
private:
    std::map<std::string, std::string> data_;
    
    bool parseJson(const std::string &content);
};
