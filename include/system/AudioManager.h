#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>

#include <future>
#include <atomic>

struct SoundEffect
{
    ALuint bufferID = 0;
    std::string name;
};

enum AudioFileType
{
    FILE_TYPE_OGG,
    FILE_TYPE_WAV,
    FILE_TYPE_MP3,
    FILE_TYPE_NONE
};

struct AudioLoadData
{
    std::string filePath;
    float startTime;
    AudioFileType fileType;
    void *streamHandle;
    unsigned int channels;
    unsigned int sampleRate;
    int totalSamples;
    bool success;
};

struct MusicStream
{
    ALuint sourceID = 0;
    std::vector<ALuint> buffers;

    void *streamHandle = nullptr;
    ALenum format = 0;
    ALsizei sampleRate = 0;

    bool isPlaying = false;
    float volume = 1.0f;
    float playbackRate = 1.0f;
    int totalSamples = 0;
    int channels = 0;
    bool isLooping = false;

    std::uint64_t samplesRead = 0;
    std::uint64_t seekOffsetFrames = 0;
    std::uint64_t totalSamplesProcessed = 0;
    AudioFileType fileType = FILE_TYPE_NONE;
};

class AudioManager
{
public:
    static constexpr int NUM_BUFFERS = 4;
    static constexpr int BUFFER_SIZE_SAMPLES = 4096;

    static AudioManager &getInstance()
    {
        static AudioManager instance;
        return instance;
    }

    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;
    AudioManager(AudioManager &&) = delete;
    AudioManager &operator=(AudioManager &&) = delete;

    bool initialize();
    void shutdown();

    bool loadSoundEffect(const std::string &name, const std::string &filePath);
    void playSoundEffect(const std::string &name, float gain = 1.0f);

    bool loadMusicStream(const std::string &filePath, float startTime = 0.0f);
    void playMusicStream();
    void stopMusicStream();
    void pauseMusicStream();
    void resumeMusicStream();

    std::future<bool> loadMusicStreamAsync(const std::string &filePath, float startTime = 0.0f);
    bool isLoadingMusic() const { return isLoadingMusic_.load(); }

    void setMusicPlaybackRate(float rate);
    float getMusicPlaybackRate() const;

    void setMusicVolume(float volume);
    float getMusicVolume() const {
        return currentStream_.volume;
    }

    void setMusicPosition(float timeInSeconds);
    std::uint64_t getMusicSamplesOffset() const;

    float getMusicPosition() const
    {
        const MusicStream* stream = (isCrossfading_ && nextStream_.sourceID != 0) 
            ? &nextStream_ 
            : &currentStream_;

        if (!stream->sourceID || stream->sampleRate == 0)
            return 0.0f;

        ALint offset = 0;
        alGetSourcei(stream->sourceID, AL_SAMPLE_OFFSET, &offset);
        std::uint64_t currentSamplePosition = stream->totalSamplesProcessed + offset;

        return (float)currentSamplePosition / stream->sampleRate;
    }

    float getMusicDuration() const
    {
        const MusicStream* stream = (isCrossfading_ && nextStream_.sourceID != 0) 
            ? &nextStream_ 
            : &currentStream_;
        if (!stream->sourceID || stream->sampleRate == 0)
            return 0.0f;
        if (stream->totalSamples == 0)
            return 0.0f;

        return static_cast<float>(stream->totalSamples) / static_cast<float>(stream->sampleRate);
    }

    bool switchMusicStream(const std::string &filePath, float crossfadeDuration = 0.0f, float startTime = 0.0f);
    bool isMusicPlaying() const { return currentStream_.isPlaying; }
    void forceStopCrossfade();
    void updateStream();

    MusicStream *getActiveStream() { return &currentStream_; }

    bool hasSongEndedNaturally() const { return songEndedNaturally_; }
    void clearSongEndedFlag() { songEndedNaturally_ = false; }

private:
    AudioManager();
    ~AudioManager();

    bool songEndedNaturally_ = false;

    std::atomic<bool> isLoadingMusic_{false};
    std::future<bool> musicLoadFuture_;

    ALCdevice *device_ = nullptr;
    ALCcontext *context_ = nullptr;

    std::map<std::string, SoundEffect> sfxMap_;
    MusicStream currentStream_;

    MusicStream nextStream_;
    float crossfadeProgress_ = 0.0f;
    float crossfadeDuration_ = 0.0f;
    bool isCrossfading_ = false;

    bool checkALError(const std::string &contextMessage);

    bool loadOggToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples);
    bool loadWavToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples);
    bool loadMp3ToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples);

    ALenum getOpenALFormat(unsigned int channels, unsigned int bitsPerSample);

    bool stream(ALuint bufferID);
    void closeStream(MusicStream *stream);

    bool streamToBuffer(ALuint bufferID, MusicStream *stream);
    void updateStreamBuffers(MusicStream *stream);
};

#endif