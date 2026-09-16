#pragma once

#include <string>
#include <fstream>
#include <memory>

class Logger {
public:
    static Logger& instance();
    
    void init(const std::string &logFile, const std::string &level);
    
    void debug(const std::string &message);
    void info(const std::string &message);
    void warn(const std::string &message);
    void error(const std::string &message);
    
private:
    Logger();
    ~Logger();
    
    std::string logFile_;
    std::string level_;
    std::ofstream file_;
    
    void write(const std::string &level, const std::string &message);
};
