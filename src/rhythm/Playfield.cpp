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

void Playfield::spawnJudgementEffect(const JudgementResult &judgement)
{
    SDL_Texture *judgementTexture = nullptr;
    std::string filePath = skinUtils_->getFilePathForSkinElement("judgements/" + JudgementSystem::judgementToString(judgement.judgement));

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
    float gapSize = skinUtils_->getSkinProperty<float>("strumlineGap", 0.0f);

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

std::string numberToString(int number) {
    static const std::unordered_map<int, std::string> numberMap = {
        {0, "Zero"}, {1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"},
        {5, "Five"}, {6, "Six"}, {7, "Seven"}, {8, "Eight"}, {9, "Nine"}
    };
    
    auto it = numberMap.find(number);
    return (it != numberMap.end()) ? it->second : std::to_string(number);
}

void Playfield::init(AppContext *appContext, int keyCount, float height, SkinUtils* skinUtils)
{
    appContext_ = appContext;
    renderer_ = appContext->renderer;

    keyCount_ = keyCount;
    skinUtils_ = skinUtils;
    
    for (int i = 0; i < keyCount; i++) {
        std::string key = appContext_->settingsManager->getSetting<std::string>("KEYS=" + std::to_string(keyCount_) + ".key" + numberToString(i + 1), "");
        keybinds_.push_back(SDL_GetKeyFromName(key.c_str()));
    }

    float fullPlayfieldWidth = skinUtils_->getSkinProperty<float>("playfieldWidth", 550.0f);
    bool scalePlayfield = skinUtils_->getSkinProperty<bool>("scalePlayfield", true);

    if (scalePlayfield && keyCount_ != 4) {
       fullPlayfieldWidth *= (keyCount_ / 4.0f);
    }

    playfieldWidth_ = fullPlayfieldWidth;

    float padding = skinUtils_->getSkinProperty<float>("playfieldPadding", 16.0f);
    setPadding(padding, padding);

    useAutoplay_ = appContext_->settingsManager->getSetting<bool>("GAMEPLAY.autoplay", false);
    scrollSpeed_ = appContext_->settingsManager->getSetting<float>("GAMEPLAY.scrollSpeed", 1000.0f);
    
    strumLineOffset_ = skinUtils_->getSkinProperty<float>("strumlineOffset", 0.0f);
    judgementOffset_ = skinUtils_->getSkinProperty<float>("judgementPopupOffset", 0.0f);

    playfieldHeight_ = height;
    float paddingLeft_, paddingRight_;
    getPadding(paddingLeft_, paddingRight_);
    float gapSize = skinUtils_->getSkinProperty<float>("strumlineGap", 0.0f);
    
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
    Note::LoadSharedTextures(renderer_, this);
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
    
    JudgementResult j = judgementSystem_->getJudgementForTimingOffset(timeDiffMs);
    if (closestNote->getType() == MINE) {
        j.judgement = Judgement::Miss;
    }

    if (j.judgement != Judgement::Miss || closestNote->getType() == MINE) {
        closestNote->markAsHit();
        
        if (closestNote->getType() == HOLD_START) {
            HoldNote* holdNote = dynamic_cast<HoldNote*>(closestNote);
            if (holdNote) {
                holdNote->setIsHolding(true);
            }
        } else {
            closestNote->despawnNote();
        }
        
        judgementSystem_->addJudgement(j.judgement, timeDiffMs);
        if (gameplayHud_ && gameplayHud_->getHitErrorBar()) {
            gameplayHud_->getHitErrorBar()->addHitError(j.judgement, timeDiffMs);
        }

        spawnJudgementEffect(j);
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
        
        JudgementResult j = judgementSystem_->getJudgementForTimingOffset(timeDiffMs, true);
        
        holdNote->setIsHolding(false);
        holdNote->despawnNote();

        judgementSystem_->addJudgement(j.judgement, timeDiffMs);
        if (gameplayHud_ && gameplayHud_->getHitErrorBar()) {
            gameplayHud_->getHitErrorBar()->addHitError(j.judgement, timeDiffMs);
        }

        spawnJudgementEffect(j);
        return;
    }
}

void Playfield::handleStrumInput(int keybind, bool isKeyDown)
{
    if (useAutoplay_)
        return;

    int strumIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (keybind == getKeybind(i)) {
            strumIndex = i;
            break;
        }
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
            if (note->getType() == MINE) continue;

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
                
                judgementSystem_->addJudgement(Judgement::Marvelous, 0.0f);
                if (gameplayHud_ && gameplayHud_->getHitErrorBar()) {
                    gameplayHud_->getHitErrorBar()->addHitError(Judgement::Marvelous, 0.0f);
                }
                spawnJudgementEffect({ Judgement::Marvelous, 0.0f });
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
                
                judgementSystem_->addJudgement(Judgement::Marvelous, 0.0f);
                if (gameplayHud_ && gameplayHud_->getHitErrorBar()) {
                    gameplayHud_->getHitErrorBar()->addHitError(Judgement::Marvelous, 0.0f);
                }
                spawnJudgementEffect({ Judgement::Marvelous, 0.0f });
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

                if (holdNote->isFadingOut()) continue;
                
                if (holdNote->isHolding()) {
                    float endTime = holdNote->getEndTime() / 1000.0f;
                    if (currentTime > endTime + maxMissWindow) {
                        holdNote->setIsHolding(false);
                        holdNote->startFadeOut();

                        judgementSystem_->addJudgement(Judgement::Miss, maxMissWindow * 1000.0f + 1.0f);
                        spawnJudgementEffect({ Judgement::Miss, maxMissWindow * 1000.0f + 1.0f });
                    }
                } else if (!holdNote->hasBeenHit()) {
                    float startTime = holdNote->getTime() / 1000.0f;
                    if (currentTime > startTime + maxMissWindow) {
                        holdNote->startFadeOut();

                        judgementSystem_->addJudgement(Judgement::Miss, maxMissWindow * 1000.0f + 1.0f);
                        spawnJudgementEffect({ Judgement::Miss, maxMissWindow * 1000.0f + 1.0f });
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
                        if (note->getType() != MINE) {
                            judgementSystem_->addJudgement(Judgement::Miss, timeDiff);
                            spawnJudgementEffect({ Judgement::Miss, timeDiff });
                            note->despawnNote();
                        }
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
    if (!renderer)
        return;

    if (!appContext_ || !skinUtils_) {
        return;
    }

    if (skinUtils_->getSkinProperty<bool>("playfieldBackgroundEnabled", true) && skinUtils_->getSkinProperty<float>("playfieldBackgroundOpacity", 0.5f) > 0.0f)
    {
        SDL_FRect playfieldRect = {
            positionX_ - playfieldWidth_ / 2,
            positionY_ - playfieldHeight_ / 2,

            playfieldWidth_,
            playfieldHeight_};

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255 * skinUtils_->getSkinProperty<float>("playfieldBackgroundOpacity", 0.5f));
        SDL_RenderFillRect(renderer, &playfieldRect);
    }

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