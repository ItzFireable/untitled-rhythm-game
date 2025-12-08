#ifndef INFO_STACK_MANAGER_H
#define INFO_STACK_MANAGER_H

#include <SDL3/SDL.h>
#include <vector>
#include "objects/debug/FPSCounter.h"

class InfoStackManager {
public:
    InfoStackManager(float startY = 8.0f, float spacing = 8.0f);
    ~InfoStackManager();

    void addInfo(FPSCounter* info);
    void removeInfo(FPSCounter* info);
    void clear();
    
    void updatePositions();
    void updateAll();
    void renderAll(SDL_Renderer* renderer);

    void setStartY(float y) { startY_ = y; updatePositions(); }
    void setSpacing(float spacing) { spacing_ = spacing; updatePositions(); }

private:
    std::vector<FPSCounter*> infoObjects_;
    float startY_ = 7.0f;
    float spacing_ = 4.0f;
};

#endif