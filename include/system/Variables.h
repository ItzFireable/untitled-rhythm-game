#pragma once

#include <SDL3/SDL.h>
#include <system/Logger.h>
#include <system/AudioManager.h>
#include <objects/debug/FPSCounter.h>
#include <objects/debug/DebugInfo.h>

#include <utils/rhythm/ChartUtils.h>
#include <rhythm/JudgementSystem.h>

#define GAME_NAME "?"
#define GAME_VERSION "?.?.?"

enum AppStateID
{
    STATE_SONG_SELECT = 1,
    STATE_PLAY = 2,
    STATE_RESULTS = 3,
    STATE_COUNT
};

extern int FRAMERATE_CAP;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

inline const std::string MAIN_FONT_PATH = "assets/fonts/GoogleSansCode-Bold.ttf";

extern Uint64 lastInputTick;
extern Uint64 lastFrameTick;
extern float currentInputLatencyMs;
extern Uint64 TARGET_TICK_DURATION;

class FPSCounter; 
class DebugInfo;
class BaseState;
class SettingsManager;

using StateSwitcher = void (*)(AppContext*, int, void*);

struct SongSelectData
{
    int previousIndex = 0;
    std::string previousPackName;
    std::string previousSongTitle;

    float previousRate = 1.0f;
};

struct PlayStateData
{
    std::string songPath;
    std::string chartFile;
    ChartData chartData;
    float playbackRate;

    SongSelectData* previousStateData = nullptr;
};

struct ResultsData
{
    float accuracy = 0.0f;
    std::map<Judgement, int> judgementCounts;
    std::vector <JudgementResult> judgementResults;

    PlayStateData* playStateData = nullptr;
};

struct AppContext
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *renderTarget;
    SDL_AppResult appQuit = SDL_APP_CONTINUE;

    AudioManager& audioManager = AudioManager::getInstance();
    
    DebugInfo* debugInfo = nullptr;
    FPSCounter* fpsCounter = nullptr;
    SettingsManager* settingsManager = nullptr;

    StateSwitcher switchState = nullptr;
    BaseState *currentState = nullptr;
    
    float renderWidth = 1920.0f;
    float renderHeight = 1080.0f;

    bool isTransitioning = false;
    float transitionProgress = 0.0f;
    float transitionDuration = 0.3f;
    int nextState = -1;
    void* nextStatePayload = nullptr;
    bool transitioningOut = true;
};