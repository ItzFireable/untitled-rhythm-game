#include "utils/SkinUtils.h"

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

std::string SkinUtils::getFontName() const
{
    std::string fontName = SkinUtils::getSkinProperty<std::string>("fontName", "GoogleSansCode-Bold");
    return "assets/fonts/" + fontName + ".ttf";
}
float SkinUtils::getFontSize() const
{
    return SkinUtils::getSkinProperty<float>("fontSize", 24.0f);
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

bool SkinUtils::getShowJudgementCounter() const
{
    return SkinUtils::getSkinProperty<std::string>("showJudgementCounter", "true") == "true";
}
bool SkinUtils::getShowAccuracyDisplay() const
{
    return SkinUtils::getSkinProperty<std::string>("showAccuracyDisplay", "true") == "true";
}
HudAlignmentX SkinUtils::getAccuracyXAlignment() const
{
    return SkinUtils::getHudAlignmentX("accuracyAlignX", HUD_ALIGN_CENTER);
}
HudAlignmentY SkinUtils::getAccuracyYAlignment() const
{
    return SkinUtils::getHudAlignmentY("accuracyAlignY", HUD_ALIGN_TOP);
}
float SkinUtils::getAccuracyXOffset() const
{
    return SkinUtils::getSkinProperty<float>("accuracyX", 0.0f);
}
float SkinUtils::getAccuracyYOffset() const
{
    return SkinUtils::getSkinProperty<float>("accuracyY", 16.0f);
}
HudAlignmentX SkinUtils::getJudgementCounterXAlignment() const
{
    return SkinUtils::getHudAlignmentX("judgementCounterAlignX", HUD_ALIGN_LEFT);
}
HudAlignmentY SkinUtils::getJudgementCounterYAlignment() const
{
    return SkinUtils::getHudAlignmentY("judgementCounterAlignY", HUD_ALIGN_MIDDLE);
}
float SkinUtils::getJudgementCounterXOffset() const
{
    return SkinUtils::getSkinProperty<float>("judgementCounterX", 16.0f);
}
float SkinUtils::getJudgementCounterYOffset() const
{
    return SkinUtils::getSkinProperty<float>("judgementCounterY", 0.0f);
}
float SkinUtils::getHoldCutAmount() const
{
    return SkinUtils::getSkinProperty<float>("holdCut", 20.0f);
}
float SkinUtils::getPlayfieldWidth() const
{
    return SkinUtils::getSkinProperty<float>("playfieldWidth", 550.0f);
}
float SkinUtils::getPadding() const
{
    return SkinUtils::getSkinProperty<float>("playfieldPadding", 16.0f);
}
float SkinUtils::getScrollSpeed() const
{
    return SkinUtils::getSkinProperty<float>("scrollSpeed", 1650.0f);
}
float SkinUtils::getStrumlineOffset() const
{
    return SkinUtils::getSkinProperty<float>("strumlineOffset", 0.0f);
}
float SkinUtils::getStrumlineGap() const
{
    return SkinUtils::getSkinProperty<float>("strumlineGap", 0.0f);
}
float SkinUtils::getJudgementPopupOffset() const
{
    return SkinUtils::getSkinProperty<float>("judgementPopupOffset", 0.0f);
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