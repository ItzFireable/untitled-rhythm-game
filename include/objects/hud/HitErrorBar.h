#ifndef HITERRORBAR_H
#define HITERRORBAR_H

#include <SDL3/SDL.h>
#include <vector>

#include <system/Variables.h>
#include <utils/rhythm/SkinUtils.h>
#include <rhythm/JudgementSystem.h>

class HitErrorBar
{
public:
    HitErrorBar(AppContext *appContext, SkinUtils *skinUtils, JudgementSystem *judgementSystem);
    ~HitErrorBar() = default;

    void update(float deltaTime);
    void render();
    void destroy();

    void addHitError(Judgement judgement, float offsetMs);
    void setBarSize(int width, int height);
    void setBarPosition(int x, int y);
private:
    AppContext *appContext_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;

    SkinUtils *skinUtils_ = nullptr;
    JudgementSystem *judgementSystem_ = nullptr;

    SDL_FRect barRect_ = {0, 0, 0, 0};
    std::vector<float> hitErrors_;
    std::vector<std::pair<SDL_FRect, Judgement>> errorData;
};

#endif