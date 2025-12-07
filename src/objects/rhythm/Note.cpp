#include <objects/rhythm/Note.h>
#include <rhythm/Playfield.h>

SDL_Texture* Note::sharedTexture_ = nullptr;
SDL_Texture* Note::mineTexture_ = nullptr;

void Note::setAlphaMod(Uint8 alpha) {
    if (sharedTexture_) {
        SDL_SetTextureAlphaMod(sharedTexture_, alpha);
    }
}

void Note::LoadSharedTextures(SDL_Renderer* renderer, Playfield* pf) {
    SkinUtils* skinUtils = pf->getSkinUtils();

    if (sharedTexture_ == nullptr) {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/note");

        sharedTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedTexture_) {
            GAME_LOG_ERROR("Failed to load shared note texture from " + filePath);
        }
    }

    if (mineTexture_ == nullptr) {
        std::string mineFilePath = skinUtils->getFilePathForSkinElement("notes/mine");

        mineTexture_ = IMG_LoadTexture(renderer, mineFilePath.c_str());
        if (!mineTexture_) {
            GAME_LOG_ERROR("Failed to load mine note texture from " + mineFilePath);
            if (sharedTexture_) {
                mineTexture_ = sharedTexture_;
            }
        }
    }
}

void Note::DestroySharedTexture() {
    if (sharedTexture_ != nullptr) {
        SDL_DestroyTexture(sharedTexture_);
        sharedTexture_ = nullptr;
    }

    if (mineTexture_ != nullptr) {
        SDL_DestroyTexture(mineTexture_);
        mineTexture_ = nullptr;
    }
}

Note::Note(float x, float y, float width, float height, int column)
    : Strum(x, y, width, height), column_(column) {
        setRenderTexture(sharedTexture_, false);
    }

Note::~Note() {}

void Note::update(float deltaTime) {
    Playfield* playfield = getPlayfield();
    if (!playfield) return;

    Conductor* conductor = playfield->getConductor();
    if (!conductor || !conductor->isPlaying()) return;

    bool isHoldBeingHeld = false;
    if (getType() == HOLD_START) {
        HoldNote* holdNote = dynamic_cast<HoldNote*>(this);
        if (holdNote && holdNote->isHolding()) {
            isHoldBeingHeld = true;
        }
    }
    
    if (isHoldBeingHeld) {
        float noteX_, noteY_;
        getPosition(noteX_, noteY_);
        
        float strumLinePos = playfield->getStrumLinePos() + playfield->getStrumLineOffset();
        setPosition(noteX_, strumLinePos);
        canRender = true;
        return;
    }
    
    if (shouldDespawn()) {
        float noteX_, noteY_;
        getPosition(noteX_, noteY_);
        float strumLinePos = playfield->getStrumLinePos() + playfield->getStrumLineOffset();
        
        canRender = true;
        return;
    }

    if (!getUpdatePos()) return;

    float currentTime = conductor->getSongPosition();
    float noteTime = getTime() / 1000.0f;
    float timeDiff = noteTime - currentTime;

    float playbackRateCompensation = (currentTime < 0.0f) ? 1.0f : conductor->getPlaybackRate();

    float speed = (playfield->getScrollSpeed() * getSpeedModifier()) / playbackRateCompensation;
    float speedMult = playfield->getPlayfieldWidth() / 400.0f;
    
    speed *= speedMult;
    speed /= (playfield->getKeyCount() / 4.0f);

    float noteX_, noteY_, noteWidth_, noteHeight_;
    float playfieldX_, playfieldY_;

    playfield->getPosition(playfieldX_, playfieldY_);
    getPosition(noteX_, noteY_);
    getSize(noteWidth_, noteHeight_);

    float playfieldHeight = playfield->getPlayfieldHeight();
    float strumLinePos = playfield->getStrumLinePos() + playfield->getStrumLineOffset();
    float newY = strumLinePos - (timeDiff * speed);
    
    setPosition(noteX_, newY);

    if (newY < 0 - noteHeight_ || newY > playfieldY_ + playfieldHeight / 2) {
        canRender = false;
    } else {
        canRender = true;
    }
}

void Note::render(SDL_Renderer* renderer) {
    if (canRender && !despawned) {
        Strum::render(renderer);
    }
}