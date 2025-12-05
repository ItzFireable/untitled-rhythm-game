#ifndef RHYTHM_GAMEPLAYHUD_H
#define RHYTHM_GAMEPLAYHUD_H

#include <string>
#include <SDL3/SDL.h>
#include <objects/TextObject.h>

#include <utils/SkinUtils.h>
#include <utils/Variables.h>

#include <rhythm/Conductor.h>
#include <rhythm/JudgementSystem.h>

class GameplayHud {
public:
    GameplayHud(AppContext* appContext, SkinUtils* skinUtils);
    ~GameplayHud() = default;

    void update(float deltaTime);
    void render();

    void setConductor(Conductor* conductor) { conductor_ = conductor; }
    void setJudgementSystem(JudgementSystem* judgementSystem) { judgementSystem_ = judgementSystem; }
private:
    AppContext* appContext_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    float screenWidth_ = 1920.0f;
    float screenHeight_ = 1080.0f;

    Conductor* conductor_ = nullptr;
    SkinUtils* skinUtils_ = nullptr;
    JudgementSystem* judgementSystem_ = nullptr;

    TextObject* judgementInfo_ = nullptr;
    TextObject* accuracyInfo_ = nullptr;
};

#endif