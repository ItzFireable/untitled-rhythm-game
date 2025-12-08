#ifndef STRUM_H
#define STRUM_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <utils/Utils.h>
#include <system/Logger.h>
#include <rhythm/Conductor.h>
#include <utils/rhythm/SkinUtils.h>
#include <vector>

class Playfield;

class Strum {
public:
    Strum(float x, float y, float width, float height, int column = 0);
    ~Strum();

    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setPadding(float top, float bottom, float left, float right);

    virtual void update(float deltaTime);
    virtual void render(SDL_Renderer* renderer);

    virtual void loadTextures(SDL_Renderer* renderer, Playfield* playfield);
    virtual void destroyTextures();

    bool isPressed() const {
        return isPressed_;
    }
    void setPressed(bool pressed) {
        isPressed_ = pressed;
    }

    int getColumn() const {
        return column_;
    }

    void getPosition(float& x, float& y) const {
        x = x_;
        y = y_;
    }

    void getSize(float& width, float& height) const {
        width = width_;
        height = height_;
    }

    void getPadding(float& top, float& bottom, float& left, float& right) const {
        top = padding_top_;
        bottom = padding_bottom_;
        left = padding_left_;
        right = padding_right_;
    }

    Playfield* getPlayfield() const {
        return playfield_;
    }
    void setPlayfield(Playfield* playfield) {
        playfield_ = playfield;
    }

protected:
    std::map<std::string, SDL_Texture*> textures_;
    std::map<SDL_Texture*, bool> ownedTextures_;

private:
    int column_ = 0;
    bool isPressed_ = false;
    
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 100.0f;
    float height_ = 100.0f;

    float padding_top_ = 4.0f;
    float padding_bottom_ = 4.0f;
    float padding_left_ = 4.0f;
    float padding_right_ = 4.0f;
    
    Playfield* playfield_ = nullptr;
    
    static SDL_Texture* sharedStrumTexture_;
    static SDL_Texture* sharedStrumPressTexture_;
};

#endif