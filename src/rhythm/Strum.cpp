#include <rhythm/Strum.h>
#include <rhythm/Playfield.h>

Strum::Strum(float x, float y, float width, float height, int column)
    : x_(x), y_(y), width_(width), height_(height), strumTexture_(nullptr), column_(column) {}

Strum::~Strum() {
    if (textureIsOwned_ && strumTexture_) {
        SDL_DestroyTexture(strumTexture_);
    }
}

void Strum::setRenderTexture(SDL_Texture* texture, bool ownedByStrum) {
    if (textureIsOwned_ && strumTexture_ && texture != strumTexture_) {
        SDL_DestroyTexture(strumTexture_);
    }
    
    strumTexture_ = texture;
    textureIsOwned_ = ownedByStrum;
}

void Strum::loadTexture(SDL_Renderer* renderer) {
    if (textureIsOwned_ && strumTexture_) {
        SDL_DestroyTexture(strumTexture_);
    }

    Playfield* playfield = getPlayfield();
    if (!playfield) return;
    
    SkinUtils* skinUtils = playfield->getSkinUtils();
    if (!skinUtils) return;

    std::string filePath = skinUtils->getFilePathForSkinElement("notes/strum");
    
    strumTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
    textureIsOwned_ = true;
    
    if (!strumTexture_) {
        GAME_LOG_ERROR("Failed to load strum texture from " + filePath);
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
    SDL_FRect strumRect = {
        x_ + padding_left_,
        y_ + padding_top_,
        width_ - padding_left_ - padding_right_,
        height_ - padding_top_ - padding_bottom_
    };
    
    if (strumTexture_) {
        SDL_RenderTexture(renderer, strumTexture_, NULL, &strumRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderRect(renderer, &strumRect);
    }
}