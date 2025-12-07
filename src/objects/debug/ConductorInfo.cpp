#include "objects/debug/ConductorInfo.h"
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

ConductorInfo::ConductorInfo(SDL_Renderer* renderer, const std::string& fontPath, int fontSize, float yPos)
    : FPSCounter(renderer, fontPath, fontSize, yPos)
{

}

void ConductorInfo::update()
{
    if (!textObject_)
        return;

    Conductor* conductor = this->conductor_;
    if (!conductor)
        return;

    bool isInitialized = conductor_->isInitialized();
    bool isAudioLoading = conductor_->isLoadingAudio();
    bool isPlaying = conductor_->isPlaying();

    float songPosition = 0.0f;
    float endTime = 0.0f;

    float songBeats = 0.0f;
    float songStep = 0.0f;
    float songBPM = 0.0f;

    if (isInitialized && !isAudioLoading) { 
        songPosition = conductor_->getSongPosition() / conductor_->getPlaybackRate();
        endTime = conductor_->getSongDuration() / conductor_->getPlaybackRate();

        songBeats = conductor_->getCurrentBeat();
        songStep = conductor_->getCurrentStep();
        songBPM = conductor_->getCurrentBPM();
    }

    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed;

    ss << "TIME: " << songPosition << " / " << endTime << "s\n"
       << "BEAT: " << songBeats << "\n"
       << "STEP: " << songStep << "\n"
       << "BPM: " << songBPM;
    ss << "\nSTATUS: " << (isPlaying ? "Playing" : "Paused");
    ss << "\nAUDIO LOAD: " << (isInitialized ? (isAudioLoading ? "Loading..." : "Loaded") : "Not Initialized");

    textObject_->setText(ss.str());
}

void ConductorInfo::render(SDL_Renderer* renderer)
{
    FPSCounter::render(renderer);
}