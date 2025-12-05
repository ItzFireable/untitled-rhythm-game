#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <sstream>
#include <thread>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <iomanip>

enum class LogLevel {
    GAME_INFO, 
    GAME_WARNING, 
    GAME_ERROR, 
    GAME_DEBUG 
};

class Logger {
public:
    static Logger& getInstance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void log(LogLevel level, const std::string& message, const std::string& file, int line);
    void setLogLevel(LogLevel maxLevel) { maxLogLevel_ = maxLevel; }
    void shutdown();

private:
    Logger();
    ~Logger();
    
    std::thread logThread_;
    std::queue<std::string> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    bool running_ = true;

    std::ofstream logFile_;
    LogLevel maxLogLevel_ = LogLevel::GAME_DEBUG; 

    void processLogQueue();

    std::string levelToString(LogLevel level);
    std::string getCurrentTimestamp();
    std::string getCurrentDate();
};

#define GAME_LOG_INFO(msg)    Logger::getInstance().log(LogLevel::GAME_INFO, msg, __FILE__, __LINE__)
#define GAME_LOG_WARN(msg)    Logger::getInstance().log(LogLevel::GAME_WARNING, msg, __FILE__, __LINE__)
#define GAME_LOG_ERROR(msg)   Logger::getInstance().log(LogLevel::GAME_ERROR, msg, __FILE__, __LINE__)
#define GAME_LOG_DEBUG(msg)   Logger::getInstance().log(LogLevel::GAME_DEBUG, msg, __FILE__, __LINE__)

#endif