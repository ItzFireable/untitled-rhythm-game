#ifndef OSU_UTILS_H
#define OSU_UTILS_H

#include <string>
#include <thread>

class OsuUtils {
public:
    OsuUtils() = default;
    ~OsuUtils() {
        if (parserThread_.joinable()) {
            parserThread_.join(); 
        }
    };

    std::string getOsuDatabasePath();
    void parseOsuDatabase();

    bool isParsingActive() const {
        return parserThread_.joinable();
    }
private:
    std::thread parserThread_;

    void parseDatabaseThread(const std::string& path) {
        try {
            GAME_LOG_INFO("OsuUtils: Starting osu! database parsing thread.");
            // TODO: Parse the database
            GAME_LOG_INFO("OsuUtils: Finished osu! database parsing.");
        } catch (const std::exception& e) {
            GAME_LOG_ERROR(std::string("OsuUtils: Error parsing osu! database: ") + e.what());
        } catch (...) {
            GAME_LOG_ERROR("OsuUtils: Unknown error occurred while parsing osu! database.");
        }
    }
};

#endif