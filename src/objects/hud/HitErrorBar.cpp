#include "objects/hud/HitErrorBar.h"
int maxErrors = 50;

HitErrorBar::HitErrorBar(AppContext *appContext, SkinUtils *skinUtils, JudgementSystem *judgementSystem)
    : appContext_(appContext), skinUtils_(skinUtils), judgementSystem_(judgementSystem)
{
    renderer_ = appContext_->renderer;
}

void HitErrorBar::update(float deltaTime)
{
    while (hitErrors_.size() > maxErrors)
    {
        hitErrors_.erase(hitErrors_.begin());
        errorData.erase(errorData.begin());
    }

    if (!skinUtils_)
        return;

    maxErrors = skinUtils_->getSkinProperty<int>("hitErrorBarMaxErrors", 50);
    
    float barX = skinUtils_->getSkinProperty<float>("hitErrorBarX", 0.0f);
    float barY = skinUtils_->getSkinProperty<float>("hitErrorBarY", 10.0f);
    float barWidth = skinUtils_->getSkinProperty<float>("hitErrorBarWidth", 200.0f);
    float barHeight = skinUtils_->getSkinProperty<float>("hitErrorBarHeight", 20.0f);

    HudAlignmentX alignX = skinUtils_->getHudAlignmentX("hitErrorBarAlignX", HudAlignmentX::HUD_ALIGN_CENTER);
    HudAlignmentY alignY = skinUtils_->getHudAlignmentY("hitErrorBarAlignY", HudAlignmentY::HUD_ALIGN_BOTTOM);

    if (alignX == HUD_ALIGN_CENTER)
    {
        barX = (appContext_->renderWidth / 2.0f) + barX - (barWidth / 2.0f);
    }
    else if (alignX == HUD_ALIGN_RIGHT)
    {
        barX = appContext_->renderWidth - barWidth - barX;
    }

    if (alignY == HUD_ALIGN_MIDDLE)
    {
        barY = (appContext_->renderHeight / 2.0f) + barY - (barHeight / 2.0f);
    }
    else if (alignY == HUD_ALIGN_BOTTOM)
    {
        barY = appContext_->renderHeight - barHeight - barY;
    }

    if (barRect_.x == barX && barRect_.y == barY &&
        barRect_.w == barWidth && barRect_.h == barHeight)
    {
        return;
    }

    barRect_ = {barX, barY, barWidth, barHeight};
}

void HitErrorBar::render()
{
    if (!skinUtils_ || !judgementSystem_ || !renderer_)
        return;

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255 * skinUtils_->getSkinProperty<float>("hitErrorBarBackgroundOpacity", 0.5f));
    SDL_RenderFillRect(renderer_, &barRect_);

    int errorIndex = 0;
    for (const auto& [rect, judge] : errorData)
    {
        SDL_Color color = JudgementSystem::judgementToColor(judge);
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, 255 - (errorIndex * (255 / maxErrors)));
        SDL_RenderFillRect(renderer_, &rect);

        errorIndex++;
    }
}

void HitErrorBar::addHitError(Judgement judgement, float error)
{
    hitErrors_.push_back(error);

    float maxHitErrorMs_ = judgementSystem_->getMaxMissWindowMs();
    if (error < -maxHitErrorMs_ || error > maxHitErrorMs_)
    {
        return;
    }

    float normalizedError = (error + maxHitErrorMs_) / (2 * maxHitErrorMs_);
    SDL_FRect errorRect;

    errorRect.w = skinUtils_->getSkinProperty<float>("hitErrorBarErrorWidth", 4.0f);
    errorRect.h = barRect_.h;
    errorRect.x = barRect_.x + normalizedError * barRect_.w - (errorRect.w / 2.0f);
    errorRect.y = barRect_.y;

    errorData.push_back({errorRect, judgement});
}

void HitErrorBar::destroy()
{
    hitErrors_.clear();
    errorData.clear();

    barRect_ = {0, 0, 0, 0};
}