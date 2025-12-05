#ifndef PLAYFIELD_H
#define PLAYFIELD_H

#include <SDL3/SDL.h>
#include <rhythm/Strum.h>
#include <rhythm/Note.h>
#include <rhythm/HoldNote.h>
#include <utils/Logger.h>
#include <rhythm/ChartManager.h>
#include <rhythm/Conductor.h>
#include <utils/SkinUtils.h>
#include <rhythm/JudgementSystem.h>
#include <utils/Variables.h>
#include <vector>
#include <map>

class Playfield {
public:
    Playfield();
    ~Playfield();

    void init(SDL_Renderer* renderer, int keyCount, float height = 600.0f, SkinUtils* skinUtils = nullptr);
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

    void loadNotes(ChartData* chartData);
    void setAppContext(AppContext* appContext) {
        appContext_ = appContext;
    }
    void setConductor(Conductor* conductor);
    Conductor* getConductor() const {
        return conductor_;
    }

    int getRenderLimit() const { return renderLimit_; }
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

    bool getAutoplay() const { return useAutoplay_; }
private:
    AppContext* appContext_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    Conductor* conductor_;
    JudgementSystem* judgementSystem_ = nullptr;

    SkinUtils* skinUtils_ = nullptr;

    bool useAutoplay_ = false;
    std::map<int, bool> keysPressed_;

    int renderLimit_ = 250;
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
};

#endif