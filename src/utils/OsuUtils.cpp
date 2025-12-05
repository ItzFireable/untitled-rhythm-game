#if defined(_WIN32)
// Define before including Windows headers to prevent conflicts with C++ standard library
// Although typically not needed for this exact set, it's good defensive programming.
#define WIN32_LEAN_AND_MEAN
// Also, define NOMINMAX to prevent conflicts between std::min/max and Windows macros.
#define NOMINMAX
#include <shlobj.h>
#include <knownfolders.h>
#pragma comment(lib, "shell32.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <utils/Logger.h>
#include <utils/OsuUtils.h>

std::string getBaseDataPath() {
    #if defined(_WIN32)
    PWSTR pszPath = nullptr;
    std::string path;

    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath);

    if (SUCCEEDED(hr)) {
        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, NULL, 0, NULL, NULL);
        if (requiredSize > 0) {
            path.resize(requiredSize - 1);
            WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &path[0], requiredSize, NULL, NULL);
        }
    }
    
    if (pszPath) {
        CoTaskMemFree(pszPath);
    }
    
    if (path.empty()) {
        const char* envPath = std::getenv("LOCALAPPDATA");
        if (envPath) {
            return std::string(envPath);
        }
        throw std::runtime_error("Failed to retrieve Windows Local AppData path.");
    }
    
    return path;

    #elif defined(__linux__)
    const char* homeDir = std::getenv("HOME");
    if (homeDir == nullptr) {
        throw std::runtime_error("HOME environment variable not set on Linux.");
    }

    std::string wineAppDataPath = std::string(homeDir) + "/.wine/drive_c/users/" + std::getenv("USER") + "/AppData/Local";
    try {
        if (std::filesystem::exists(wineAppDataPath)) {
            return wineAppDataPath;
        }
    } catch (...) { }
    
    std::string defaultPath = std::string(homeDir) + "/.local/share/osu-wine/drive_c/users/" + std::getenv("USER") + "/AppData/Local";
    return defaultPath;

    #elif defined(__APPLE__)
    const char* homeDir = std::getenv("HOME");
    if (homeDir == nullptr) {
        throw std::runtime_error("HOME environment variable not set on macOS.");
    }
    return std::string(homeDir) + "/.wine/drive_c/users/" + std::getenv("USER") + "/AppData/Local";
    #else
    throw std::runtime_error("Unsupported operating system for osu! database path resolution.");
    #endif
}

std::string OsuUtils::getOsuDatabasePath() {
    try {
        std::string base = getBaseDataPath();
        #if defined(_WIN32)
        return base + "\\osu!\\osu!.db";
        #else
        return base + "/osu!/osu!.db";
        #endif
    } catch (const std::exception& e) {
        throw; 
    }
}

void OsuUtils::parseOsuDatabase()
{
    if (parserThread_.joinable()) {
        GAME_LOG_WARN("OsuUtils: Database parsing is already in progress. Ignoring new request.");
        return;
    }

    std::string filePath = getOsuDatabasePath();
    GAME_LOG_DEBUG("OsuUtils: Attempting to read osu! database at: " + filePath);
    parserThread_ = std::thread(&OsuUtils::parseDatabaseThread, this, filePath);
}