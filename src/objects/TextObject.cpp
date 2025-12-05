#include "objects/TextObject.h"
#include "utils/Logger.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <vector>
#include <sstream>

std::vector<std::string> splitStringByNewlines(const std::string& s) {
    std::vector<std::string> lines;
    std::stringstream ss(s);
    std::string line;
    
    while (std::getline(ss, line, '\n')) {
        lines.push_back(line);
    }
    
    if (s.length() > 0 && s.back() == '\n') {
        lines.push_back("");
    }
    
    return lines;
}

TextObject::TextObject(SDL_Renderer* renderer, const std::string& fontPath, int fontSize) 
    : renderer_(renderer) 
{
    if (!_loadFont(fontPath, fontSize)) {
        GAME_LOG_ERROR("Failed to load font from path: " + fontPath);
    }
}

TextObject::~TextObject() {
    _destroyTexture();
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
}

void TextObject::setText(const std::string& newText) {
    if (text_ != newText) {
        text_ = newText;
        _updateTexture();
    }
}

void TextObject::setPosition(int x, int y) {
    anchorX_ = x;
    anchorY_ = y;
    _updatePosition();
}

void TextObject::setColor(SDL_Color color) {
    if (color_.r != color.r || color_.g != color.g || color_.b != color.b || color_.a != color.a) {
        color_ = color;
        _updateTexture();
    }
}

void TextObject::setAlignment(TextAlignment alignment) {
    if (textAlignment_ != alignment) {
        textAlignment_ = alignment;
        _updatePosition();
    }
}

void TextObject::setXAlignment(TextXAlignment alignment) {
    if (alignmentX_ != alignment) {
        alignmentX_ = alignment;
        _updatePosition();
    }
}

void TextObject::setYAlignment(TextYAlignment alignment) {
    if (alignmentY_ != alignment) {
        alignmentY_ = alignment;
        _updatePosition();
    }
}

void TextObject::render() {
    if (texture_ && renderer_) {
        SDL_RenderTexture(renderer_, texture_, NULL, &destRect_);
    }
}

bool TextObject::_loadFont(const std::string& path, int size) {
    if (font_) TTF_CloseFont(font_);
    
    font_ = TTF_OpenFont(path.c_str(), size);
    if (!font_) {
        return false;
    }

    TTF_SetFontHinting(font_, TTF_HINTING_LIGHT_SUBPIXEL);
    return true;
}

void TextObject::setTextGap(float gap) {
    if (textGap_ != gap) {
        textGap_ = gap;
        _updateTexture();
    }
}

void TextObject::_destroyTexture() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

void TextObject::_updatePosition() {
    switch (alignmentX_) {
        case ALIGN_LEFT:
            destRect_.x = anchorX_;
            break;

        case ALIGN_CENTER:
            destRect_.x = anchorX_ - (destRect_.w / 2.0f);
            break;

        case ALIGN_RIGHT:
            destRect_.x = anchorX_ - destRect_.w;
            break;
    }

    switch (alignmentY_) {
        case ALIGN_TOP:
            destRect_.y = anchorY_;
            break;

        case ALIGN_MIDDLE:
            destRect_.y = anchorY_ - (destRect_.h / 2.0f);
            break;

        case ALIGN_BOTTOM:
            destRect_.y = anchorY_ - destRect_.h;
            break;
    }
}

void TextObject::_updateTexture() {
    if (!font_ || renderer_ == nullptr || text_.empty()) {
        destRect_.w = 0.0f;
        destRect_.h = 0.0f;
        return;
    }

    std::vector<std::string> lines = splitStringByNewlines(text_);
    
    int lineSkip = TTF_GetFontLineSkip(font_);
    int totalHeight = lineSkip * lines.size();
    int maxWidth = 0;

    for (const std::string& line : lines) {
        if (line.empty()) continue;
        int w, h;
        TTF_GetStringSize(font_, line.c_str(), line.length(),&w, &h);
        if (w > maxWidth) {
            maxWidth = w;
        }
    }

    SDL_Texture* finalTexture = SDL_CreateTexture(
        renderer_, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_TARGET, 
        maxWidth, 
        totalHeight
    );
    
    if (!finalTexture) {
        GAME_LOG_ERROR("Failed to create render target texture: " + std::string(SDL_GetError()));
        destRect_.w = 0.0f; destRect_.h = 0.0f; return;
    }

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer_);
    SDL_SetRenderTarget(renderer_, finalTexture);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0); 
    SDL_RenderClear(renderer_);

    float currentY = 0.0f;

    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        
        if (!line.empty()) {
            SDL_Surface* tempSurface = TTF_RenderText_Blended(font_, line.c_str(), line.length(), color_);
            if (!tempSurface) {
                continue;
            };

            SDL_Surface* lineSurface = SDL_ConvertSurface(
                tempSurface, 
                SDL_PIXELFORMAT_RGBA8888
            );
            SDL_DestroySurface(tempSurface);
            
            if (!lineSurface) {
                continue;
            };

            int lineW = lineSurface->w;
            int lineH = lineSurface->h; 

            float lineOffsetX = 0.0f;
            switch (textAlignment_) {
                case TEXT_ALIGN_CENTER:
                    lineOffsetX = (maxWidth - lineW) / 2.0f;
                    break;
                case TEXT_ALIGN_RIGHT:
                    lineOffsetX = (maxWidth - lineW);
                    break;
                case TEXT_ALIGN_LEFT:
                default:
                    lineOffsetX = 0.0f;
                    break;
            }

            SDL_Texture* lineTexture = SDL_CreateTextureFromSurface(renderer_, lineSurface);
            SDL_DestroySurface(lineSurface); 

            if (lineTexture) {
                SDL_FRect lineDest = {
                    lineOffsetX,               
                    currentY,           
                    (float)lineW,
                    (float)lineH
                };
                
                SDL_RenderTexture(renderer_, lineTexture, NULL, &lineDest);
                SDL_DestroyTexture(lineTexture);
            }
        }
        
        currentY += lineSkip + textGap_;
    }

    SDL_SetRenderTarget(renderer_, oldTarget);
    _destroyTexture();
    texture_ = finalTexture;
    
    destRect_.w = (float)maxWidth;
    destRect_.h = (float)totalHeight;
    _updatePosition();
}