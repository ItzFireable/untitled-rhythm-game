#ifndef PLAYFIELD_H
#define PLAYFIELD_H

#include <SDL3/SDL.h>
#include <objects/rhythm/Strum.h>
#include <objects/rhythm/Note.h>
#include <objects/rhythm/HoldNote.h>
#include <system/Logger.h>
#include <utils/rhythm/ChartUtils.h>
#include <rhythm/Conductor.h>
#include <utils/rhythm/SkinUtils.h>
#include <rhythm/JudgementSystem.h>
#include <utils/SettingsManager.h>
#include <system/Variables.h>
#include <rhythm/GameplayHud.h>
#include <vector>
#include <map>
#include <random>
#include <chrono>

class Playfield {
public:
    Playfield();
    ~Playfield();

    void init(AppContext* appContext, int keyCount, float height = 600.0f, SkinUtils* skinUtils = nullptr);
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    void destroy();

    void spawnJudgementEffect(const JudgementResult &judgement);
    void handleStrumInput(int keybind, bool isKeyDown);

    void handleKeyPress(int column);
    void handleKeyRelease(int column);

    void setScrollSpeed(float speed) {
        scrollSpeed_ = speed;
    }

    float getStrumXPosition(int index, int keyCount, float playfieldWidth);
    float getScrollSpeed() const {
        return scrollSpeed_;
    }

    void setPosition(float x, float y);
    void getPosition(float& x, float& y) const {
        x = positionX_;
        y = positionY_;
    }

    void setPadding(float left, float right) {
        paddingLeft_ = left;
        paddingRight_ = right;
    }
    void getPadding(float& left, float& right) const {
        left = paddingLeft_;
        right = paddingRight_;
    }

    float getPlayfieldHeight() const {
        return playfieldHeight_;
    }

    float getPlayfieldWidth() const {
        return playfieldWidth_;
    }

    void loadNotes(ChartData* chartData);
    void setConductor(Conductor* conductor);
    Conductor* getConductor() const {
        return conductor_;
    }

    int getRenderLimit() const { return renderLimit_; }
    float getKeyCount() const { return keyCount_; }
    SDL_Keycode getKeybind(int index) const {
        if (index < 0 || index >= static_cast<int>(keybinds_.size())) {
            return SDLK_UNKNOWN;
        }
        return keybinds_[index];
    }

    float getStrumLinePos() const { return strumLinePos_; }
    float getStrumLineOffset() const { return strumLineOffset_;}
    float getJudgementOffset() const { return judgementOffset_; }

    void setStrumLineOffset(float offset) { strumLineOffset_ = offset; }
    void setJudgementOffset(float offset) { judgementOffset_ = offset; }

    void loadSkin(const std::string& skinName) { skinUtils_->loadSkin(skinName); }
    const std::string& getSkinName() const { return skinUtils_->getSkinName(); }

    SkinUtils* getSkinUtils() const { return skinUtils_; }

    void setJudgementSystem(JudgementSystem* judgementSystem) { judgementSystem_ = judgementSystem; }
    JudgementSystem* getJudgementSystem() const { return judgementSystem_; }

    void setGameplayHud(GameplayHud* hud) { gameplayHud_ = hud; }
    GameplayHud* getGameplayHud() const { return gameplayHud_; }

    bool getAutoplay() const { return useAutoplay_; }
private:
    AppContext* appContext_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    Conductor* conductor_;
    JudgementSystem* judgementSystem_ = nullptr;

    SkinUtils* skinUtils_ = nullptr;
    GameplayHud *gameplayHud_ = nullptr;

    std::vector<SDL_Keycode> keybinds_;

    bool useAutoplay_ = true;
    std::map<int, bool> keysPressed_;

    int renderLimit_ = 100;
    float strumLinePos_ = 0.0f;
    float strumLineOffset_ = -32.0f;

    float judgementOffset_ = -128.0f;

    int keyCount_ = 4;
    float keySize_ = 40.0f;

    float scrollSpeed_ = 1650.0f;
    
    float playfieldWidth_ = 0.0f;
    float playfieldHeight_ = 0.0f;

    float positionX_ = 0.0f;
    float positionY_ = 0.0f;

    float paddingLeft_ = 16.0f;
    float paddingRight_ = 16.0f;
    
    std::vector<Strum*> strums_;
    std::vector<Note*> notes_;
    
    std::map<std::string, SDL_Texture*> judgementSpriteCache_;
    
    SDL_Texture* activeJudgementTexture_ = nullptr;
    float judgementDisplayTime_ = 0.0f;
    const float JUDGEMENT_FADE_DURATION = 0.8f;
    const float JUDGEMENT_FADE_START_RATIO = 0.5f;
    float judgementScale_ = 1.0f;

    float judgementTexW_ = 0.0f;
    float judgementTexH_ = 0.0f;

    std::mt19937 randomGenerator_;
    std::uniform_real_distribution<float> offsetDistribution_;
    std::unordered_map<int, float> autoplayReleaseTimes_;
};

#endif