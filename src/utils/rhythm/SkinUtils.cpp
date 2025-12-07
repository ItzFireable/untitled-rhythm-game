#include "utils/rhythm/SkinUtils.h"

bool SkinUtils::isAllowedHeader(const std::string& header) {
    return std::find(allowedSkinConfigHeaders.begin(), allowedSkinConfigHeaders.end(), header) != allowedSkinConfigHeaders.end();
}

template<>
std::string SkinUtils::getSkinProperty<std::string>(const std::string& propertyName, const std::string& defaultValue) const {
    auto it = skinConfig_.find(propertyName);
    if (it != skinConfig_.end()) {
        return it->second;
    }
    return defaultValue;
}

template<>
bool SkinUtils::getSkinProperty<bool>(const std::string& propertyName, const bool& defaultValue) const {
    auto it = skinConfig_.find(propertyName);
    if (it != skinConfig_.end()) {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        return (val == "true");
    }
    return defaultValue;
}

std::string SkinUtils::getFontName() const
{
    std::string fontName = SkinUtils::getSkinProperty<std::string>("fontName", "GoogleSansCode-Bold");
    return "assets/fonts/" + fontName + ".ttf";
}

HudAlignmentX SkinUtils::getHudAlignmentX(const std::string &propertyName, HudAlignmentX defaultValue) const
{
    std::string alignStr = SkinUtils::getSkinProperty<std::string>(propertyName, "");
    if (alignStr == "left")
        return HUD_ALIGN_LEFT;
    if (alignStr == "center")
        return HUD_ALIGN_CENTER;
    if (alignStr == "right")
        return HUD_ALIGN_RIGHT;
    return defaultValue;
}

HudAlignmentY SkinUtils::getHudAlignmentY(const std::string &propertyName, HudAlignmentY defaultValue) const
{
    std::string alignStr = SkinUtils::getSkinProperty<std::string>(propertyName, "");
    if (alignStr == "top")
        return HUD_ALIGN_TOP;
    if (alignStr == "middle")
        return HUD_ALIGN_MIDDLE;
    if (alignStr == "bottom")
        return HUD_ALIGN_BOTTOM;
    return defaultValue;
}

HudAlignmentX SkinUtils::getJudgementCounterXAlignment() const
{
    return SkinUtils::getHudAlignmentX("judgementCounterAlignX", HUD_ALIGN_LEFT);
}
HudAlignmentY SkinUtils::getJudgementCounterYAlignment() const
{
    return SkinUtils::getHudAlignmentY("judgementCounterAlignY", HUD_ALIGN_MIDDLE);
}

HudAlignmentX SkinUtils::getAccuracyXAlignment() const
{
    return SkinUtils::getHudAlignmentX("accuracyAlignX", HUD_ALIGN_CENTER);
}
HudAlignmentY SkinUtils::getAccuracyYAlignment() const
{
    return SkinUtils::getHudAlignmentY("accuracyAlignY", HUD_ALIGN_TOP);
}

SkinConfigMap SkinUtils::getDefaultSkinConfig()
{
    SkinConfigMap config;

    // TODO: Populate with actual default skin config values

    return config;
}

std::string SkinUtils::getFilePathForSkinElement(const std::string& elementName) const
{
    std::string path = "assets/skins/" + skinName_ + "/" + elementName;
    std::vector<std::string> extensions = {".png", ".jpg"};

    for (const auto& ext : extensions)
    {
        std::string fullPath = path + ext;
        SDL_IOStream * file = SDL_IOFromFile(fullPath.c_str(), "rb");
        if (file)
        {
            SDL_CloseIO(file);
            return fullPath;
        }
    }
    
    return "assets/skins/default/" + elementName + ".png";
}

void SkinUtils::loadSkin(const std::string& skinName)
{
    skinName_ = skinName;

    std::string configPath = "assets/skins/" + skinName_ + "/skin.vsc";
    std::string configData = Utils::readFile(configPath);

    SkinConfigMap defaultConfig = getDefaultSkinConfig();

    if (configData.empty())
    {
        GAME_LOG_WARN("SkinUtils: Could not load skin config at " + configPath + ". Using default skin config.");
        skinConfig_ = defaultConfig;
        return;
    }

    std::stringstream ss(configData);
    std::string line, currentSection;
    
    while (std::getline(ss, line)) {
        line = Utils::trim(line);
        if (line.empty() || line.rfind("//", 0) == 0) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
        } else {
            std::string headerCheck = currentSection;
            if (isAllowedHeader(headerCheck)) {
                size_t eqPos = line.find('=');
                
                if (eqPos != std::string::npos) {
                    std::string key = line.substr(0, eqPos);
                    std::string value = line.substr(eqPos + 1);

                    key = Utils::trim(key);
                    value = Utils::trim(value);
                    skinConfig_[key] = value;
                }
            }
        }
    }

    for (const auto& [key, value] : defaultConfig) {
        if (skinConfig_.find(key) == skinConfig_.end()) {
            skinConfig_[key] = value;
        }
    }
}

void SkinUtils::saveSkin()
{
    std::string configPath = "assets/skins/" + skinName_ + "/skin.vsc";
    std::stringstream ss;

    std::map<std::string, std::vector<std::pair<std::string, std::string>>> sections;

    for (const auto& header : allowedSkinConfigHeaders) {
        sections[header] = {};
    }

    for (const auto& [key, value] : skinConfig_) {
        for (const auto& header : allowedSkinConfigHeaders) {
            if (key.find(header) == 0) {
                sections[header].emplace_back(key, value);
                break;
            }
        }
    }

    for (const auto& header : allowedSkinConfigHeaders) {
        ss << "[" << header << "]\n";
        for (const auto& [key, value] : sections[header]) {
            ss << key << " = " << value << "\n";
        }
        ss << "\n";
    }

    std::ofstream configFile(configPath);
    if (!configFile.is_open()) {
        GAME_LOG_ERROR("SettingsManager: Could not open settings.vsc for writing.");
        return;
    }

    configFile << ss.str();
    configFile.close();
}