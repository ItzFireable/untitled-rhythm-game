#include <rhythm/Playfield.h>

Playfield::Playfield() {}
Playfield::~Playfield()
{
    Note::DestroySharedTexture();

    for (auto const &[key, val] : judgementSpriteCache_)
    {
        if (val)
        {
            SDL_DestroyTexture(val);
        }
    }
    judgementSpriteCache_.clear();
}

void Playfield::setConductor(Conductor *conductor)
{
    conductor_ = conductor;
}

void Playfield::spawnJudgementEffect(const std::string &judgementName)
{
    SDL_Texture *judgementTexture = nullptr;
    std::string filePath = skinUtils_->getFilePathForSkinElement("judgements/" + judgementName);

    auto it = judgementSpriteCache_.find(filePath);
    if (it != judgementSpriteCache_.end())
    {
        judgementTexture = it->second;
    }

    if (!judgementTexture)
    {
        judgementTexture = IMG_LoadTexture(renderer_, filePath.c_str());
        if (judgementTexture)
        {
            judgementSpriteCache_[filePath] = judgementTexture;
        }
        else
        {
            GAME_LOG_ERROR("Failed to load judgement texture from " + filePath);
            return;
        }
    }

    activeJudgementTexture_ = judgementTexture;
    judgementDisplayTime_ = 0.0f;
    judgementScale_ = 0.4f;

    SDL_GetTextureSize(activeJudgementTexture_, &judgementTexW_, &judgementTexH_);
}

float Playfield::getStrumXPosition(int index, int keyCount, float playfieldWidth)
{
    float paddingLeft_;
    float paddingRight_;

    Playfield::getPadding(paddingLeft_, paddingRight_);
    float gapSize = skinUtils_->getStrumlineGap();

    float totalReservedPadding = paddingLeft_ + paddingRight_;
    float availableWidth = playfieldWidth - totalReservedPadding;

    float totalGapWidth = gapSize * static_cast<float>(keyCount - 1);
    float keySpace = availableWidth - totalGapWidth;
    float keyWidth = keySpace / static_cast<float>(keyCount);

    float keyBlockWidth = (keyWidth * keyCount) + totalGapWidth;
    float emptySpace = playfieldWidth - keyBlockWidth;
    float centeringOffset = emptySpace / 2.0f;

    float playfieldLeftEdge = positionX_ - playfieldWidth / 2.0f;
    return playfieldLeftEdge + centeringOffset + (index * keyWidth) + (index * gapSize);
}

void Playfield::init(SDL_Renderer *renderer, int keyCount, float height, SkinUtils* skinUtils)
{
    renderer_ = renderer;
    keyCount_ = keyCount;
    skinUtils_ = skinUtils;

    float fullPlayfieldWidth = skinUtils_->getPlayfieldWidth();
    playfieldWidth_ = fullPlayfieldWidth;

    setPadding(skinUtils_->getPadding(), skinUtils_->getPadding());

    scrollSpeed_ = skinUtils_->getScrollSpeed();
    strumLineOffset_ = skinUtils_->getStrumlineOffset();
    judgementOffset_ = skinUtils_->getJudgementPopupOffset();

    playfieldHeight_ = height;
    float paddingLeft_, paddingRight_;
    getPadding(paddingLeft_, paddingRight_);
    float gapSize = skinUtils_->getStrumlineGap();
    
    float totalReservedPadding = paddingLeft_ + paddingRight_;
    float availableWidthForKeysAndGaps = playfieldWidth_ - totalReservedPadding;

    float totalGapWidth = gapSize * static_cast<float>(keyCount_ - 1);
    float keySpace = availableWidthForKeysAndGaps - totalGapWidth;
    keySize_ = keySpace / static_cast<float>(keyCount_);
    strumLinePos_ = positionY_ + playfieldHeight_ - keySize_;

    for (int i = 0; i < keyCount_; ++i)
    {
        float strumX = getStrumXPosition(i, keyCount_, playfieldWidth_);
        Strum *strum = new Strum(strumX, strumLinePos_ + strumLineOffset_, keySize_, keySize_, i);

        strum->setPlayfield(this);
        strum->loadTexture(renderer_);

        strums_.push_back(strum);
    }
}

