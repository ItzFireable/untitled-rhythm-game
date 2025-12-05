#ifndef PLAY_STATE_H
#define PLAY_STATE_H

#include <BaseState.h>
#include <rhythm/ChartManager.h>
#include <rhythm/Conductor.h>
#include <rhythm/Playfield.h>
#include <utils/Utils.h>
#include <string>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <rhythm/JudgementSystem.h>
#include <objects/ConductorInfo.h>
#include <rhythm/GameplayHud.h>

class PlayState : public BaseState
{
public:
    void init(AppContext* appContext, void* payload) override;
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
    void render() override;
    void destroy() override;

    std::string getChartBackgroundName(ChartData chartData);
    std::string getStateName() const override {
        return "PlayState";
    }
private:
    bool isReady_ = false;
    float readyTimer_ = 0.0f;
    const float START_DELAY = 2.0f;

    std::unique_ptr<Conductor> conductor_;
    std::unique_ptr<SkinUtils> skinUtils_;

    ChartData chartData_;

    Playfield* playfield_ = nullptr;
    std::unique_ptr<JudgementSystem> judgementSystem_;
    
    SDL_Texture* backgroundTexture_;
    ConductorInfo* conductorInfo_ = nullptr;
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