#include "rhythm/Conductor.h"
#include "utils/AudioManager.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>

Conductor::Conductor()
{
}

Conductor::~Conductor()
{
    stop();
}

float Conductor::getSongDuration() const
{
    return AudioManager::getInstance().getMusicDuration();
}

bool Conductor::initialize(const std::vector<TimingPoint> &timingPoints, const std::string &audioPath, float playbackRate, float crossfadeDuration, float startTime)
{
    if (timingPoints.empty())
    {
        GAME_LOG_ERROR("Conductor: No timing points provided");
        return false;
    }

    timingPoints_ = timingPoints;
    std::sort(timingPoints_.begin(), timingPoints_.end(),
              [](const TimingPoint &a, const TimingPoint &b)
              {
                  return a.time < b.time;
              });

    audioPath_ = audioPath;
    playbackRate_ = playbackRate;
    crossfadeDuration_ = crossfadeDuration;

    currentBPM_ = timingPoints_[0].bpm;
    crotchet_ = 60.0f / currentBPM_;
    currentTimingIndex_ = 0;

    songPosition_ = startTime; 
    songPositionInBeats_ = getBeatsAtTime(startTime);

    isInitialized_ = true;
    
    hasAudioStarted_ = (startTime >= 0.0f);
    if (hasAudioStarted_) {
        loadAndPlay(startTime, crossfadeDuration_);
    }

    return true;
}

void Conductor::loadAndPlay(float startTime, float crossfadeDuration)
{
    if (!isInitialized_)
    {
        GAME_LOG_ERROR("Conductor is not initialized! Cannot load audio.");
        return;
    }

    AudioManager &audio = AudioManager::getInstance();

    if (!audio.switchMusicStream(audioPath_, crossfadeDuration, startTime))
    {
        GAME_LOG_ERROR("Conductor: Failed to switch music stream to: " + audioPath_);
        return;
    }

    audio.setMusicPlaybackRate(playbackRate_);
    isPlaying_ = true;
}

void Conductor::play()
{
    if (!isInitialized_)
    {
        GAME_LOG_ERROR("Conductor: Not initialized");
        return;
    }

    AudioManager &audio = AudioManager::getInstance();
    if (!isPlaying_)
    {
        if (hasAudioStarted_) 
        {
            if (songPosition_ == 0.0f)
            {
                seek(0.0f);
                audio.playMusicStream();
            }
            else
            {
                audio.resumeMusicStream();
            }
        }
    }

    isPlaying_ = true;
}

void Conductor::pause()
{
    if (!isPlaying_)
        return;

    AudioManager &audio = AudioManager::getInstance();
    audio.pauseMusicStream();
    isPlaying_ = false;
}

void Conductor::stop()
{
    if (!isInitialized_)
        return;

    AudioManager &audio = AudioManager::getInstance();
    audio.stopMusicStream();

    isPlaying_ = false;
    songPosition_ = 0.0f;
    songPositionInBeats_ = 0.0f;
    currentTimingIndex_ = 0;
    currentBPM_ = timingPoints_[0].bpm;
    crotchet_ = 60.0f / currentBPM_;
}

void Conductor::update(float deltaTime)
{
    if (!isInitialized_)
        return;
    if (hasTriggeredSongEnd_)
        return;

    if (!hasAudioStarted_)
    {
        songPosition_ += deltaTime;
        
        if (songPosition_ >= 0.0f)
        {
            songPosition_ = 0.0f; 
            hasAudioStarted_ = true;
            justStartedAudio_ = true;
            smoothedPosition_ = 0.0f;
            
            loadAndPlay(0.0f, crossfadeDuration_);

            audioPosition_ = 0.0f;
            previousAudioPosition_ = 0.0f;
            songTimeAccumulator_ = 0.0f;
        }
    }

    if (hasAudioStarted_) 
    {
        AudioManager& audio = AudioManager::getInstance();
        float rawPosition  = audio.getMusicPosition();

        if (justStartedAudio_) {
            smoothedPosition_ = std::min(smoothedPosition_ + deltaTime, rawPosition);
            songPosition_ = smoothedPosition_;

            if (std::abs(songPosition_ - rawPosition) < 0.001f) {
                justStartedAudio_ = false;
            }
        } else {
            songPosition_ = rawPosition;
        }

        if (audio.hasSongEndedNaturally())
        {
            hasTriggeredSongEnd_ = true;
            isPlaying_ = false;
            
            if (onSongEndCallback_)
            {
                onSongEndCallback_();
            }

            audio.clearSongEndedFlag();
            audio.stopMusicStream();
            return;
        }
    }

    updateBPM();
    songPositionInBeats_ = getBeatsAtTime(songPosition_);

    int currentBeat = floorf(songPositionInBeats_);
    if (currentBeat != lastReportedBeat_)
    {
        lastReportedBeat_ = currentBeat;
        if (onBeatCallback_)
            onBeatCallback_(currentBeat);
    }

    float stepFloat = songPositionInBeats_ * 4.0f;
    int step = (int)floorf(stepFloat);

    if (step != lastReportedStep_)
    {
        lastReportedStep_ = step;
        if (onStepCallback_)
            onStepCallback_(step);
    }
}

