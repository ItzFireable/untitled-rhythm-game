#include <states/ResultsState.h>

#include <objects/TextObject.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>

float getJudgementCount(const std::map<Judgement, int>& counts, Judgement j) {
    auto it = counts.find(j);
    return (it != counts.end()) ? it->second : 0;
}

void ResultsState::init(AppContext* appContext, void* payload)
{
    BaseState::init(appContext, payload);
    ResultsData* resultsData = static_cast<ResultsData*>(payload);
    PlayStateData* playData = resultsData ? resultsData->playStateData : nullptr;
    SongSelectData* previousData = playData ? playData->previousStateData : nullptr;

    if (!resultsData || !playData || !previousData) {
        GAME_LOG_ERROR("ResultsState initialized without ResultsData payload.");
        requestStateSwitch(STATE_SONG_SELECT, nullptr);
    }

    currentStateData_ = resultsData;

    std::string bgName = ChartUtils::getChartBackgroundName(playData->chartData);
    if (!bgName.empty())
    {
        std::string bgPath = playData->chartData.filePath + "/" + bgName;
        backgroundTexture_ = IMG_LoadTexture(renderer, bgPath.c_str());

        if (!backgroundTexture_)
        {
            GAME_LOG_ERROR("PlayState: Failed to load background image: " + bgPath);
            backgroundTexture_ = nullptr;
        }
        else
        {
            SDL_SetTextureColorMod(backgroundTexture_, 30, 30, 30);
        }
    }

    this->chartInfoText_ = new TextObject(renderer, MAIN_FONT_PATH, 16);
    this->chartInfoText_->setAlignment(TEXT_ALIGN_CENTER);
    this->chartInfoText_->setXAlignment(ALIGN_CENTER);
    this->chartInfoText_->setYAlignment(ALIGN_TOP);
    this->chartInfoText_->setPosition(screenWidth_ / 2.0f, 16.0f);
    this->chartInfoText_->setColor({255, 255, 255, 255});
    this->chartInfoText_->setText(ChartUtils::getChartInfo(playData->chartData, playData->playbackRate));
    
    this->accuracyText_ = new TextObject(renderer, MAIN_FONT_PATH, 24);
    this->accuracyText_->setAlignment(TEXT_ALIGN_CENTER);
    this->accuracyText_->setXAlignment(ALIGN_CENTER);
    this->accuracyText_->setYAlignment(ALIGN_MIDDLE);
    this->accuracyText_->setPosition(screenWidth_ / 2.0f, screenHeight_ / 2.0f);
    this->accuracyText_->setColor({255, 255, 255, 255});

    std::stringstream dataStream;
    dataStream << std::fixed << std::setprecision(2) << "Accuracy: " << (resultsData->accuracy) << "% (osu!mania v1)\n\n";

    dataStream.precision(0);
    for (Judgement j : JudgementSystem::getAllJudgements()) {
        dataStream << JudgementSystem::judgementToString(j) << ": " << getJudgementCount(resultsData->judgementCounts, j) << "\n";
    }

    this->accuracyText_->setText(dataStream.str());

    this->returnText_ = new TextObject(renderer, MAIN_FONT_PATH, 16);
    this->returnText_->setAlignment(TEXT_ALIGN_CENTER);
    this->returnText_->setXAlignment(ALIGN_CENTER);
    this->returnText_->setYAlignment(ALIGN_BOTTOM);
    this->returnText_->setPosition(screenWidth_ / 2.0f, screenHeight_ - 16.0f);
    this->returnText_->setColor({255, 255, 255, 255});
    this->returnText_->setText("Press Enter to return to song select");
}

void ResultsState::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_RETURN:
                SongSelectData* previousData = nullptr;
                if (currentStateData_) {
                    PlayStateData* playData = currentStateData_->playStateData;
                    if (playData) {
                        previousData = playData->previousStateData;
                    }
                }
                requestStateSwitch(STATE_SONG_SELECT, previousData);
                break;
        }
    }
}

void ResultsState::update(float deltaTime)
{
    // No update logic needed for now
}

void ResultsState::render()
{
    if (backgroundTexture_)
    {
        SDL_RenderTexture(renderer, backgroundTexture_, nullptr, nullptr);
    }
    
    if (this->accuracyText_) { this->accuracyText_->render(); }
    if (this->chartInfoText_) { this->chartInfoText_->render(); }
    if (this->returnText_) { this->returnText_->render(); }
}

void ResultsState::destroy()
{
    if (backgroundTexture_) 
    {
        SDL_DestroyTexture(backgroundTexture_);
        backgroundTexture_ = nullptr;
    }

    if (this->accuracyText_)
    {
        delete this->accuracyText_;
        this->accuracyText_ = nullptr;
    }

    if (this->chartInfoText_)
    {
        delete this->chartInfoText_;
        this->chartInfoText_ = nullptr;
    }

    if (this->returnText_)
    {
        delete this->returnText_;
        this->returnText_ = nullptr;
    }
}