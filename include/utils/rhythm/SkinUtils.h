#ifndef SKINUTILS_H
#define SKINUTILS_H

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <utils/Utils.h>
#include <system/Logger.h>
#include <SDL3/SDL_iostream.h>

using SkinConfigMap = std::map<std::string, std::string>;

enum HudAlignmentX
{
    HUD_ALIGN_LEFT,
    HUD_ALIGN_CENTER,
    HUD_ALIGN_RIGHT
};

enum HudAlignmentY
{
    HUD_ALIGN_TOP,
    HUD_ALIGN_MIDDLE,
    HUD_ALIGN_BOTTOM
};

class SkinUtils
{
public:
    SkinUtils() = default;
    ~SkinUtils() = default;

    SkinConfigMap getDefaultSkinConfig();

    std::vector<std::string> allowedSkinConfigHeaders = {
        "GAMEPLAY",
        "NOTES",
        "HUD",
        "HITERROR",
        "JUDGEMENTS",
        "ACCURACY"
    };

    bool isAllowedHeader(const std::string &header);

    template <typename T>
    T getSkinProperty(const std::string &propertyName, const T &defaultValue) const
    {
        auto it = skinConfig_.find(propertyName);
        if (it != skinConfig_.end())
        {
            std::istringstream iss(it->second);
            T value;
            if (iss >> value)
            {
                return value;
            }
            else
            {
                GAME_LOG_WARN("SkinUtils: Failed to convert skin property '" + propertyName + "' to desired type. Using default value.");
            }
        }
        return defaultValue;
    }

    std::string getFontName() const;

    HudAlignmentX getHudAlignmentX(const std::string &propertyName, HudAlignmentX defaultValue) const;
    HudAlignmentY getHudAlignmentY(const std::string &propertyName, HudAlignmentY defaultValue) const;

    HudAlignmentX getAccuracyXAlignment() const;
    HudAlignmentY getAccuracyYAlignment() const;

    HudAlignmentX getJudgementCounterXAlignment() const;
    HudAlignmentY getJudgementCounterYAlignment() const;

    void loadSkin(const std::string &skinName);
    void saveSkin();
    const std::string &getSkinName() const { return skinName_; }

    std::string getFilePathForSkinElement(const std::string &elementName) const;

private:
    std::string skinName_ = "default";
    SkinConfigMap skinConfig_ = SkinConfigMap();
};

template <>
std::string SkinUtils::getSkinProperty<std::string>(const std::string &propertyName, const std::string &defaultValue) const;

template <>
bool SkinUtils::getSkinProperty<bool>(const std::string &propertyName, const bool &defaultValue) const;

#endif