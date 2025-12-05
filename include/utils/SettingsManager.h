#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <utils/Utils.h>
#include <utils/Logger.h>

using SettingsConfigMap = std::map<std::string, std::string>;

class SettingsManager
{
public:
    SettingsManager() = default;
    ~SettingsManager() = default;

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
        return defaultValue;
    }

    void loadSettings();
    void saveSettings() const;
private:
    SettingsConfigMap settingConfig_;
};

template <>
std::string SettingsManager::getSetting<std::string>(const std::string &propertyName, const std::string &defaultValue) const;

#endif