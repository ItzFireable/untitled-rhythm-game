#ifndef PLAY_STATE_H
#define PLAY_STATE_H

#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <BaseState.h>
#include <utils/Utils.h>
#include <rhythm/Conductor.h>
#include <rhythm/Playfield.h>
#include <rhythm/GameplayHud.h>
#include <utils/SettingsManager.h>
#include <rhythm/JudgementSystem.h>
#include <utils/rhythm/ChartUtils.h>

class PlayState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload) override;
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    std::string getStateName() const override {
        return "PlayState";
    }
private:
    bool isReady_ = false;
    float readyTimer_ = 0.0f;
    const float START_DELAY = 2.0f;

    PlayStateData* currentStateData_ = nullptr;
    SongSelectData* previousStateData_ = nullptr;

    Conductor* conductor_;
    std::unique_ptr<SkinUtils> skinUtils_;

    ChartData chartData_;

    Playfield* playfield_ = nullptr;
    std::unique_ptr<JudgementSystem> judgementSystem_;
    
    SDL_Texture* backgroundTexture_;
    GameplayHud* gameplayHud_ = nullptr;

    float baseSize = 200.0f;
    float beatSize = baseSize;
    float beatLength = -1.0f;
    
    std::string songPath_;
    std::string audioPath_;

    float playbackRate_;
    bool isPaused_;
    
    void loadChart(const ChartData& data);
};

#endif