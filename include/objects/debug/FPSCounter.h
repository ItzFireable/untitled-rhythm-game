#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <SDL3/SDL.h>
#include <string>
#include "objects/TextObject.h"

struct AppContext;

class FPSCounter {
public:
    FPSCounter(SDL_Renderer* renderer, const std::string& fontPath, int fontSize, float yPos);
    ~FPSCounter();

    void setAppContext(AppContext* appContext) { appContext_ = appContext; }
    AppContext* getAppContext() const { return appContext_; }

    virtual void update();
    virtual void render(SDL_Renderer* renderer);

    void setYPosition(float yPos) {
        float x, y;
        textObject_->getPosition(x, y);
        textObject_->setPosition((int)x, (int)yPos);
    }

    float getYPosition() const {
        float y;
        textObject_->getPosition(y, y);
        return y;
    }

    float getHeight() const {
        return textObject_->getRenderedHeight();
    }
protected:
    AppContext* appContext_ = nullptr;

    TextObject* textObject_ = nullptr;
    SDL_FRect* backgroundRect_ = nullptr;
    
    Uint64 lastTick_ = 0;
    Uint64 perfFrequency_ = 0;
    
    Uint32 frameCount_ = 0;
    float frameAccumulator_ = 0.0f;

    float latestFrameTimeMs_ = 0.0f;
    long memoryUsageKb_ = 0;

private:
    long getAppMemoryUsageKb();
};

#endif