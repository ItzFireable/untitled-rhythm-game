#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <SDL3/SDL.h>
#include <string>
#include "TextObject.h"
#include "objects/FPSCounter.h"

class DebugInfo : public FPSCounter {
public:
    DebugInfo(SDL_Renderer* renderer, const std::string& fontPath, int fontSize, float yPos);
    ~DebugInfo() = default;

    void update() override;
    void render(SDL_Renderer* renderer) override;
private:
};

#endif