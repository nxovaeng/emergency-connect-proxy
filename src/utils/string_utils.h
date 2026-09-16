#pragma once

#include <string>
#include <vector>

class StringUtils {
public:
    // 分割字符串
    static std::vector<std::string> split(const std::string &str, char delimiter);
    
    // 去除前后空格
    static std::string trim(const std::string &str);
    
    // 转换为小写
    static std::string toLower(const std::string &str);
    
    // 转换为大写
    static std::string toUpper(const std::string &str);
};
