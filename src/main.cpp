#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>

#include <utils/AudioManager.h>
#include <utils/SettingsManager.h>
#include <utils/Variables.h>

#include <BaseState.h>
#include <states/SongSelectState.h>
#include <states/PlayState.h>
#include <states/ResultsState.h>
#include <objects/FPSCounter.h>

#include <utils/OsuUtils.h>

BaseState *state = NULL;
void* statePayload = nullptr;

int curState = -1;
int prevState = -1;

int WINDOW_WIDTH = 1600;
int WINDOW_HEIGHT = 900;
int FRAMERATE_CAP = 999;

Uint64 lastInputTick = 0;
Uint64 lastFrameTick = 0;
float currentInputLatencyMs = 0.0f;

Uint64 TARGET_TICK_DURATION = SDL_GetPerformanceFrequency() / FRAMERATE_CAP;

BaseState *createState(int stateID)
{
    switch (stateID)
    {
    case STATE_SONG_SELECT:
        return new SongSelectState();
    case STATE_PLAY:
        return new PlayState();
    case STATE_RESULTS:
        return new ResultsState();
    default:
        return nullptr;
    }
}

void setState(AppContext* app, int stateID, void* payload)
{
    if (stateID >= 0 && stateID < STATE_COUNT)
    {
        if (!app->isTransitioning) {
            app->isTransitioning = true;
            app->transitioningOut = true;
            app->transitionProgress = 0.0f;
            app->nextState = stateID;
            app->nextStatePayload = payload;
        }
    }
}