void Playfield::loadNotes(ChartData *chartData)
{
    Note::LoadSharedTexture(renderer_, this);
    HoldNote::LoadSharedTextures(renderer_, this);

    std::map<int, HoldNote*> openHoldNotes;

    std::sort(chartData->notes.begin(), chartData->notes.end(),
        [](const NoteStruct &a, const NoteStruct &b)
        {
            if (a.time != b.time)
            {
                return a.time < b.time;
            }

            return a.type < b.type;
        });

    for (const auto &noteStruct : chartData->notes)
    {
        if (noteStruct.column < 0 || noteStruct.column >= keyCount_)
        {
            GAME_LOG_WARN("Note column " + std::to_string(noteStruct.column) + " is out of bounds for key count " + std::to_string(keyCount_) + ". Skipping note.");
            continue;
        }

        Note *note = nullptr;
        float strumX = getStrumXPosition(noteStruct.column, keyCount_, playfieldWidth_);

        if (noteStruct.type == HOLD_START)
        {
            note = new HoldNote(strumX, -keySize_, keySize_, keySize_, noteStruct.column);
            openHoldNotes[noteStruct.column] = static_cast<HoldNote*>(note);
        }
        else if (noteStruct.type == HOLD_END)
        {
            auto it = openHoldNotes.find(noteStruct.column);
            
            if (it != openHoldNotes.end()) {
                HoldNote* holdStartNote = it->second;
                holdStartNote->setEndTime(noteStruct.time);

                openHoldNotes.erase(it); 
                continue; 
            } else {
                GAME_LOG_WARN("No matching HOLD_START found for HOLD_END at time " + std::to_string(noteStruct.time) + "ms in column " + std::to_string(noteStruct.column) + ". Skipping HOLD_END data.");
                continue;
            }
        }
        else
        {
            note = new Note(strumX, -keySize_, keySize_, keySize_, noteStruct.column);
        }

        if (!note)
        {
            continue;
        }

        note->setTime(noteStruct.time);
        note->setType(noteStruct.type);
        note->setColumn(noteStruct.column);
        note->setSpeedModifier(1.0f);
        note->setPlayfield(this);

        notes_.push_back(note);
    }
}

void Playfield::setPosition(float x, float y)
{
    positionX_ = x;
    positionY_ = y;

    for (int i = 0; i < keyCount_; ++i)
    {
        float strumX = getStrumXPosition(i, keyCount_, playfieldWidth_);
        strums_[i]->setPosition(strumX, positionY_ + playfieldHeight_ / 2 - keySize_ + strumLineOffset_);
    }
}

void Playfield::handleKeyPress(int column) {
    if (column < 0 || column >= keyCount_) return;
    if (!conductor_ || !judgementSystem_) return;
    if (!conductor_->isPlaying()) return;
    if (keysPressed_.find(column) != keysPressed_.end() && keysPressed_[column]) return;
    
    keysPressed_[column] = true;
    float currentTime = conductor_->getSongPosition();
    float maxWindowMs = judgementSystem_->getMaxMissWindowMs();
    
    Note* closestNote = nullptr;
    float closestOffset = maxWindowMs + 1.0f;
    
    for (size_t i = 0; i < notes_.size(); ++i) {
        Note* note = notes_[i];
        
        if (!note || note->shouldDespawn()) continue;
        if (note->getColumn() != column) continue;
        if (note->hasBeenHit()) continue;
        
        if (note->getType() == HOLD_START) {
            HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
            if (!holdNote || holdNote->isHolding()) continue;
        }
        
        float noteTime = note->getTime() / 1000.0f;
        float timeDiffMs = (noteTime - currentTime) * 1000.0f;
        
        if (timeDiffMs > maxWindowMs) break;
        float absOffset = std::abs(timeDiffMs);
        
        if (absOffset <= maxWindowMs && absOffset < closestOffset) {
            closestOffset = absOffset;
            closestNote = note;
        }
    }

    if (!closestNote) return;

    float noteTime = closestNote->getTime() / 1000.0f;
    float timeDiffMs = (noteTime - currentTime) * 1000.0f;
    
    Judgement j = judgementSystem_->getJudgementForTimingOffset(timeDiffMs);
    if (j != Judgement::Miss) {
        closestNote->markAsHit();
        
        if (closestNote->getType() == HOLD_START) {
            HoldNote* holdNote = dynamic_cast<HoldNote*>(closestNote);
            if (holdNote) {
                holdNote->setIsHolding(true);
            }
        } else {
            closestNote->despawnNote();
        }
        
        judgementSystem_->addJudgement(j);
        spawnJudgementEffect(judgementSystem_->judgementToString(j));
    }
}

