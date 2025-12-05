#ifndef TEXT_OBJECT_H
#define TEXT_OBJECT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <iostream>
#include <algorithm>

enum TextXAlignment {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

enum TextYAlignment {
    ALIGN_TOP,
    ALIGN_MIDDLE,
    ALIGN_BOTTOM
};

enum TextAlignment {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
};

class TextObject {
public:
    TextObject(SDL_Renderer* renderer, const std::string& fontPath, int fontSize);
    
    ~TextObject();

    void setText(const std::string& newText);
    void setPosition(int x, int y);
    void setColor(SDL_Color color);
    void render();

    void getPosition(float& x, float& y) const {
        x = destRect_.x;
        y = destRect_.y;
    }

    float getTextGap() const { return textGap_; }
    float getLineCount() const {
        if (text_.empty()) {
            return 0.0f;
        }
        return static_cast<float>(std::count(text_.begin(), text_.end(), '\n') + 1);
    }

    void setTextGap(float gap);
    void setAlignment(TextAlignment alignment);

    void setXAlignment(TextXAlignment alignment);
    void setYAlignment(TextYAlignment alignment);

    float getRenderedWidth() const { return destRect_.w; }
    float getRenderedHeight() const { return destRect_.h; }
private:
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    SDL_Texture* texture_ = nullptr;

    std::string text_ = "";
    SDL_Color color_ = {255, 255, 255, 255};

    SDL_FRect destRect_ = {0, 0, 0, 0};

    TextXAlignment alignmentX_ = ALIGN_LEFT;
    TextYAlignment alignmentY_ = ALIGN_TOP;

    TextAlignment textAlignment_ = TEXT_ALIGN_LEFT;

    float anchorX_ = 0.0f;
    float anchorY_ = 0.0f;
    float textGap_ = 0.0f;

    bool _loadFont(const std::string& path, int size);
    bool _updateFontStyle();
    void _destroyTexture();
    void _updateTexture();
    void _updatePosition();
};

#endif