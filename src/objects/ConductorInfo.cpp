#include "objects/ConductorInfo.h"
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

    float songPosition = conductor_->getSongPosition() / conductor_->getPlaybackRate();
    float endTime = conductor_->getSongDuration() / conductor_->getPlaybackRate();

    float songBeats = conductor_->getCurrentBeat();
    float songStep = conductor_->getCurrentStep();
    float songBPM = conductor_->getCurrentBPM();

    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed;

    ss << "TIME: " << songPosition << " / " << endTime << "s\n"
       << "BEAT: " << songBeats << "\n"
       << "STEP: " << songStep << "\n"
       << "BPM: " << songBPM;

    textObject_->setText(ss.str());
}

void ConductorInfo::render(SDL_Renderer* renderer)
{
    FPSCounter::render(renderer);
}