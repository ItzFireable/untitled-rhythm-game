#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <future>
#include <atomic>
#include <utils/rhythm/ChartUtils.h>

class Conductor {
public:
    Conductor();
    ~Conductor();

    bool initialize(const std::vector<TimingPoint>& timingPoints, const std::string& audioPath, float playbackRate = 1.0f, float crossfadeDuration = 0.0f, float startTime = 0.0f);
    void loadAndPlay(float startTime, float crossfadeDuration = 0.0f);
    
    void play();
    void pause();
    void stop();
    
    void update(float deltaTime);
    void seek(float timeInSeconds);
    
    float getSongDuration() const;
    float getSongPosition() const { return songPosition_; }

    float getSongPositionInBeats() const { return songPositionInBeats_; }
    float getCurrentBPM() const { return currentBPM_ * playbackRate_; }
    float getCrotchet() const { return crotchet_ * playbackRate_; }
    float getPlaybackRate() const { return playbackRate_; }
    
    bool isPlaying() const { return isPlaying_; }
    bool isInitialized() const { return isInitialized_; }
    
    void setPlaybackRate(float rate);
    
    float getBeatsAtTime(float timeInSeconds) const;
    float getBeatLength(float timePos) const;
    float getTimeAtBeat(float beat) const;
    
    int getCurrentBeat() const { return lastReportedBeat_; }
    int getCurrentStep() const { return lastReportedStep_; }
    
    void setOnBeatCallback(std::function<void(int)> cb) { onBeatCallback_ = cb; }
    void setOnStepCallback(std::function<void(int)> cb) { onStepCallback_ = cb; }
    void setOnSongEndCallback(std::function<void()> cb) { onSongEndCallback_ = cb; }
    void setOnBPMChangeCallback(std::function<void(float)> cb) { onBPMChangeCallback_ = cb; }

    float getAudioTimeFromSamples();
    bool isLoadingAudio() const { return isLoadingAudio_.load(); }

    void setLooping(bool enabled) { enableLooping_ = enabled; }
    void setLoopRegion(float startTime, float endTime = -1.0f) {
        loopStartTime_ = startTime;
        loopEndTime_ = endTime;
    }
    
    void setLoopPauseDuration(float duration) { loopPauseDuration_ = duration; }
    void setLoopFadeOutDuration(float duration) { loopFadeOutDuration_ = duration; }
    bool isLooping() const { return enableLooping_; }
private:
    std::vector<TimingPoint> timingPoints_;
    std::string audioPath_;
    
    float songPosition_ = 0.0f;
    float songPositionInBeats_ = 0.0f;
    float currentBPM_ = 120.0f;
    float crotchet_ = 0.0f;
    float playbackRate_ = 1.0f;
    
    bool isPlaying_ = false;
    bool isInitialized_ = false;
    bool hasTriggeredSongEnd_ = false;

    bool hasAudioStarted_ = false;
    float crossfadeDuration_ = 0.0f;

    float smoothedPosition_ = 0.0f;
    bool justStartedAudio_ = false;

    float audioPosition_ = 0.0f;
    float previousAudioPosition_ = 0.0f;
    float songTimeAccumulator_ = 0.0f;

    bool enableLooping_ = false;
    float loopStartTime_ = 0.0f;
    float loopEndTime_ = -1.0f;
    float loopPauseDuration_ = 2.0f;
    float loopFadeOutDuration_ = 1.5f;

    bool isInLoopPause_ = false;
    float loopPauseTimer_ = 0.0f;
    bool isLoopFadingOut_ = false;
    float loopFadeOutTimer_ = 0.0f;
    float originalVolume_ = 1.0f;
    
    int currentTimingIndex_ = -1;
    int lastReportedBeat_ = -1;
    int lastReportedStep_ = -4;

    std::atomic<bool> isLoadingAudio_{false};
    std::future<bool> audioLoadFuture_;
    float pendingStartTime_ = 0.0f;
    float pendingCrossfade_ = 0.0f;

    std::function<void(int)> onBeatCallback_;
    std::function<void(int)> onStepCallback_;
    std::function<void()> onSongEndCallback_;
    std::function<void(float)> onBPMChangeCallback_;
    
    void updateBPM();
    void loadAndPlayAsync(float startTime, float crossfadeDuration);

    int findTimingPointIndex(float timeInSeconds) const;
    float calculateBeats(float startTime, float endTime) const;
};

#endif