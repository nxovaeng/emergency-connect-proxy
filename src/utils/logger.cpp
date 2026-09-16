#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::init(const std::string &logFile, const std::string &level) {
    logFile_ = logFile;
    level_ = level;
    
    if (!logFile.empty()) {
        file_.open(logFile, std::ios::app);
    }
}

void Logger::debug(const std::string &message) {
    if (level_ == "debug") {
        write("DEBUG", message);
    }
}

void Logger::info(const std::string &message) {
    write("INFO", message);
}

void Logger::warn(const std::string &message) {
    write("WARN", message);
}

void Logger::error(const std::string &message) {
    write("ERROR", message);
}

void Logger::write(const std::string &level, const std::string &message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    
    std::string logMessage = "[" + ss.str() + "] [" + level + "] " + message;
    
    std::cout << logMessage << std::endl;
    
    if (file_.is_open()) {
        file_ << logMessage << std::endl;
        file_.flush();
    }
}
