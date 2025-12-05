#include <states/PlayState.h>
#include <utils/Logger.h>
#include <SDL3/SDL.h>
#include <iostream>

const float bpmThreshold = 200.0f;

std::string PlayState::getChartBackgroundName(ChartData chartData) {
    auto it = chartData.metadata.find("background");
    if (it != chartData.metadata.end()) {
        return it->second;
    }
    return "";
}

void PlayState::init(AppContext *appContext, void *payload)
{
    BaseState::init(appContext, payload);
    PlayStateData *data = static_cast<PlayStateData *>(payload);

    if (!data)
    {
        GAME_LOG_ERROR("PlayState: No payload data provided");
        return;
    }

    chartData_ = data->chartData;
    songPath_ = data->songPath;
    audioPath_ = Utils::getAudioPath(chartData_);
    playbackRate_ = data->playbackRate;
    isPaused_ = false;

    float transitionTime = appContext->transitionDuration;
    readyTimer_ = -(transitionTime + START_DELAY);
    isReady_ = false;

    loadChart(chartData_);
    conductor_ = std::make_unique<Conductor>();
    judgementSystem_ = std::make_unique<JudgementSystem>();
    judgementSystem_->reset();

    if (!conductor_->initialize(chartData_.timingPoints, audioPath_, playbackRate_, 0.0f, -(transitionTime + START_DELAY)))
    {
        GAME_LOG_ERROR("PlayState: Failed to initialize conductor");
        return;
    }

    std::string bgName = getChartBackgroundName(chartData_);
    if (!bgName.empty())
    {
        std::string bgPath = chartData_.filePath + "/" + bgName;
        backgroundTexture_ = IMG_LoadTexture(renderer, bgPath.c_str());

        if (!backgroundTexture_)
        {
            GAME_LOG_ERROR("PlayState: Failed to load background image: " + bgPath);
            backgroundTexture_ = nullptr;
        }
        else
        {
            SDL_SetTextureColorMod(backgroundTexture_, 77, 77, 77);
        }
    }

    DebugInfo* debugInfo = appContext->debugInfo;
    conductorInfo_ = new ConductorInfo(renderer, MAIN_FONT_PATH, 16, debugInfo->getYPosition() + debugInfo->getHeight() + 7.0f);
    conductorInfo_->setConductor(conductor_.get());

    skinUtils_ = std::make_unique<SkinUtils>();
    skinUtils_->loadSkin("stars");

    gameplayHud_ = new GameplayHud(appContext, skinUtils_.get());
    gameplayHud_->setConductor(conductor_.get());
    gameplayHud_->setJudgementSystem(judgementSystem_.get());

    playfield_ = new Playfield();
    playfield_->init(renderer, chartData_.keyCount, screenHeight_, skinUtils_.get());
    playfield_->setPosition(screenWidth_ / 2.0f, screenHeight_ / 2.0f);

    playfield_->setConductor(conductor_.get());
    playfield_->setAppContext(appContext);

    playfield_->setJudgementSystem(judgementSystem_.get());
    playfield_->loadNotes(&chartData_);

    conductor_->play();
    conductor_->setOnSongEndCallback([this]()
    {
        requestStateSwitch(STATE_RESULTS, nullptr);
    });

    conductor_->setOnBeatCallback([this](int beat)
    { 
        // TODO: Add beat-based visual effects here
    });

    conductor_->setOnStepCallback([this](int step) 
    { 
        // TODO: Add step-based visual effects here
    });
}

void PlayState::loadChart(const ChartData &data)
{
    // TODO: Add chart loading logic if needed
}

void PlayState::handleEvent(const SDL_Event &event)
{
    if (!conductor_)
        return;

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        bool isStrumKey = false;
        for (int i = 0; i < 4; i++)
        {
            if (event.key.key == appContext->keybinds[i])
            {
                if (playfield_)
                {
                    playfield_->handleStrumInput(event.key.key, true);
                }
                isStrumKey = true;
                break;
            }
        }
        
        if (!isStrumKey)
        {
            switch (event.key.key)
            {
            case SDLK_RETURN:
                if (isPaused_)
                {
                    conductor_->play();
                    isPaused_ = false;
                }
                else
                {
                    conductor_->pause();
                    isPaused_ = true;
                }
                break;
            case SDLK_ESCAPE:
                conductor_->stop();
                requestStateSwitch(STATE_SONG_SELECT, nullptr);
                break;
            }
        }
    }
    else if (event.type == SDL_EVENT_KEY_UP)
    {
        for (int i = 0; i < 4; i++)
        {
            if (event.key.key == appContext->keybinds[i])
            {
                if (playfield_)
                {
                    playfield_->handleStrumInput(event.key.key, false);
                }
                break;
            }
        }
    }
}

float easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

void PlayState::update(float deltaTime)
{
    if (!conductor_)
        return;

    if (!isReady_)
    {
        readyTimer_ += deltaTime;
        
        if (readyTimer_ >= 0.0f)
        {
            isReady_ = true;
        }
    }
    
    conductor_->update(deltaTime);

    if (playfield_) { playfield_->update(deltaTime);}
    if (gameplayHud_) { gameplayHud_->update(deltaTime);}

    if (conductorInfo_)
    {
        DebugInfo* debugInfo = appContext->debugInfo;
        conductorInfo_->setYPosition(debugInfo->getYPosition() + debugInfo->getHeight() + 9.0f);
        
        conductorInfo_->update();
    }
}

void PlayState::render()
{
    if (backgroundTexture_)
    {
        SDL_RenderTexture(renderer, backgroundTexture_, nullptr, nullptr);
    }
    if (playfield_) { playfield_->render(renderer);}
    if (conductorInfo_) { conductorInfo_->render(renderer);}

    if (gameplayHud_) { gameplayHud_->render();}
}

void PlayState::destroy()
{
    if (conductor_)
    {
        conductor_->stop();
        conductor_.reset();
    }

    if (playfield_)
    {
        playfield_->destroy();
        delete playfield_;
        playfield_ = nullptr;
    }

    if (judgementSystem_)
    {
        judgementSystem_.reset();
    }

    if (conductorInfo_)
    {
        delete conductorInfo_;
        conductorInfo_ = nullptr;
    }

    if (gameplayHud_)
    {
        delete gameplayHud_;
        gameplayHud_ = nullptr;
    }



    if (backgroundTexture_) 
    {
        SDL_DestroyTexture(backgroundTexture_);
        backgroundTexture_ = nullptr;
    }
}