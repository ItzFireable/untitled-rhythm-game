#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <utils/Utils.h>
#include <system/Logger.h>

using SettingsConfigMap = std::map<std::string, std::string>;

class SettingsManager
{
public:
    SettingsManager() = default;
    ~SettingsManager() = default;

    template <typename T>
    void setSetting(const std::string &propertyName, const T &value) const
    {
        settingConfig_[propertyName] = Utils::toString(value);
    }

    template <typename T>
    T getSetting(const std::string &propertyName, const T &defaultValue) const
    {
        auto it = settingConfig_.find(propertyName);
        if (it != settingConfig_.end())
        {
            std::istringstream iss(it->second);
            T value;
            if (iss >> value)
            {
                return value;
            }
            else
            {
                GAME_LOG_WARN("Settings: Failed to fetch property '" + propertyName + "'. Using default value.");
            }
        }

        setSetting(propertyName, defaultValue);
        return defaultValue;
    }

    void loadSettings();
    void saveSettings() const;
private:
    mutable SettingsConfigMap settingConfig_;
    
    // Helper to parse section headers
    std::string parseSectionHeader(const std::string& line) const;
};

template <>
std::string SettingsManager::getSetting<std::string>(const std::string &propertyName, const std::string &defaultValue) const;
template <>
bool SettingsManager::getSetting<bool>(const std::string &propertyName, const bool &defaultValue) const;

template <>
void SettingsManager::setSetting<std::string>(const std::string &propertyName, const std::string &value) const;
template <>
void SettingsManager::setSetting<bool>(const std::string &propertyName, const bool &value) const;

#endif