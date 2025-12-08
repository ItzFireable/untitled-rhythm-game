#ifndef HOLDNOTE_H
#define HOLDNOTE_H

#include <rhythm/JudgementSystem.h>
#include <objects/rhythm/Note.h>
#include <utils/rhythm/ChartUtils.h>

class Playfield;

class HoldNote : public Note
{
public:
    HoldNote(float x, float y, float width, float height, int column = 0);
    ~HoldNote();

    void update(float deltaTime) override;
    void render(SDL_Renderer *renderer) override;
    
    void loadTextures(SDL_Renderer* renderer, Playfield* pf) override;
    void destroyTextures() override;

    float getEndTime() const { return endTime_; }
    bool hasEndTime() const { return endTime_ > 0.0f; }

    void setEndTime(float endTime) { 
        endTime_ = endTime;
        if (endTime_ <= getTime()) {
            endTime_ = 0.0f;
        }
     }
    void setEndY(float y) { endY_ = y; }
    float getEndY() const { return endY_; }
    void setIsHolding(bool holding) { isHolding_ = holding; }
    bool isHolding() const { return isHolding_; }

    void startFadeOut() { isFadingOut_ = true; }
    bool isFadingOut() const { return isFadingOut_; }
    bool shouldDespawn() const override;
    void despawnNote() override { startFadeOut(); };

    void setHoldEndHeight(float height) { holdEndHeight_ = height; }
    float getHoldEndHeight() const { return holdEndHeight_; }

private:
    int column_;
    float endTime_ = -1.0f;
    bool isHolding_ = false;
    bool isFadingOut_ = false;

    float endY_ = 0.0f;
    float holdEndHeight_ = -1.0f;
    
    static SDL_Texture* sharedHoldStartTexture_;
    static SDL_Texture* sharedHoldBodyTexture_;
    static SDL_Texture* sharedHoldEndTexture_;
};

#endif