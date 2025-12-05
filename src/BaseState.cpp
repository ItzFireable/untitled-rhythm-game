#include "BaseState.h"
#include <iostream>

#include <SDL3/SDL.h>

BaseState::~BaseState() {}; 

void BaseState::handleEvent(const SDL_Event& e) {}
void BaseState::update(float deltaTime) {}
void BaseState::render() {}
void BaseState::destroy() {}
void BaseState::postBuffer() {}