void Playfield::handleKeyRelease(int column) {
    if (column < 0 || column >= keyCount_) return;
    if (!conductor_ || !judgementSystem_) return;
    if (!conductor_->isPlaying()) return;
    if (keysPressed_.find(column) == keysPressed_.end() || !keysPressed_[column]) return;
    
    keysPressed_[column] = false;
    float currentTime = conductor_->getSongPosition();
    
    for (size_t i = 0; i < notes_.size(); ++i) {
        Note* note = notes_[i];
        
        if (!note || note->shouldDespawn()) continue;
        if (note->getColumn() != column) continue;
        if (note->getType() != HOLD_START) continue;
        
        HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
        if (!holdNote || !holdNote->isHolding()) continue;
        
        float endTime = holdNote->getEndTime() / 1000.0f;
        float timeDiffMs = (endTime - currentTime) * 1000.0f;
        
        Judgement j = judgementSystem_->getJudgementForTimingOffset(timeDiffMs, true);
        
        holdNote->setIsHolding(false);
        holdNote->despawnNote();
        
        judgementSystem_->addJudgement(j);
        spawnJudgementEffect(judgementSystem_->judgementToString(j));
        return;
    }
}

void Playfield::handleStrumInput(int keybind, bool isKeyDown)
{
    if (useAutoplay_)
        return;

    int strumIndex = -1;
    switch (keybind)
    {
    case KEYBIND_STRUM_LEFT:
        strumIndex = 0;
        break;
    case KEYBIND_STRUM_DOWN:
        strumIndex = 1;
        break;
    case KEYBIND_STRUM_UP:
        strumIndex = 2;
        break;
    case KEYBIND_STRUM_RIGHT:
        strumIndex = 3;
        break;
    default:
        return; // Not a strum keybind
    }

    if (strumIndex >= 0 && strumIndex < static_cast<int>(strums_.size()))
    {
        if (isKeyDown)
        {
            handleKeyPress(strumIndex);
        }
        else
        {
            handleKeyRelease(strumIndex);
        }
    }
}

void Playfield::update(float deltaTime)
{
    if (activeJudgementTexture_)
    {
        judgementDisplayTime_ += deltaTime;
        if (judgementDisplayTime_ >= JUDGEMENT_FADE_DURATION)
        {
            activeJudgementTexture_ = nullptr;
        }

        float timeRatio = judgementDisplayTime_ / JUDGEMENT_FADE_DURATION;
        const float SCALE_UP_TIME = 0.1f;
        if (timeRatio < SCALE_UP_TIME)
        {
            judgementScale_ = 0.4f + (timeRatio / SCALE_UP_TIME) * 0.1f;
        }
        else
        {
            judgementScale_ = 0.5f;
        }
    }

    if (!conductor_ || !judgementSystem_) return;
    
    float currentTime = conductor_->getSongPosition();
    float maxMissWindow = judgementSystem_->getMaxMissWindowMs() / 1000.0f;

    if (useAutoplay_) {
        for (size_t i = 0; i < notes_.size(); ++i) {
            Note* note = notes_[i];
            
            if (!note || note->shouldDespawn() || note->hasBeenHit()) continue;
            float noteTime = note->getTime() / 1000.0f;
            
            if (currentTime >= noteTime) {
                note->markAsHit();
                
                if (note->getType() == HOLD_START) {
                    HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
                    if (holdNote) {
                        holdNote->setIsHolding(true);
                    }
                } else {
                    note->despawnNote();
                }
                
                judgementSystem_->addJudgement(Judgement::Marvelous);
                spawnJudgementEffect(judgementSystem_->judgementToString(Judgement::Marvelous));
            }
        }
        
        for (size_t i = 0; i < notes_.size(); ++i) {
            Note* note = notes_[i];
            
            if (!note || note->shouldDespawn()) continue;
            if (note->getType() != HOLD_START) continue;
            
            HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
            if (!holdNote || !holdNote->isHolding()) continue;
            
            float endTime = holdNote->getEndTime() / 1000.0f;
            
            if (currentTime >= endTime) {
                holdNote->setIsHolding(false);
                holdNote->despawnNote();
                
                judgementSystem_->addJudgement(Judgement::Marvelous);
                spawnJudgementEffect(judgementSystem_->judgementToString(Judgement::Marvelous));
            }
        }
    }

    int notesToUpdate = std::min(static_cast<int>(notes_.size()), getRenderLimit());
    for (int i = 0; i < notesToUpdate; ++i)
    {
        if (i >= notes_.size()) break;
        if (notes_[i]) {
            notes_[i]->update(deltaTime);
        }
    }

    if (!useAutoplay_) {
        for (size_t i = 0; i < notes_.size(); ++i)
        {
            Note* note = notes_[i];
            if (!note || note->shouldDespawn()) continue;
            
            if (note->getType() == HOLD_START) {
                HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
                if (!holdNote) {
                    note->despawnNote();
                    continue;
                }
                
                if (holdNote->isHolding()) {
                    float endTime = holdNote->getEndTime() / 1000.0f;
                    if (currentTime > endTime + maxMissWindow) {
                        holdNote->setIsHolding(false);
                        judgementSystem_->addJudgement(Judgement::Miss);
                        spawnJudgementEffect(judgementSystem_->judgementToString(Judgement::Miss));
                        holdNote->despawnNote();
                    }
                } else if (!holdNote->hasBeenHit()) {
                    float startTime = holdNote->getTime() / 1000.0f;
                    if (currentTime > startTime + maxMissWindow) {
                        judgementSystem_->addJudgement(Judgement::Miss);
                        spawnJudgementEffect(judgementSystem_->judgementToString(Judgement::Miss));
                        holdNote->despawnNote();
                    }
                } else {
                    float endTime = holdNote->getEndTime() / 1000.0f;
                    if (currentTime > endTime + maxMissWindow) {
                        holdNote->despawnNote();
                    }
                }
            } else {
                if (!note->hasBeenHit()) {
                    float noteTime = note->getTime() / 1000.0f;
                    if (currentTime > noteTime + maxMissWindow) {
                        float timeDiff = (noteTime - currentTime) * 1000.0f;
                        Judgement j = judgementSystem_->getJudgementForTimingOffset(timeDiff);
                        judgementSystem_->addJudgement(j);
                        spawnJudgementEffect(judgementSystem_->judgementToString(j));
                        note->despawnNote();
                    }
                }
            }
        }
    }

    for (int i = notes_.size() - 1; i >= 0; --i)
{
    Note* note = notes_[i];
    if (!note) continue;
    
    bool canDelete = false;

    if (note->getType() == HOLD_START) {
        HoldNote* holdNote = dynamic_cast<HoldNote*>(note);
        
        if (holdNote && holdNote->isHolding()) {
            continue;
        }
        
        if (holdNote && holdNote->isFadingOut()) {
            canDelete = holdNote->shouldDespawn(); 
        } else {
            canDelete = note->shouldDespawn(); 
        }
    } else {
        canDelete = note->shouldDespawn();
    }
    
    if (canDelete) {
        delete notes_[i];
        notes_.erase(notes_.begin() + i);
    }
}
}

