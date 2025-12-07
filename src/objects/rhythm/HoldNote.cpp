#include <objects/rhythm/HoldNote.h>
#include <rhythm/Playfield.h>

SDL_Texture *HoldNote::sharedTexture_ = nullptr;
SDL_Texture *HoldNote::sharedBodyTexture_ = nullptr;
SDL_Texture *HoldNote::sharedEndTexture_ = nullptr;

void HoldNote::LoadSharedTextures(SDL_Renderer *renderer, Playfield *pf)
{
    SkinUtils *skinUtils = pf->getSkinUtils();

    if (sharedTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdStart");

        sharedTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold texture from " + filePath);
        }
    }

    if (sharedBodyTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdBody");

        sharedBodyTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedBodyTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold body texture from " + filePath);
        }
    }

    if (sharedEndTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdEnd");

        sharedEndTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedEndTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold end texture from " + filePath);
        }
    }
}

void HoldNote::DestroySharedTexture()
{
    if (sharedTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedTexture_);
        sharedTexture_ = nullptr;
    }

    if (sharedBodyTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedBodyTexture_);
        sharedBodyTexture_ = nullptr;
    }

    if (sharedEndTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedEndTexture_);
        sharedEndTexture_ = nullptr;
    }
}

HoldNote::HoldNote(float x, float y, float width, float height, int column)
    : Note(x, y, width, height), column_(column)
{
    setRenderTexture(sharedTexture_, false);
}

HoldNote::~HoldNote() {}

bool HoldNote::shouldDespawn() const
{
    if (isFadingOut()) {
        Playfield* playfield = getPlayfield();
        if (!playfield) return false;

        float noteWidth_, noteHeight_;
        getSize(noteWidth_, noteHeight_);

        float posX_, playfieldPosY_;
        playfield->getPosition(posX_, playfieldPosY_);

        float holdEndY = getEndY() + noteHeight_;
        float playfieldHeight = playfield->getPlayfieldHeight();
        float playfieldBottom = playfieldPosY_ + playfieldHeight / 2;

        if (holdEndY > playfieldBottom) {
            return true;
        }
        return false;
    }

    return Note::shouldDespawn();
}

void HoldNote::update(float deltaTime)
{
    Note::update(deltaTime);

    if (!hasEndTime())
        return;

    if (holdEndHeight_ < 0.0f && sharedEndTexture_ != nullptr)
    {
        float w, h;
        SDL_GetTextureSize(sharedEndTexture_, &w, &h);
        setHoldEndHeight(h);
    }

    Playfield *playfield = getPlayfield();
    if (!playfield)
        return;

    Conductor *conductor = playfield->getConductor();
    if (!conductor || !conductor->isPlaying())
        return;

    float currentTime = conductor->getSongPosition();
    float endNoteTime = getEndTime() / 1000.0f;
    float timeDiff = endNoteTime - currentTime;

    float playbackRateCompensation = (currentTime < 0.0f) ? 1.0f : conductor->getPlaybackRate();

    float speed = (playfield->getScrollSpeed() * getSpeedModifier()) / playbackRateCompensation;
    float speedMult = playfield->getPlayfieldWidth() / 400.0f;
    
    speed *= speedMult;
    speed /= (playfield->getKeyCount() / 4.0f);

    float strumLinePos = playfield->getStrumLinePos() + playfield->getStrumLineOffset();
    float newEndY = strumLinePos - (timeDiff * speed);

    setEndY(newEndY);
}

void HoldNote::render(SDL_Renderer *renderer)
{
    if (!canRenderNote() || shouldDespawn())
        return;
        
    if (!isHolding() && hasBeenHit())
    {
        return;
    }

    Playfield *playfield = getPlayfield();
    if (!playfield)
        return;

    SkinUtils *skinUtils = playfield->getSkinUtils();
    if (!skinUtils)
        return;

    float noteX_, noteY_, noteWidth_, noteHeight_;
    getPosition(noteX_, noteY_);
    getSize(noteWidth_, noteHeight_);

    float strumLinePos = playfield->getStrumLinePos() + playfield->getStrumLineOffset();
    float strumLineCenterY = strumLinePos + noteHeight_ / 2;

    Uint8 alphaMod = 255;
    const float FADE_DISTANCE = noteHeight_ * 2.0f;

    if (isFadingOut())
    {
        alphaMod = 128;

        SDL_SetTextureAlphaMod(sharedEndTexture_, alphaMod);
        SDL_SetTextureAlphaMod(sharedBodyTexture_, alphaMod);
    }

    float finalEndY = getEndY();
    float holdCut = skinUtils->getSkinProperty<float>("holdCut", 0.0f);

    finalEndY += holdCut;

    float bodyStartY = noteY_ + noteHeight_ / 2;
    float renderStartY;

    if (!isHolding() && hasBeenHit())
    {
        renderStartY = strumLineCenterY;
    }
    else
    {
        renderStartY = bodyStartY;
    }

    float topY = std::min(renderStartY, finalEndY);
    float bottomY = std::max(renderStartY, finalEndY);

    if (!isHolding() && hasBeenHit())
    {
        bottomY = std::min(bottomY, strumLineCenterY);
    }

    float bodyHeight = bottomY - topY;
    float fullHeight = bodyHeight;
    float endHeight = getHoldEndHeight();

    float bodyHeightMinusEnd = bodyHeight - endHeight;

    if (fullHeight > 1.0f)
    {
        float actualEndHeight = std::min(endHeight, bodyHeight);
        float bodyTextureHeight = bodyHeight - actualEndHeight;

        SDL_FRect endRect = {
            noteX_,
            topY,
            noteWidth_,
            actualEndHeight};

        SDL_FRect holdRect = {
            noteX_,
            topY + actualEndHeight,
            noteWidth_,
            bodyTextureHeight};

        if (sharedBodyTexture_ && sharedEndTexture_)
        {
            if (bodyTextureHeight > 0.0f)
            {
                SDL_RenderTexture(renderer, sharedBodyTexture_, nullptr, &holdRect);
            }
            SDL_RenderTexture(renderer, sharedEndTexture_, nullptr, &endRect);
        }
        else
        {
            holdRect.y = topY;
            holdRect.h = fullHeight;

            SDL_SetRenderDrawColor(renderer, 200, 200, 255, 150);
            SDL_RenderFillRect(renderer, &holdRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
    }

    SDL_SetTextureAlphaMod(sharedEndTexture_, 255);
    SDL_SetTextureAlphaMod(sharedBodyTexture_, 255);

    if (isFadingOut())
    {
        setAlphaMod(alphaMod);
    }
    Note::render(renderer);
    if (isFadingOut())
    {
        setAlphaMod(255);
    }
}