#include "objects/debug/FPSCounter.h"
#include "system/Variables.h"
#include "utils/Utils.h"
#include <iomanip>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <string>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

FPSCounter::FPSCounter(SDL_Renderer *renderer, const std::string &fontPath, int fontSize, float yPos = 6.0f)
{
    textObject_ = new TextObject(renderer, fontPath, fontSize);

    textObject_->setPosition(9.0f, yPos);
    textObject_->setTextGap(-2.0f);
    textObject_->setColor({255, 255, 255, 255});
    textObject_->setText("Init...");
}

FPSCounter::~FPSCounter()
{
    if (textObject_)
    {
        delete textObject_;
        textObject_ = nullptr;
    }

    backgroundRect_ = nullptr;
}

long FPSCounter::getAppMemoryUsageKb()
{
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        return (long)(pmc.WorkingSetSize / 1024);
    }
    return 0;

#elif defined(__linux__)
    long residentSet = 0;
    std::ifstream stat_stream("/proc/self/status", std::ios_base::in);
    std::string line;

    while (std::getline(stat_stream, line))
    {
        if (line.compare(0, 8, "VmRSS:") == 0)
        {
            std::stringstream ss(line.substr(8));
            ss >> residentSet;
            break;
        }
    }
    return residentSet;

#else
    return 0;
#endif
}

void FPSCounter::update()
{
    if (!perfFrequency_)
    {
        perfFrequency_ = SDL_GetPerformanceFrequency();
        lastTick_ = SDL_GetPerformanceCounter();
    }

    if (!textObject_)
        return;

    Uint64 currentTick = SDL_GetPerformanceCounter();
    Uint64 tickDifference = currentTick - lastTick_;
    lastTick_ = currentTick;

    float frameTimeSeconds = (float)tickDifference / (float)perfFrequency_;

    float frameTimeMs = frameTimeSeconds * 1000.0f;
    latestFrameTimeMs_ = frameTimeMs;

    frameAccumulator_ += frameTimeSeconds;
    frameCount_++;

    if (frameAccumulator_ >= 0.1f)
    {
        float fps = (float)frameCount_ / frameAccumulator_;
        memoryUsageKb_ = getAppMemoryUsageKb();

        std::stringstream ss;
        ss << std::fixed;

        ss << "FPS: " << std::setprecision(0) << fps
           << "\nFT: " << std::setprecision(2) << latestFrameTimeMs_ << "ms";

        if (memoryUsageKb_ >= 0)
            ss << "\nMEM: " << Utils::formatMemorySize(memoryUsageKb_ * 1024);

        textObject_->setText(ss.str());
        
        frameAccumulator_ = 0.0f;
        frameCount_ = 0;
    }
}

void FPSCounter::render(SDL_Renderer* renderer) 
{
    if (!textObject_) {
        return;
    }
    
    float textX, textY;
    textObject_->getPosition(textX, textY);

    float textW = textObject_->getRenderedWidth();
    float textH = textObject_->getRenderedHeight();

    float padding = 4.0f;

    SDL_FRect backgroundRect = {
        textX - padding,
        textY - padding + (padding / 2.0f),
        textW + (2.0f * padding),
        textH + (2.0f * padding) - padding
    };

    SDL_Color oldColor;
    SDL_GetRenderDrawColor(renderer, &oldColor.r, &oldColor.g, &oldColor.b, &oldColor.a);
    SDL_BlendMode oldMode;
    SDL_GetRenderDrawBlendMode(renderer, &oldMode);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128); 
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_RenderFillRect(renderer, &backgroundRect);

    SDL_SetRenderDrawColor(renderer, oldColor.r, oldColor.g, oldColor.b, oldColor.a);
    SDL_SetRenderDrawBlendMode(renderer, oldMode);

    textObject_->render();
}