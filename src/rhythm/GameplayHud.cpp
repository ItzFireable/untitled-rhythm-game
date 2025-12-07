#include <rhythm/GameplayHud.h>

TextXAlignment convertHudAlignXToTextAlignX(HudAlignmentX hudAlign) {
    switch (hudAlign) {
        case HUD_ALIGN_LEFT:
            return ALIGN_LEFT;
        case HUD_ALIGN_CENTER:
            return ALIGN_CENTER;
        case HUD_ALIGN_RIGHT:
            return ALIGN_RIGHT;
        default:
            return ALIGN_LEFT;
    }
}

TextYAlignment convertHudAlignYToTextAlignY(HudAlignmentY hudAlign) {
    switch (hudAlign) {
        case HUD_ALIGN_TOP:
            return ALIGN_TOP;
        case HUD_ALIGN_MIDDLE:
            return ALIGN_MIDDLE;
        case HUD_ALIGN_BOTTOM:
            return ALIGN_BOTTOM;
        default:
            return ALIGN_TOP;
    }
}

TextAlignment convertHudAlignXToTextAlignment(HudAlignmentX hudAlign) {
    switch (hudAlign) {
        case HUD_ALIGN_LEFT:
            return TEXT_ALIGN_LEFT;
        case HUD_ALIGN_CENTER:
            return TEXT_ALIGN_CENTER;
        case HUD_ALIGN_RIGHT:
            return TEXT_ALIGN_RIGHT;
        default:
            return TEXT_ALIGN_LEFT;
    }
}

GameplayHud::GameplayHud(AppContext* appContext, SkinUtils* skinUtils, JudgementSystem* judgementSystem)
    : appContext_(appContext),
      renderer_(appContext->renderer),
      skinUtils_(skinUtils),
      judgementSystem_(judgementSystem)
{
    if (appContext_) {
        screenWidth_ = appContext_->renderWidth;
        screenHeight_ = appContext_->renderHeight;
    }

    std::string fontName = skinUtils_->getFontName();
    float fontSize = skinUtils_->getSkinProperty<float>("fontSize", 24.0f);

    if (skinUtils_->getSkinProperty<bool>("showJudgementCounter", true)) {
        GAME_LOG_DEBUG("GameplayHud: Initializing JudgementCounter");
        HudAlignmentX alignX = skinUtils_->getJudgementCounterXAlignment();
        HudAlignmentY alignY = skinUtils_->getJudgementCounterYAlignment();

        float posX = skinUtils_->getSkinProperty<float>("judgementCounterX", 16.0f);
        float posY = skinUtils_->getSkinProperty<float>("judgementCounterY", 0.0f);

        if (alignX == HUD_ALIGN_CENTER) {
            posX += screenWidth_ / 2.0f;
        } else if (alignX == HUD_ALIGN_RIGHT) {
            posX = screenWidth_ - posX;
        }

        if (alignY == HUD_ALIGN_MIDDLE) {
            posY += screenHeight_ / 2.0f;
        } else if (alignY == HUD_ALIGN_BOTTOM) {
            posY = screenHeight_ - posY;
        }

        judgementInfo_ = new TextObject(renderer_, fontName, fontSize);
        judgementInfo_->setPosition(posX, posY);
        judgementInfo_->setColor({255, 255, 255, 255});
        judgementInfo_->setTextGap(-8.0f);
        judgementInfo_->setAlignment(convertHudAlignXToTextAlignment(alignX));
        judgementInfo_->setXAlignment(convertHudAlignXToTextAlignX(alignX));
        judgementInfo_->setYAlignment(convertHudAlignYToTextAlignY(alignY));
    }

    if (skinUtils_->getSkinProperty<bool>("showAccuracyDisplay", true)) {
        GAME_LOG_DEBUG("GameplayHud: Initializing AccuracyDisplay");
        HudAlignmentX alignX = skinUtils_->getAccuracyXAlignment();
        HudAlignmentY alignY = skinUtils_->getAccuracyYAlignment();

        float posX = skinUtils_->getSkinProperty<float>("accuracyX", 0.0f);
        float posY = skinUtils_->getSkinProperty<float>("accuracyY", 16.0f);

        if (alignX == HUD_ALIGN_CENTER) {
            posX += screenWidth_ / 2.0f;
        } else if (alignX == HUD_ALIGN_RIGHT) {
            posX = screenWidth_ - posX;
        }

        if (alignY == HUD_ALIGN_MIDDLE) {
            posY += screenHeight_ / 2.0f;
        } else if (alignY == HUD_ALIGN_BOTTOM) {
            posY = screenHeight_ - posY;
        }

        accuracyInfo_ = new TextObject(renderer_, fontName, fontSize);
        accuracyInfo_->setPosition(posX, posY);
        accuracyInfo_->setColor({255, 255, 255, 255});
        accuracyInfo_->setAlignment(convertHudAlignXToTextAlignment(alignX));
        accuracyInfo_->setXAlignment(convertHudAlignXToTextAlignX(alignX));
        accuracyInfo_->setYAlignment(convertHudAlignYToTextAlignY(alignY));
    }

    bool showHitErrorBar = skinUtils_->getSkinProperty<bool>("showHitErrorBar", true);
    if (showHitErrorBar) {
        GAME_LOG_DEBUG("GameplayHud: Initializing HitErrorBar");
        hitErrorBar_ = new HitErrorBar(appContext_, skinUtils_, judgementSystem_);
    }
}

void GameplayHud::update(float deltaTime)
{
    if (conductor_)
    {
        conductor_->update(deltaTime);
    }

    if (judgementSystem_)
    {
        if (skinUtils_->getSkinProperty<bool>("showAccuracyDisplay", true) && accuracyInfo_)
        {
            std::stringstream acc;
            acc.precision(2);
            acc << std::fixed;
            acc << judgementSystem_->getAccuracy() << "%";

            accuracyInfo_->setText(acc.str());
        }

        if (skinUtils_->getSkinProperty<bool>("showJudgementCounter", true) && judgementInfo_)
        {
            std::stringstream js;
            js.precision(0);
            js << std::fixed;

            for (Judgement j : JudgementSystem::getAllJudgements()) {
                js << JudgementSystem::judgementToString(j) << ": " << judgementSystem_->getJudgementCount(j) << "\n";
            }
            judgementInfo_->setText(js.str());
        }
    }

    if (hitErrorBar_)
    {
        hitErrorBar_->update(deltaTime);
    }
}

void GameplayHud::render()
{
    if (judgementInfo_ && skinUtils_->getSkinProperty<bool>("showJudgementCounter", true)){ judgementInfo_->render(); }
    if (accuracyInfo_ && skinUtils_->getSkinProperty<bool>("showAccuracyDisplay", true)){ accuracyInfo_->render(); }
    if (hitErrorBar_ && skinUtils_->getSkinProperty<bool>("showHitErrorBar", true)){ hitErrorBar_->render(); }
}

void GameplayHud::destroy()
{
    if (hitErrorBar_) {
        hitErrorBar_->destroy();
        delete hitErrorBar_;
        hitErrorBar_ = nullptr;
    }

    if (judgementInfo_) {
        delete judgementInfo_;
        judgementInfo_ = nullptr;
    }

    if (accuracyInfo_) {
        delete accuracyInfo_;
        accuracyInfo_ = nullptr;
    }
}