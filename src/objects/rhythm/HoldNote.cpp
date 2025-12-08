#include <objects/rhythm/HoldNote.h>
#include <rhythm/Playfield.h>

SDL_Texture* HoldNote::sharedHoldStartTexture_ = nullptr;
SDL_Texture* HoldNote::sharedHoldBodyTexture_ = nullptr;
SDL_Texture* HoldNote::sharedHoldEndTexture_ = nullptr;

void HoldNote::loadTextures(SDL_Renderer *renderer, Playfield *pf)
{
    SkinUtils *skinUtils = pf->getSkinUtils();

    if (sharedHoldStartTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdStart");
        sharedHoldStartTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedHoldStartTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold texture from " + filePath);
        }
    }

    if (sharedHoldBodyTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdBody");
        sharedHoldBodyTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedHoldBodyTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold body texture from " + filePath);
        }
    }

    if (sharedHoldEndTexture_ == nullptr)
    {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/holdEnd");
        sharedHoldEndTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedHoldEndTexture_)
        {
            GAME_LOG_ERROR("Failed to load shared hold end texture from " + filePath);
        }
    }
    
    textures_["note"] = sharedHoldStartTexture_;
    ownedTextures_[sharedHoldStartTexture_] = false;
    
    textures_["holdBody"] = sharedHoldBodyTexture_;
    ownedTextures_[sharedHoldBodyTexture_] = false;
    
    textures_["holdEnd"] = sharedHoldEndTexture_;
    ownedTextures_[sharedHoldEndTexture_] = false;
}

void HoldNote::destroyTextures()
{
    if (sharedHoldStartTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedHoldStartTexture_);
        sharedHoldStartTexture_ = nullptr;
    }

    if (sharedHoldBodyTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedHoldBodyTexture_);
        sharedHoldBodyTexture_ = nullptr;
    }

    if (sharedHoldEndTexture_ != nullptr)
    {
        SDL_DestroyTexture(sharedHoldEndTexture_);
        sharedHoldEndTexture_ = nullptr;
    }
}

HoldNote::HoldNote(float x, float y, float width, float height, int column)
    : Note(x, y, width, height), column_(column)
{
    textures_["note"] = sharedHoldStartTexture_;
    ownedTextures_[sharedHoldStartTexture_] = false;
    
    textures_["holdBody"] = sharedHoldBodyTexture_;
    ownedTextures_[sharedHoldBodyTexture_] = false;
    
    textures_["holdEnd"] = sharedHoldEndTexture_;
    ownedTextures_[sharedHoldEndTexture_] = false;
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

    if (holdEndHeight_ < 0.0f && textures_["holdEnd"] != nullptr)
    {
        float w, h;
        SDL_GetTextureSize(textures_["holdEnd"], &w, &h);
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

        if (textures_.count("holdEnd") && textures_["holdEnd"])
            SDL_SetTextureAlphaMod(textures_["holdEnd"], alphaMod);
        if (textures_.count("holdBody") && textures_["holdBody"])
            SDL_SetTextureAlphaMod(textures_["holdBody"], alphaMod);
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

    if (fullHeight > 1.0f && endTime_ != -1.0f)
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

        SDL_Texture* bodyTex = textures_.count("holdBody") ? textures_["holdBody"] : nullptr;
        SDL_Texture* endTex = textures_.count("holdEnd") ? textures_["holdEnd"] : nullptr;

        if (bodyTex && endTex)
        {
            if (bodyTextureHeight > 0.0f)
            {
                SDL_RenderTexture(renderer, bodyTex, nullptr, &holdRect);
            }
            SDL_RenderTexture(renderer, endTex, nullptr, &endRect);
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

    if (textures_.count("holdEnd") && textures_["holdEnd"])
        SDL_SetTextureAlphaMod(textures_["holdEnd"], 255);
    if (textures_.count("holdBody") && textures_["holdBody"])
        SDL_SetTextureAlphaMod(textures_["holdBody"], 255);

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