void Conductor::seek(float timeInSeconds)
{
    if (!isInitialized_)
        return;

    AudioManager &audio = AudioManager::getInstance();
    audio.setMusicPosition(timeInSeconds);

    songPosition_ = timeInSeconds;
    songPositionInBeats_ = getBeatsAtTime(timeInSeconds);

    updateBPM();
}

void Conductor::setPlaybackRate(float rate)
{
    playbackRate_ = std::clamp(rate, 0.1f, 5.0f);

    if (isInitialized_)
    {
        AudioManager &audio = AudioManager::getInstance();
        audio.setMusicPlaybackRate(playbackRate_);
    }
}

void Conductor::updateBPM()
{
    int timeMs = static_cast<int>(songPosition_ * 1000.0f);

    int newIndex = findTimingPointIndex(songPosition_);

    if (newIndex != currentTimingIndex_)
    {
        currentTimingIndex_ = newIndex;
        currentBPM_ = timingPoints_[currentTimingIndex_].bpm;
        crotchet_ = 60.0f / currentBPM_;

        if (onBPMChangeCallback_)
        {
            onBPMChangeCallback_(currentBPM_);
        }
    }
}

int Conductor::findTimingPointIndex(float timeInSeconds) const
{
    int timeMs = static_cast<int>(timeInSeconds * 1000.0f);

    for (int i = static_cast<int>(timingPoints_.size()) - 1; i >= 0; --i)
    {
        if (timingPoints_[i].time <= timeMs)
        {
            return i;
        }
    }

    return 0;
}

float Conductor::calculateBeats(float startTime, float endTime) const
{
    if (startTime >= endTime)
        return 0.0f;

    int startMs = static_cast<int>(startTime * 1000.0f);
    int endMs = static_cast<int>(endTime * 1000.0f);

    float totalBeats = 0.0f;

    int startIndex = findTimingPointIndex(startTime);

    for (size_t i = startIndex; i < timingPoints_.size(); ++i)
    {
        float sectionStartTime = std::max(startTime, timingPoints_[i].time / 1000.0f);

        float sectionEndTime;
        if (i + 1 < timingPoints_.size())
        {
            sectionEndTime = std::min(endTime, timingPoints_[i + 1].time / 1000.0f);
        }
        else
        {
            sectionEndTime = endTime;
        }

        if (sectionStartTime >= sectionEndTime)
            break;

        float sectionDuration = sectionEndTime - sectionStartTime;
        float beatsPerSecond = timingPoints_[i].bpm / 60.0f;
        totalBeats += sectionDuration * beatsPerSecond;

        if (sectionEndTime >= endTime)
            break;
    }

    return totalBeats;
}

float Conductor::getBeatsAtTime(float timeInSeconds) const
{
    if (timingPoints_.empty())
        return 0.0f;

    float startTime = timingPoints_[0].time / 1000.0f;

    if (timeInSeconds <= startTime)
    {
        float duration = timeInSeconds;
        float beatsPerSecond = timingPoints_[0].bpm / 60.0f;
        return duration * beatsPerSecond;
    }

    return calculateBeats(startTime, timeInSeconds);
}

float Conductor::getBeatLength(float timePos) const
{
    int index = findTimingPointIndex(timePos);
    if (index >= 0 && index < timingPoints_.size())
    {
        return 60.0f / timingPoints_[index].bpm;
    }
    return 0.5f;
}

float Conductor::getTimeAtBeat(float beat) const
{
    if (timingPoints_.empty() || beat <= 0.0f)
        return 0.0f;

    float currentTime = timingPoints_[0].time / 1000.0f;
    float currentBeat = 0.0f;

    for (size_t i = 0; i < timingPoints_.size(); ++i)
    {
        float sectionStartTime = timingPoints_[i].time / 1000.0f;
        float beatsPerSecond = timingPoints_[i].bpm / 60.0f;

        float sectionEndTime;
        if (i + 1 < timingPoints_.size())
        {
            sectionEndTime = timingPoints_[i + 1].time / 1000.0f;
        }
        else
        {
            float remainingBeats = beat - currentBeat;
            return sectionStartTime + (remainingBeats / beatsPerSecond);
        }

        float sectionDuration = sectionEndTime - sectionStartTime;
        float sectionBeats = sectionDuration * beatsPerSecond;

        if (currentBeat + sectionBeats >= beat)
        {
            float beatsIntoSection = beat - currentBeat;
            return sectionStartTime + (beatsIntoSection / beatsPerSecond);
        }

        currentBeat += sectionBeats;
        currentTime = sectionEndTime;
    }

    return currentTime;
}