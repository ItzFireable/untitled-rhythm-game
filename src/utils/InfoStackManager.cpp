#include "utils/InfoStackManager.h"

InfoStackManager::InfoStackManager(float startY, float spacing)
    : startY_(startY), spacing_(spacing) {}

InfoStackManager::~InfoStackManager() {
    clear();
}

void InfoStackManager::addInfo(FPSCounter* info) {
    if (!info) return;
    infoObjects_.push_back(info);
    updatePositions();
}

void InfoStackManager::removeInfo(FPSCounter* info) {
    if (!info) return;
    
    auto it = std::find(infoObjects_.begin(), infoObjects_.end(), info);
    if (it != infoObjects_.end()) {
        infoObjects_.erase(it);
        updatePositions();
    }
}

void InfoStackManager::clear() {
    for (auto* info : infoObjects_) {
        delete info;
    }
    infoObjects_.clear();
}

void InfoStackManager::updatePositions() {
    float currentY = startY_;
    
    for (auto* info : infoObjects_) {
        if (!info) continue;
        
        info->setYPosition(currentY);
        currentY += info->getHeight() + spacing_;
    }
}

void InfoStackManager::updateAll() {
    for (auto* info : infoObjects_) {
        if (info) {
            info->update();
        }
    }
    updatePositions();
}

void InfoStackManager::renderAll(SDL_Renderer* renderer) {
    for (auto* info : infoObjects_) {
        if (info) {
            info->render(renderer);
        }
    }
}