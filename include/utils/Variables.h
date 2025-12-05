#pragma once

#include <SDL3/SDL.h>
#include <utils/Logger.h>
#include <utils/AudioManager.h>
#include <objects/FPSCounter.h>
#include <objects/DebugInfo.h>

#include <rhythm/ChartManager.h>

#define GAME_NAME "?"
#define GAME_VERSION "?.?.?"

enum AppStateID
{
    STATE_SONG_SELECT = 1,
    STATE_PLAY = 2,
    STATE_RESULTS = 3,
    STATE_COUNT
};

static constexpr int FRAMERATE_CAP = 999;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;

inline const std::string MAIN_FONT_PATH = "assets/fonts/GoogleSansCode-Bold.ttf";

extern Uint64 lastInputTick;
extern Uint64 lastFrameTick;
extern float currentInputLatencyMs;
extern Uint64 TARGET_TICK_DURATION;

#define KEYBIND_STRUM_LEFT SDLK_Z
#define KEYBIND_STRUM_DOWN SDLK_X
#define KEYBIND_STRUM_UP SDLK_COMMA
#define KEYBIND_STRUM_RIGHT SDLK_PERIOD

class FPSCounter; 
class DebugInfo;
class BaseState;

using StateSwitcher = void (*)(AppContext*, int, void*);

struct PlayStateData
{
    std::string songPath;
    std::string chartFile;
    ChartData chartData;
    float playbackRate;
};

struct AppContext
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *renderTarget;
    SDL_AppResult appQuit = SDL_APP_CONTINUE;

    AudioManager& audioManager = AudioManager::getInstance();
    FPSCounter* fpsCounter = nullptr;
    DebugInfo* debugInfo = nullptr;

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