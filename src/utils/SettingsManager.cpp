#include "utils/SettingsManager.h"

std::string SettingsManager::parseSectionHeader(const std::string& line) const
{
    return line.substr(1, line.length() - 2);
}

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
    std::string currentSection = "";
    
    while (std::getline(ss, line)) {
        line = Utils::trim(line);
        
        if (line.empty() || line.rfind("//", 0) == 0) continue;
        if (line.front() == '[' && line.back() == ']') {
            currentSection = parseSectionHeader(line);
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            key = Utils::trim(key);
            value = Utils::trim(value);
            
            std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            settingConfig_[fullKey] = value;
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

    std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;
    
    for (const auto& [key, value] : settingConfig_) {
        size_t dotPos = key.find('.');
        if (dotPos != std::string::npos) {
            std::string section = key.substr(0, dotPos);
            std::string settingName = key.substr(dotPos + 1);
            sections[section].push_back({settingName, value});
        } else {
            sections[""].push_back({key, value});
        }
    }

    for (const auto& [section, settings] : sections) {
        if (!section.empty()) {
            configFile << "[" << section << "]\n";
        }
        
        for (const auto& [key, value] : settings) {
            configFile << key << " = " << value << "\n";
        }
        
        configFile << "\n";
    }

    configFile.close();
}

template <>
void SettingsManager::setSetting<std::string>(const std::string &propertyName, const std::string &value) const
{
    settingConfig_[propertyName] = value;
}

template <>
void SettingsManager::setSetting<bool>(const std::string &propertyName, const bool &value) const
{
    settingConfig_[propertyName] = value ? "true" : "false";
}

template <>
std::string SettingsManager::getSetting<std::string>(const std::string &propertyName, const std::string &defaultValue) const 
{
    auto it = settingConfig_.find(propertyName);
    if (it != settingConfig_.end())
    {
        return it->second;
    }

    setSetting(propertyName, defaultValue);
    return defaultValue;
}

template <>
bool SettingsManager::getSetting<bool>(const std::string &propertyName, const bool &defaultValue) const
{
    auto it = settingConfig_.find(propertyName);
    if (it != settingConfig_.end())
    {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        return (val == "true");
    }

    setSetting(propertyName, defaultValue);
    return defaultValue;
}