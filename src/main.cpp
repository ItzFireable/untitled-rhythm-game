#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>
#include <iostream>

#include <state.h>

State *state = NULL;
int curState = -1;
int prevState = -1; 

struct AppContext
{
    SDL_Window *window;
    SDL_GLContext glContext;
    SDL_AppResult appQuit = SDL_APP_CONTINUE;
};

SDL_AppResult SDL_Fail()
{
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Failed to load video, quitting..." << std::endl;
        return SDL_Fail();
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    SDL_Window *window = SDL_CreateWindow("SDL3, OpenGL 4.6, C++", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
    if (!window)
    {
        return SDL_Fail();
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        return SDL_Fail();
    }

    if (!gladLoadGL())
    {
        std::cout << "Failed to load OpenGL functions, quitting..." << std::endl;
        return SDL_APP_FAILURE;
    }

    glClearColor(0.f, 0.5f, 0.5f, 1.f);
    SDL_ShowWindow(window);

    *appstate = new AppContext {
        window,
        glContext,
    };

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    auto *app = (AppContext *)appstate;
    if (state != nullptr)
    {
        state->handleEvent(*event);
    }

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            app->appQuit = SDL_APP_SUCCESS;
            break;
        }
        default:
        {
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    auto *app = (AppContext *)appstate;

    if (curState < 0)
        curState = 0;

    if (curState != prevState)
    {
        if (state)
        {
            state->destroy();
            delete state;
        }
        
        state = new State();
        state->init();
        prevState = curState;

        std::cout << "Initialized state: " << state->getName() << std::endl;
    }

    // Variable to see if state is valid
    bool stateValid = (state != nullptr);
    glClear(GL_COLOR_BUFFER_BIT);

    if (stateValid) {
        state->update();
    }

    SDL_GL_SwapWindow(app->window);

    if (stateValid) {
        state->postBuffer();
    }

    return app->appQuit;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *app = (AppContext *)appstate;
    if (app)
    {
        SDL_GL_DestroyContext(app->glContext);
        SDL_DestroyWindow(app->window);
        delete app;
    }

    SDL_Quit();
}