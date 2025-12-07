#include "system/Logger.h"

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

std::string Logger::getCurrentDate() {
    using namespace std::chrono;
    
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time_t);
    
    std::stringstream ss;
    ss << std::put_time(local_time, "%Y-%m-%d");
    return ss.str();
}

std::string Logger::getCurrentTimestamp(bool showMs) {
    using namespace std::chrono;
    
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);

    auto ms = duration_cast<milliseconds>(now - system_clock::from_time_t(now_time_t));
    std::tm* local_time = std::localtime(&now_time_t);
    
    std::stringstream ss;
    ss << std::put_time(local_time, "%H:%M:%S");

    if (showMs) {
        ss << "." << std::setw(3) << std::setfill('0') << ms.count();
    }

    return ss.str();
}

Logger::Logger() {
    std::string currentDate = getCurrentDate();
    std::string currentTime = getCurrentTimestamp(false);
    std::string logDirectory = "logs"; 

    try {
        if (!std::filesystem::exists(logDirectory)) {
            if (!std::filesystem::create_directories(logDirectory)) {
                std::cerr << "ERROR: Failed to create log directory: " << logDirectory << ". Logging to console only." << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Filesystem error while creating log directory: " << e.what() << ". Logging to console only." << std::endl;
    }

    for (auto& ch : currentTime) {
        if (ch == ':' || ch == ' ') {
            ch = '-';
        }
    }

    std::string logFileName = "game_" + currentDate + "_" + currentTime + ".log";
    logFile_.open(logDirectory + "/" + logFileName, std::ios::out | std::ios::trunc);
    std::cout << "Log file created at: " << logDirectory + "/" + logFileName << std::endl;

    if (!logFile_.is_open()) {
        std::cerr << "ERROR: Failed to open log file '" << logFileName << "'. Logging to console only." << std::endl;
    }

    logThread_ = std::thread(&Logger::processLogQueue, this);
}

Logger::~Logger() {
    shutdown();
}

void Logger::shutdown() {
    if (logThread_.joinable()) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            running_ = false;
        }
        cv_.notify_one();
        logThread_.join();
        
        if (logFile_.is_open()) {
            logFile_.flush();
            logFile_.close();
        }
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::GAME_INFO:    return "[INFO]";
        case LogLevel::GAME_WARNING: return "[WARN]";
        case LogLevel::GAME_ERROR:   return "[ERROR]";
        case LogLevel::GAME_DEBUG:   return "[DEBUG]";
        default:                     return "[UNKWN]";
    }
}

void Logger::processLogQueue() {
    while (running_) {
        std::string message;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]{ return !logQueue_.empty() || !running_; });

            if (!running_ && logQueue_.empty()) {
                break;
            }
            
            message = logQueue_.front();
            logQueue_.pop();
            
        }

        if (message.find("[ERROR]") != std::string::npos || 
            message.find("[WARN]") != std::string::npos) 
        {
            std::cerr << message << "\n";
        } else {
            std::cout << message << "\n";
        }

        if (logFile_.is_open()) {
            logFile_ << message << "\n";
            if (message.find("[ERROR]") != std::string::npos) {
                 logFile_.flush(); 
            }
        }
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, int line) {
    if (level > maxLogLevel_) {
        return; 
    }

    std::string levelStr = levelToString(level);
    std::string timestamp = getCurrentTimestamp();
    size_t lastSlash = file.find_last_of("/\\");
    std::string fileName = (lastSlash == std::string::npos) ? file : file.substr(lastSlash + 1);

    std::stringstream logStream;
    logStream << timestamp << " " << levelStr << " "
              << "(" << fileName << ":" << line << ") "
              << message;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        logQueue_.push(logStream.str());
    }
    
    cv_.notify_one(); 
}