void Playfield::render(SDL_Renderer *renderer)
{
    SDL_FRect playfieldRect = {
        positionX_ - playfieldWidth_ / 2,
        positionY_ - playfieldHeight_ / 2,

        playfieldWidth_,
        playfieldHeight_};

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &playfieldRect);

    for (auto &strum : strums_)
    {
        strum->render(renderer);
    }
    
    int notesToRender = std::min(static_cast<int>(notes_.size()), getRenderLimit());
    for (int i = 0; i < notesToRender; ++i)
    {
        notes_[i]->render(renderer);
    }

    if (activeJudgementTexture_)
    {
        float alpha = 255.0f;
        float fadeStartTime = JUDGEMENT_FADE_DURATION * JUDGEMENT_FADE_START_RATIO;

        if (judgementDisplayTime_ > fadeStartTime)
        {
            float fadeTime = judgementDisplayTime_ - fadeStartTime;
            float fadeDuration = JUDGEMENT_FADE_DURATION - fadeStartTime;

            alpha = 255.0f * (1.0f - (fadeTime / fadeDuration));
            alpha = std::max(0.0f, alpha);
        }

        SDL_SetTextureAlphaMod(activeJudgementTexture_, (Uint8)alpha);

        float centerX = positionX_;
        float centerY = positionY_ + judgementOffset_;

        SDL_FRect judgementRect = {
            centerX - (judgementTexW_ * judgementScale_) / 2.0f,
            centerY - (judgementTexH_ * judgementScale_) / 2.0f,
            (float)judgementTexW_ * judgementScale_,
            (float)judgementTexH_ * judgementScale_};

        SDL_RenderTexture(renderer, activeJudgementTexture_, NULL, &judgementRect);
        SDL_SetTextureAlphaMod(activeJudgementTexture_, 255);
    }
}

void Playfield::destroy()
{
    for (auto& strum : strums_)
    {
        delete strum;
    }
    strums_.clear();

    for (auto& note : notes_)
    {
        delete note;
    }
    notes_.clear();

    Note::DestroySharedTexture();
    HoldNote::DestroySharedTexture();

    for (auto const &[key, val] : judgementSpriteCache_)
    {
        if (val)
        {
            SDL_DestroyTexture(val);
        }
    }
    judgementSpriteCache_.clear();
}