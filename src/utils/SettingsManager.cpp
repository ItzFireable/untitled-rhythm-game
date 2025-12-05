#include "utils/SettingsManager.h"

void SettingsManager::loadSettings()
{
    std::string configData = Utils::readFile("settings.vsc");
    SettingsConfigMap defaultConfig;

    if (configData.empty())
    {
        GAME_LOG_WARN("SettingsManager: Could not load settings config. Using default settings.");
        settingConfig_ = defaultConfig;

        saveSettings();
        return;
    }

    std::stringstream ss(configData);
    std::string line;
    
    while (std::getline(ss, line)) {
        line = Utils::trim(line);
        if (line.empty() || line.rfind("//", 0) == 0) continue;

        if (line.front() == '[' && line.back() == ']') continue;

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            key = Utils::trim(key);
            value = Utils::trim(value);
            settingConfig_[key] = value;
        }
    }

    for (const auto& [key, value] : defaultConfig) {
        if (settingConfig_.find(key) == settingConfig_.end()) {
            settingConfig_[key] = value;
        }
    }
}

void SettingsManager::saveSettings() const
{
    std::ofstream configFile("settings.vsc");
    if (!configFile.is_open()) {
        GAME_LOG_ERROR("SettingsManager: Could not open settings.vsc for writing.");
        return;
    }

    for (const auto& [key, value] : settingConfig_) {
        configFile << key << " = " << value << "\n";
    }

    configFile.close();
}

template <>
std::string SettingsManager::getSetting<std::string>(const std::string &propertyName, const std::string &defaultValue) const
{
    auto it = settingConfig_.find(propertyName);
    if (it != settingConfig_.end())
    {
        return it->second;
    }
    return defaultValue;
}