bool setRenderResolution(AppContext *app, float width, float height) {
    if (app->renderTarget) {
        SDL_DestroyTexture(app->renderTarget);
    }

    app->renderTarget = SDL_CreateTexture(
        app->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        (int)width,
        (int)height
    );
    
    if (!app->renderTarget) {
        GAME_LOG_ERROR("Failed to recreate render target: " + std::string(SDL_GetError()));
        return false;
    }

    app->renderWidth = width;
    app->renderHeight = height;
    
    return true;
}

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    Logger::getInstance().setLogLevel(LogLevel::GAME_DEBUG);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        GAME_LOG_ERROR("Failed to initialize SDL: " + std::string(SDL_GetError()));
        return SDL_Fail();
    }

    if (TTF_Init() == -1) {
        GAME_LOG_ERROR("Failed to initialize SDL_ttf: " + std::string(SDL_GetError()));
        return SDL_Fail();
    }

    SettingsManager *settingsManager = new SettingsManager();
    settingsManager->loadSettings();

    char *title = (char *)malloc(256);
    snprintf(title, 256, "%s v%s", GAME_NAME, GAME_VERSION);

    WINDOW_WIDTH = settingsManager->getSetting<int>("windowWidth", 1600);
    WINDOW_HEIGHT = settingsManager->getSetting<int>("windowHeight", 900);

    FRAMERATE_CAP = settingsManager->getSetting<int>("fpsCap", 999);
    TARGET_TICK_DURATION = SDL_GetPerformanceFrequency() / FRAMERATE_CAP;

    SDL_Window *window = SDL_CreateWindow(title, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT);
    if (!window)
    {
        GAME_LOG_ERROR("Failed to create SDL window: " + std::string(SDL_GetError()));
        return SDL_Fail();
    }

    auto *app = new AppContext();
    *appstate = app;

    app->window = window;
    app->renderer = SDL_CreateRenderer(window, NULL);
    app->fpsCounter = new FPSCounter(app->renderer, MAIN_FONT_PATH, 16, 7.0f);

    app->fpsCounter->setAppContext(app);
    app->fpsCounter->update();

    app->debugInfo = new DebugInfo(app->renderer, MAIN_FONT_PATH, 16, 30.0f);
    app->debugInfo->setAppContext(app);

    app->debugInfo->setYPosition(app->fpsCounter->getYPosition() + app->fpsCounter->getHeight() + 5.0f);
    app->debugInfo->update();

    app->switchState = setState;
    app->settingsManager = settingsManager;

    OsuUtils* osuUtils = new OsuUtils();
    osuUtils->parseOsuDatabase();

    const std::string keyNames[4] = {"keyLeft", "keyDown", "keyUp", "keyRight"};
    const std::string defaults[4] = {"A", "S", "K", "L"};
    
    for (int i = 0; i < 4; i++) {
        std::string key = app->settingsManager->getSetting(keyNames[i], defaults[i]);
        app->keybinds[i] = SDL_GetKeyFromName(key.c_str());
    }

    if (!app->renderer)
    {
        GAME_LOG_ERROR("Failed to create SDL renderer: " + std::string(SDL_GetError()));
        return SDL_Fail();
    }

    setRenderResolution(app, app->renderWidth, app->renderHeight);
    if (!app->renderTarget)
    {
        GAME_LOG_ERROR("Failed to create render target texture: " + std::string(SDL_GetError()));
        return SDL_Fail();
    }

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    SDL_ShowWindow(window);

    if (!AudioManager::getInstance().initialize()) {
        GAME_LOG_ERROR("Failed to initialize AudioManager.");
        return SDL_Fail();
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    auto *app = (AppContext *)appstate;

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
    {
        app->appQuit = SDL_APP_SUCCESS;
        break;
    }
    default:
    {
        if (state != nullptr)
        {
            state->handleEvent(*event);
        }
    }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    auto *app = (AppContext *)appstate;

    if (curState < 0)
        curState = STATE_SONG_SELECT;

    Uint64 frameStartTime = SDL_GetPerformanceCounter();
    float deltaTime = (float)(frameStartTime - lastFrameTick) / (float)SDL_GetPerformanceFrequency();
    lastFrameTick = frameStartTime;

   if (app->isTransitioning)
    {
        app->transitionProgress += deltaTime / app->transitionDuration;
        
        if (app->transitioningOut && app->transitionProgress >= 1.0f)
        {
            if (state)
            {
                state->destroy();
                delete state;
                state = nullptr;
            }
            
            curState = app->nextState;
            state = createState(curState);
            prevState = curState;
            
            if (state != nullptr)
            {
                state->init(app, app->nextStatePayload);
                app->nextStatePayload = nullptr;
                app->currentState = state;
            }
            
            app->transitioningOut = false;
            app->transitionProgress = 0.0f;

            lastFrameTick = SDL_GetPerformanceCounter();
        }
        else if (!app->transitioningOut && app->transitionProgress >= 1.0f)
        {
            app->isTransitioning = false;
            app->transitionProgress = 0.0f;
        }
    }

    if (!app->isTransitioning && curState != prevState)
    {
        if (state)
        {
            state->destroy();
            delete state;
            state = nullptr;
        }

        state = createState(curState);
        prevState = curState;

        if (state != nullptr)
        {
            state->init(app, statePayload);
            statePayload = nullptr;
            app->currentState = state;
        }
    }

    if (app->renderer != nullptr)
    {
        Utils::getWindowSize(app->window, WINDOW_WIDTH, WINDOW_HEIGHT);
        AudioManager::getInstance().updateStream();

        if (app->fpsCounter) {
            app->fpsCounter->update();
        }

        if (app->debugInfo) {
            FPSCounter* fpsCounter = app->fpsCounter;
            app->debugInfo->setYPosition(fpsCounter->getYPosition() + fpsCounter->getHeight() + 5.0f);

            app->debugInfo->update();
        }

        SDL_SetRenderTarget(app->renderer, app->renderTarget);
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderClear(app->renderer);

        if (state != nullptr)
        {
            state->update(deltaTime);
            state->render();
        }

        if (app->fpsCounter) {
            app->fpsCounter->render(app->renderer);
        }

        if (app->debugInfo) {
            app->debugInfo->render(app->renderer);
        }

        if (app->isTransitioning)
        {
            float fadeAlpha;
            if (app->transitioningOut) {
                fadeAlpha = app->transitionProgress;
            } else {
                fadeAlpha = 1.0f - app->transitionProgress;
            }
            
            Uint8 alpha = (Uint8)(std::clamp(fadeAlpha, 0.0f, 1.0f) * 255.0f);
            SDL_FRect fadeRect = {0, 0, app->renderWidth, app->renderHeight};
            
            SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, alpha);
            SDL_RenderFillRect(app->renderer, &fadeRect);

            SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 255);
        }

        SDL_SetRenderTarget(app->renderer, NULL);
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
        SDL_RenderClear(app->renderer);

        float scaleX = (float)WINDOW_WIDTH / app->renderWidth;
        float scaleY = (float)WINDOW_HEIGHT / app->renderHeight;
        float scale = (scaleX < scaleY) ? scaleX : scaleY;

        int scaledWidth = (int)(app->renderWidth * scale);
        int scaledHeight = (int)(app->renderHeight * scale);
        int offsetX = (WINDOW_WIDTH - scaledWidth) / 2;
        int offsetY = (WINDOW_HEIGHT - scaledHeight) / 2;

        SDL_FRect destRect = {
            (float)offsetX,
            (float)offsetY,
            (float)scaledWidth,
            (float)scaledHeight
        };

        SDL_RenderTexture(app->renderer, app->renderTarget, NULL, &destRect);
        SDL_RenderPresent(app->renderer);

        if (state != nullptr)
        {
            state->postBuffer();
        }

        Uint64 frameEndTime = SDL_GetPerformanceCounter();
        Uint64 frameDurationTicks = frameEndTime - frameStartTime;

        if (frameDurationTicks < TARGET_TICK_DURATION)
        {
            Uint64 waitTicks = TARGET_TICK_DURATION - frameDurationTicks;
            
            float waitTimeMs = (float)waitTicks * 1000.0f / (float)SDL_GetPerformanceFrequency();
            float pollingMarginMs = 0.5f; 

            if (waitTimeMs > pollingMarginMs) {
                SDL_Delay((Uint32)(waitTimeMs - pollingMarginMs));
            }

            Uint64 finalWaitEnd = frameStartTime + TARGET_TICK_DURATION;
            while (SDL_GetPerformanceCounter() < finalWaitEnd) { } 
        }
    }

    return app->appQuit;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (state != nullptr)
    {
        state->destroy();
        delete state;
        state = nullptr;
    }

    Logger::getInstance().shutdown();

    auto *app = (AppContext *)appstate;
    if (app)
    {
        if (app->fpsCounter) {
            delete app->fpsCounter;
            app->fpsCounter = nullptr;
        }
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        delete app;
    }

    TTF_Quit();
    SDL_Quit();
}