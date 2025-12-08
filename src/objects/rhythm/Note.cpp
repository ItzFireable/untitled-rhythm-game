#include <objects/rhythm/Note.h>
#include <rhythm/Playfield.h>

SDL_Texture* Note::sharedNoteTexture_ = nullptr;
SDL_Texture* Note::sharedMineTexture_ = nullptr;

void Note::setAlphaMod(Uint8 alpha) {
    if (type == MINE && textures_.count("mine") && textures_["mine"]) {
        SDL_SetTextureAlphaMod(textures_["mine"], alpha);
    } else if (textures_.count("note") && textures_["note"]) {
        SDL_SetTextureAlphaMod(textures_["note"], alpha);
    }
}

void Note::loadTextures(SDL_Renderer* renderer, Playfield* pf) {
    SkinUtils* skinUtils = pf->getSkinUtils();

    if (sharedNoteTexture_ == nullptr) {
        std::string filePath = skinUtils->getFilePathForSkinElement("notes/note");
        sharedNoteTexture_ = IMG_LoadTexture(renderer, filePath.c_str());
        if (!sharedNoteTexture_) {
            GAME_LOG_ERROR("Failed to load shared note texture from " + filePath);
        }
    }

    if (sharedMineTexture_ == nullptr) {
        std::string mineFilePath = skinUtils->getFilePathForSkinElement("notes/mine");
        sharedMineTexture_ = IMG_LoadTexture(renderer, mineFilePath.c_str());
        if (!sharedMineTexture_) {
            GAME_LOG_ERROR("Failed to load mine note texture from " + mineFilePath);
            if (sharedNoteTexture_) {
                sharedMineTexture_ = sharedNoteTexture_;
            }
        }
    }
    
    textures_["note"] = sharedNoteTexture_;
    ownedTextures_[sharedNoteTexture_] = false;
    
    textures_["mine"] = sharedMineTexture_;
    ownedTextures_[sharedMineTexture_] = false;
}

void Note::destroyTextures() {
    if (sharedNoteTexture_ != nullptr) {
        SDL_DestroyTexture(sharedNoteTexture_);
        sharedNoteTexture_ = nullptr;
    }

    if (sharedMineTexture_ != nullptr && sharedMineTexture_ != sharedNoteTexture_) {
        SDL_DestroyTexture(sharedMineTexture_);
        sharedMineTexture_ = nullptr;
    }
}

Note::Note(float x, float y, float width, float height, int column)
    : Strum(x, y, width, height), column_(column) 
{
    textures_["note"] = sharedNoteTexture_;
    ownedTextures_[sharedNoteTexture_] = false;
    
    textures_["mine"] = sharedMineTexture_;
    ownedTextures_[sharedMineTexture_] = false;
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
    if (!canRender || despawned) 
        return;
    
    std::string textureKey = (type == MINE) ? "mine" : "note";
    SDL_Texture* noteTexture = textures_.count(textureKey) ? textures_[textureKey] : nullptr;
    
    float noteX, noteY, noteWidth, noteHeight;
    getPosition(noteX, noteY);
    getSize(noteWidth, noteHeight);
    
    float paddingTop, paddingBottom, paddingLeft, paddingRight;
    getPadding(paddingTop, paddingBottom, paddingLeft, paddingRight);
    
    SDL_FRect noteRect = {
        noteX + paddingLeft,
        noteY + paddingTop,
        noteWidth - paddingLeft - paddingRight,
        noteHeight - paddingTop - paddingBottom
    };
    
    if (noteTexture) {
        SDL_RenderTexture(renderer, noteTexture, NULL, &noteRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderRect(renderer, &noteRect);
    }
}