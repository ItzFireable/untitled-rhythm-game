#include <objects/rhythm/Strum.h>
#include <rhythm/Playfield.h>

SDL_Texture* Strum::sharedStrumTexture_ = nullptr;
SDL_Texture* Strum::sharedStrumPressTexture_ = nullptr;

Strum::Strum(float x, float y, float width, float height, int column)
    : x_(x), y_(y), width_(width), height_(height), column_(column) {
    textures_["strum"] = sharedStrumTexture_;
    ownedTextures_[sharedStrumTexture_] = false;
    
    textures_["strumPress"] = sharedStrumPressTexture_;
    ownedTextures_[sharedStrumPressTexture_] = false;
}

Strum::~Strum() {
    for (const auto& pair : ownedTextures_) {
        if (pair.second && pair.first) {
            SDL_DestroyTexture(pair.first);
        }
    }
}

void Strum::loadTextures(SDL_Renderer* renderer, Playfield* playfield) {
    SkinUtils* skinUtils = playfield->getSkinUtils();
    if (!skinUtils) return;

    if (sharedStrumTexture_ == nullptr) {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/strum");
        sharedStrumTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedStrumTexture_) {
            GAME_LOG_ERROR("Failed to load shared strum texture from " + filePath);
        }
    }

    if (sharedStrumPressTexture_ == nullptr) {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/strumPress");
        sharedStrumPressTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedStrumPressTexture_) {
            GAME_LOG_ERROR("Failed to load shared strum press texture from " + filePath);
        }
    }
    
    textures_["strum"] = sharedStrumTexture_;
    ownedTextures_[sharedStrumTexture_] = false;
    
    textures_["strumPress"] = sharedStrumPressTexture_;
    ownedTextures_[sharedStrumPressTexture_] = false;
}

void Strum::destroyTextures() {
    if (sharedStrumTexture_ != nullptr) {
        SDL_DestroyTexture(sharedStrumTexture_);
        sharedStrumTexture_ = nullptr;
    }

    if (sharedStrumPressTexture_ != nullptr) {
        SDL_DestroyTexture(sharedStrumPressTexture_);
        sharedStrumPressTexture_ = nullptr;
    }
}

void Strum::setPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void Strum::setSize(float width, float height) {
    width_ = width;
    height_ = height;
}

void Strum::setPadding(float top, float bottom, float left, float right) {
    padding_top_ = top;
    padding_bottom_ = bottom;
    padding_left_ = left;
    padding_right_ = right;
}

void Strum::update(float deltaTime) {}

void Strum::render(SDL_Renderer* renderer) {
    std::string textureKey = isPressed_ ? "strumPress" : "strum";
    SDL_Texture* textureToRender = textures_.count(textureKey) ? textures_[textureKey] : nullptr;

    SDL_FRect strumRect = {
        x_ + padding_left_,
        y_ + padding_top_,
        width_ - padding_left_ - padding_right_,
        height_ - padding_top_ - padding_bottom_
    };
    
    if (textureToRender) {
        SDL_RenderTexture(renderer, textureToRender, NULL, &strumRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderRect(renderer, &strumRect);
    }
}