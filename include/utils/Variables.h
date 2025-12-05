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

    SDL_Keycode keybinds[4] = {SDLK_A, SDLK_S, SDLK_K, SDLK_L};
};