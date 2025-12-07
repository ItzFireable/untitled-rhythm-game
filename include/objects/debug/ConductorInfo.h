#ifndef CONDUCTOR_INFO_H
#define CONDUCTOR_INFO_H

#include <SDL3/SDL.h>
#include <string>
#include "rhythm/Conductor.h"
#include "objects/debug/FPSCounter.h"
#include "objects/TextObject.h"

class ConductorInfo : public FPSCounter {
public:
    ConductorInfo(SDL_Renderer* renderer, const std::string& fontPath, int fontSize, float yPos);
    ~ConductorInfo() = default;

    void update() override;
    void render(SDL_Renderer* renderer) override;
    void setConductor(Conductor* conductor) { conductor_ = conductor; }
private:
    Conductor* conductor_ = nullptr;
};

#endif