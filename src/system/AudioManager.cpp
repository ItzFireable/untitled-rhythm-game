

#include "system/AudioManager.h"
#include "system/Logger.h"
#include <stdexcept>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <numeric>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

std::string getFileExtension(const std::string &filePath)
{
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == filePath.length() - 1)
    {
        return "";
    }
    std::string ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

ALenum AudioManager::getOpenALFormat(unsigned int channels, unsigned int bitsPerSample)
{
    if (channels == 1)
    {
        if (bitsPerSample == 8)
            return AL_FORMAT_MONO8;
        if (bitsPerSample == 16)
            return AL_FORMAT_MONO16;
    }
    else if (channels == 2)
    {
        if (bitsPerSample == 8)
            return AL_FORMAT_STEREO8;
        if (bitsPerSample == 16)
            return AL_FORMAT_STEREO16;
    }
    return AL_NONE;
}

void AudioManager::closeStream(MusicStream *stream)
{
    if (!stream || !stream->streamHandle)
        return;

    switch (stream->fileType)
    {
    case FILE_TYPE_OGG:
        stb_vorbis_close(static_cast<stb_vorbis *>(stream->streamHandle));
        break;
    case FILE_TYPE_WAV:
        drwav_uninit(static_cast<drwav *>(stream->streamHandle));
        delete static_cast<drwav *>(stream->streamHandle);
        break;
    case FILE_TYPE_MP3:
        drmp3_uninit(static_cast<drmp3 *>(stream->streamHandle));
        delete static_cast<drmp3 *>(stream->streamHandle);
        break;
    case FILE_TYPE_NONE:
        break;
    }
    stream->streamHandle = nullptr;
}

bool AudioManager::streamToBuffer(ALuint bufferID, MusicStream *stream)
{
    if (!stream || !stream->streamHandle)
        return false;

    if (stream->sourceID == 0) {
        GAME_LOG_ERROR("FATAL ERROR: Attempted to queue buffer on invalid (0) source ID in streamToBuffer.");
        return false;
    }

    int numChannels = (stream->format == AL_FORMAT_MONO16) ? 1 : 2;
    int shortsToRead = BUFFER_SIZE_SAMPLES * numChannels;

    std::vector<short> pcmData(shortsToRead);
    std::uint64_t framesRead = 0;

    switch (stream->fileType)
    {
    case FILE_TYPE_OGG:
        framesRead = stb_vorbis_get_samples_short_interleaved(
            static_cast<stb_vorbis *>(stream->streamHandle),
            numChannels,
            pcmData.data(),
            shortsToRead);
        break;
    case FILE_TYPE_WAV:
        framesRead = drwav_read_pcm_frames_s16(
            static_cast<drwav *>(stream->streamHandle),
            BUFFER_SIZE_SAMPLES,
            pcmData.data());
        break;
    case FILE_TYPE_MP3:
        framesRead = drmp3_read_pcm_frames_s16(
            static_cast<drmp3 *>(stream->streamHandle),
            BUFFER_SIZE_SAMPLES,
            pcmData.data());
        break;
    case FILE_TYPE_NONE:
        return false;
    }

    if (framesRead == 0)
    {
        return false;
    }

    if (stream->sampleRate <= 0)
    {
        GAME_LOG_ERROR("ERROR: Invalid sample rate in streamToBuffer: " + std::to_string(stream->sampleRate));
        return false;
    }

    ALsizei dataSize = (ALsizei)(framesRead * numChannels * sizeof(short));
    if (dataSize <= 0) {
        GAME_LOG_ERROR("Buffer data size is 0 or less!");
        return false;
    }

    alBufferData(bufferID, stream->format, pcmData.data(),
                 dataSize, stream->sampleRate);

    alSourceQueueBuffers(stream->sourceID, 1, &bufferID);

    stream->samplesRead += framesRead;
    return !checkALError("streamToBuffer");
}

AudioManager::AudioManager() : device_(nullptr), context_(nullptr) {}

AudioManager::~AudioManager()
{
    shutdown();
}

bool AudioManager::checkALError(const std::string &contextMessage)
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        GAME_LOG_ERROR("OpenAL Error (" + contextMessage + "): ");
        
        switch (error)
        {
        case AL_INVALID_NAME:
            GAME_LOG_ERROR("AL_INVALID_NAME");
            break;
        case AL_INVALID_ENUM:
            GAME_LOG_ERROR("AL_INVALID_ENUM");
            break;
        case AL_INVALID_VALUE:
            GAME_LOG_ERROR("AL_INVALID_VALUE");
            break;
        case AL_INVALID_OPERATION:
            GAME_LOG_ERROR("AL_INVALID_OPERATION");
            break;
        case AL_OUT_OF_MEMORY:
            GAME_LOG_ERROR("AL_OUT_OF_MEMORY");
            break;
        default:
            GAME_LOG_ERROR("Unknown error code: " + std::to_string(error));
            break;
        }
        return true;
    }

    if (device_)
    {
        ALCenum alcError = alcGetError(device_);
        if (alcError != ALC_NO_ERROR)
        {
            GAME_LOG_ERROR("OpenAL Context Error (" + contextMessage + "): ");
            
            const ALCchar *errStr = alcGetString(device_, alcError);
            GAME_LOG_ERROR(errStr ? std::string(reinterpret_cast<const char*>(errStr)) : "Unknown");
            return true;
        }
    }
    return false;
}

bool AudioManager::initialize()
{
    if (device_ != nullptr || context_ != nullptr)
    {
        return true;
    }

#ifdef _WIN32
    HMODULE openalModule = GetModuleHandleA("OpenAL32.dll");
    if (!openalModule)
    {
        openalModule = LoadLibraryA("OpenAL32.dll");
        if (!openalModule)
        {
            DWORD error = GetLastError();
            GAME_LOG_ERROR("FATAL: Failed to load OpenAL32.dll");
            GAME_LOG_ERROR("Windows Error Code: " + std::to_string(error));

            return false;
        }
    }
#endif
    device_ = alcOpenDevice(nullptr);

    if (!device_)
    {
        GAME_LOG_ERROR("FATAL: alcOpenDevice returned NULL");
        
        ALCenum error = alcGetError(nullptr);
        if (error != ALC_NO_ERROR)
        {
            const ALCchar *errorStr = alcGetString(nullptr, error);
            GAME_LOG_ERROR("ALC Error: " + std::string(errorStr ? reinterpret_cast<const char*>(errorStr) : "Unknown"));
            GAME_LOG_ERROR("Error Code: 0x" + std::to_string(error));
        }
        else
        {
            GAME_LOG_ERROR("No error code available.");
        }

        return false;
    }

    const ALCchar *deviceName = alcGetString(device_, ALC_DEVICE_SPECIFIER);
    context_ = alcCreateContext(device_, nullptr);

    if (!context_)
    {
        GAME_LOG_ERROR("FATAL: alcCreateContext returned NULL");
        ALCenum error = alcGetError(device_);
        if (error != ALC_NO_ERROR)
        {
            const ALCchar *errorStr = alcGetString(device_, error);
            GAME_LOG_ERROR("ALC Error: " + std::string(errorStr ? reinterpret_cast<const char*>(errorStr) : "Unknown"));
        }

        alcCloseDevice(device_);
        device_ = nullptr;
        return false;
    }
    if (!alcMakeContextCurrent(context_))
    {
        GAME_LOG_ERROR("FATAL: alcMakeContextCurrent failed");
        shutdown();
        return false;
    }

    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    ALfloat orientation[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
    alListenerfv(AL_ORIENTATION, orientation);
    alListenerf(AL_GAIN, 1.0f);

    return true;
}

void AudioManager::shutdown()
{
    if (!device_ && !context_)
    {
        return;
    }

    stopMusicStream();
    for (auto const &[name, sfx] : sfxMap_)
    {
        if (sfx.bufferID)
        {
            alDeleteBuffers(1, &sfx.bufferID);
        }
    }
    sfxMap_.clear();

    if (context_)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context_);
        context_ = nullptr;
    }

    if (device_)
    {
        ALCboolean result = alcCloseDevice(device_);
        if (!result)
        {
            GAME_LOG_ERROR("WARNING: Failed to close device properly");
        }
        device_ = nullptr;
    }
}

void AudioManager::setMusicPlaybackRate(float rate)
{
    currentStream_.playbackRate = std::clamp(rate, 0.0f, 4.0f);

    if (currentStream_.sourceID)
    {
        alSourcef(currentStream_.sourceID, AL_PITCH, currentStream_.playbackRate);
        checkALError("setMusicPlaybackRate");
    }
}

void AudioManager::setMusicVolume(float volume)
{
    currentStream_.volume = std::clamp(volume, 0.0f, 1.0f);

    if (currentStream_.sourceID)
    {
        alSourcef(currentStream_.sourceID, AL_GAIN, currentStream_.volume);
        checkALError("setMusicVolume");
    }
}

void AudioManager::setMusicPosition(float timeInSeconds)
{
    if (!currentStream_.sourceID || !currentStream_.streamHandle)
        return;

    std::uint64_t targetSample = static_cast<std::uint64_t>(timeInSeconds * currentStream_.sampleRate);
    targetSample = std::min(targetSample, static_cast<std::uint64_t>(currentStream_.totalSamples));

    switch (currentStream_.fileType)
    {
    case FILE_TYPE_OGG:
        stb_vorbis_seek(static_cast<stb_vorbis *>(currentStream_.streamHandle), targetSample);
        currentStream_.samplesRead = stb_vorbis_get_sample_offset(static_cast<stb_vorbis *>(currentStream_.streamHandle));
        break;
    case FILE_TYPE_WAV:
        drwav_seek_to_pcm_frame(static_cast<drwav *>(currentStream_.streamHandle), targetSample);
        break;
    case FILE_TYPE_MP3:
        drmp3_seek_to_pcm_frame(static_cast<drmp3 *>(currentStream_.streamHandle), targetSample);
        break;
    case FILE_TYPE_NONE:
        return;
    }

    currentStream_.samplesRead = targetSample;
    currentStream_.totalSamplesProcessed = targetSample;

    alSourceStop(currentStream_.sourceID);
    ALint queued = 0;
    alGetSourcei(currentStream_.sourceID, AL_BUFFERS_QUEUED, &queued);
    while (queued--)
    {
        ALuint buf;
        alSourceUnqueueBuffers(currentStream_.sourceID, 1, &buf);
    }

    for (int i = 0; i < NUM_BUFFERS; ++i)
    {
        if (!streamToBuffer(currentStream_.buffers[i], &currentStream_))
            break;
    }

    if (currentStream_.isPlaying)
    {
        alSourcePlay(currentStream_.sourceID);
    }
}

std::uint64_t AudioManager::getMusicSamplesOffset() const
{
    if (!currentStream_.sourceID)
        return 0;

    ALint sampleOffset = 0;
    alGetSourcei(currentStream_.sourceID, AL_SAMPLE_OFFSET, &sampleOffset);

    return currentStream_.totalSamplesProcessed + (std::uint64_t)sampleOffset;
}

bool AudioManager::loadOggToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples)
{
    int error = 0;
    stb_vorbis *vorbis = stb_vorbis_open_filename(filePath.c_str(), &error, nullptr);

    if (error || !vorbis)
    {
        GAME_LOG_ERROR("ERROR: Failed to open OGG file: " + filePath);
        GAME_LOG_ERROR("stb_vorbis error code: " + std::to_string(error));
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    sampleRate = info.sample_rate;
    totalSamples = stb_vorbis_stream_length_in_samples(vorbis);

    if (info.channels == 1)
    {
        format = AL_FORMAT_MONO16;
    }
    else if (info.channels == 2)
    {
        format = AL_FORMAT_STEREO16;
    }
    else
    {
        GAME_LOG_ERROR("ERROR: Unsupported channel count: " + std::to_string(info.channels));
        
        stb_vorbis_close(vorbis);
        return false;
    }

    int dataSize = totalSamples * info.channels;
    std::vector<short> pcmData(dataSize);

    int readSamples = stb_vorbis_get_samples_short_interleaved(
        vorbis, info.channels, pcmData.data(), dataSize);

    alBufferData(bufferID, format, pcmData.data(),
                 static_cast<ALsizei>(pcmData.size() * sizeof(short)), sampleRate);

    stb_vorbis_close(vorbis);

    return !checkALError("loadOggToBuffer");
}

bool AudioManager::loadWavToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples)
{
    unsigned int channels;
    unsigned int rate;
    drwav_uint64 totalPCMFrameCount;

    short *pcmData = drwav_open_file_and_read_pcm_frames_s16(
        filePath.c_str(),
        &channels,
        &rate,
        &totalPCMFrameCount,
        nullptr);

    if (pcmData == nullptr)
    {
        GAME_LOG_ERROR("ERROR: Failed to open or decode WAV file: " + filePath);
        
        return false;
    }

    unsigned int bitsPerSample = 16;
    format = getOpenALFormat(channels, bitsPerSample);
    sampleRate = rate;
    totalSamples = (int)totalPCMFrameCount;

    if (format == AL_NONE)
    {
        GAME_LOG_ERROR("ERROR: Unsupported WAV format: " + std::to_string(channels) + " channels, " + std::to_string(bitsPerSample) + " bits.");
        
        drwav_free(pcmData, nullptr);
        return false;
    }

    ALsizei dataSize = (ALsizei)(totalPCMFrameCount * channels * sizeof(short));

    alBufferData(bufferID, format, pcmData, dataSize, sampleRate);

    drwav_free(pcmData, nullptr);

    return !checkALError("loadWavToBuffer");
}

bool AudioManager::loadMp3ToBuffer(const std::string &filePath, ALuint bufferID, ALenum &format, ALsizei &sampleRate, int &totalSamples)
{
    std::uint64_t totalPCMFrameCount;

    drmp3_config config = {0};

    short *pcmData = (short *)drmp3_open_file_and_read_pcm_frames_s16(
        filePath.c_str(),
        &config,
        &totalPCMFrameCount,
        nullptr);

    if (pcmData == nullptr)
    {
        GAME_LOG_ERROR("ERROR: Failed to open or decode MP3 file: " + filePath);
        
        return false;
    }

    unsigned int channels = config.channels;
    unsigned int rate = config.sampleRate;

    unsigned int bitsPerSample = 16;
    format = getOpenALFormat(channels, bitsPerSample);
    sampleRate = rate;
    totalSamples = (int)totalPCMFrameCount;

    if (format == AL_NONE)
    {
        GAME_LOG_ERROR("ERROR: Unsupported MP3 format: " + std::to_string(channels) + " channels, " + std::to_string(bitsPerSample) + " bits.");
        
        drmp3_free(pcmData, nullptr);
        return false;
    }

    ALsizei dataSize = (ALsizei)(totalPCMFrameCount * channels * sizeof(short));

    alBufferData(bufferID, format, pcmData, dataSize, sampleRate);

    drmp3_free(pcmData, nullptr);

    return !checkALError("loadMp3ToBuffer");
}

bool AudioManager::loadSoundEffect(const std::string &name, const std::string &filePath)
{
    if (sfxMap_.count(name))
    {
        return true;
    }

    ALuint bufferID;
    alGenBuffers(1, &bufferID);
    if (checkALError("loadSoundEffect/alGenBuffers"))
        return false;

    ALenum format;
    ALsizei sampleRate;
    int totalSamples;
    bool loaded = false;

    std::string ext = getFileExtension(filePath);

    if (ext == "ogg")
    {
        loaded = loadOggToBuffer(filePath, bufferID, format, sampleRate, totalSamples);
    }
    else if (ext == "wav")
    {
        loaded = loadWavToBuffer(filePath, bufferID, format, sampleRate, totalSamples);
    }
    else if (ext == "mp3")
    {
        loaded = loadMp3ToBuffer(filePath, bufferID, format, sampleRate, totalSamples);
    }
    else
    {
        GAME_LOG_ERROR("ERROR: Unsupported SFX file format: " + ext + " (Supported: ogg, wav, mp3)");
    }

    if (!loaded)
    {
        alDeleteBuffers(1, &bufferID);
        return false;
    }

    sfxMap_[name] = {bufferID, name};
    return true;
}

void AudioManager::playSoundEffect(const std::string &name, float gain)
{
    if (!sfxMap_.count(name))
    {
        GAME_LOG_ERROR("ERROR: Unknown SFX: " + name);
        
        return;
    }

    ALuint sourceID;
    alGenSources(1, &sourceID);
    if (checkALError("playSoundEffect/alGenSources"))
        return;

    alSourcei(sourceID, AL_BUFFER, static_cast<ALint>(sfxMap_[name].bufferID));
    alSourcef(sourceID, AL_GAIN, std::clamp(gain, 0.0f, 1.0f));
    alSourcei(sourceID, AL_LOOPING, AL_FALSE);
    alSourcei(sourceID, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);

    alSourcePlay(sourceID);
    checkALError("playSoundEffect/alSourcePlay");

    alDeleteSources(1, &sourceID);
}

bool AudioManager::loadMusicStream(const std::string &filePath, float startTime)
{
    stopMusicStream();

    std::string ext = getFileExtension(filePath);
    if (ext == "ogg")
        currentStream_.fileType = FILE_TYPE_OGG;
    else if (ext == "wav")
        currentStream_.fileType = FILE_TYPE_WAV;
    else if (ext == "mp3")
        currentStream_.fileType = FILE_TYPE_MP3;
    else
    {
        GAME_LOG_ERROR("ERROR: Unsupported stream file format: " + ext);
        
        return false;
    }

    bool success = false;
    unsigned int channels = 0;
    unsigned int sampleRate = 0;

    switch (currentStream_.fileType)
    {
    case FILE_TYPE_OGG:
    {
        int error = 0;
        stb_vorbis *vorbis = stb_vorbis_open_filename(filePath.c_str(), &error, nullptr);

        if (error || !vorbis)
        {
            GAME_LOG_ERROR("ERROR: Failed to open OGG file: " + filePath);
            GAME_LOG_ERROR("stb_vorbis error code: " + std::to_string(error));
            break;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        currentStream_.streamHandle = vorbis;
        channels = info.channels;
        sampleRate = info.sample_rate;
        currentStream_.totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
        success = true;
        break;
    }
    case FILE_TYPE_WAV:
    {
        drwav *wav = new drwav();
        if (!drwav_init_file(wav, filePath.c_str(), nullptr))
        {
            GAME_LOG_ERROR("ERROR: Failed to open WAV file: " + filePath);
            
            delete wav;
            break;
        }

        currentStream_.streamHandle = wav;
        channels = wav->channels;
        sampleRate = wav->sampleRate;
        currentStream_.totalSamples = (int)wav->totalPCMFrameCount;
        success = true;
        break;
    }
    case FILE_TYPE_MP3:
    {
        drmp3 *mp3 = new drmp3();
        if (!drmp3_init_file(mp3, filePath.c_str(), nullptr))
        {
            GAME_LOG_ERROR("ERROR: Failed to open MP3 file: " + filePath);
            
            delete mp3;
            break;
        }

        currentStream_.streamHandle = mp3;
        channels = mp3->channels;
        sampleRate = mp3->sampleRate;
        currentStream_.totalSamples = (int)drmp3_get_pcm_frame_count(mp3);
        success = true;
        break;
    }
    default:
        success = false;
        break;
    }

    if (!success)
    {
        currentStream_.streamHandle = nullptr;
        return false;
    }

    currentStream_.channels = channels;
    currentStream_.sampleRate = sampleRate;
    currentStream_.samplesRead = 0;

    if (channels == 1)
        currentStream_.format = AL_FORMAT_MONO16;
    else if (channels == 2)
        currentStream_.format = AL_FORMAT_STEREO16;
    else
    {
        GAME_LOG_ERROR("ERROR: Unsupported channel count: " + std::to_string(channels));
        
        closeStream(&currentStream_);
        currentStream_.streamHandle = nullptr;
        return false;
    }

    std::uint64_t startFrameOffset = 0;
    if (startTime > 0.0f)
    {
        startFrameOffset = (std::uint64_t)std::round(startTime * sampleRate);

        if (currentStream_.totalSamples > 0 && startFrameOffset >= (std::uint64_t)currentStream_.totalSamples)
        {
            startFrameOffset = currentStream_.totalSamples - 1;
        }

        switch (currentStream_.fileType)
        {
        case FILE_TYPE_OGG:
            stb_vorbis_seek(static_cast<stb_vorbis *>(currentStream_.streamHandle), (int)startFrameOffset);
            break;
        case FILE_TYPE_WAV:
            drwav_seek_to_pcm_frame(static_cast<drwav *>(currentStream_.streamHandle), startFrameOffset);
            break;
        case FILE_TYPE_MP3:
            drmp3_seek_to_pcm_frame(static_cast<drmp3 *>(currentStream_.streamHandle), startFrameOffset);
            break;
        default:
            break;
        }
    }

    currentStream_.seekOffsetFrames = startFrameOffset;
    currentStream_.samplesRead = startFrameOffset;
    currentStream_.totalSamplesProcessed = startFrameOffset;

    alGenSources(1, &currentStream_.sourceID);
    if (currentStream_.sourceID == 0) {
        GAME_LOG_ERROR("FATAL ERROR: alGenSources failed to generate a valid source ID (returned 0).");
        stopMusicStream();
        return false;
    }

    currentStream_.buffers.resize(NUM_BUFFERS);
    alGenBuffers(NUM_BUFFERS, currentStream_.buffers.data());

    if (checkALError("loadMusicStream/Gen"))
    {
        stopMusicStream();
        return false;
    }

    alSourcef(currentStream_.sourceID, AL_PITCH, currentStream_.playbackRate);
    for (ALuint bufferID : currentStream_.buffers)
    {
        if (!streamToBuffer(bufferID, &currentStream_))
            break;
    }

    return true;
}

void AudioManager::playMusicStream()
{
    if (currentStream_.sourceID == 0 || currentStream_.isPlaying)
        return;

    alSourcef(currentStream_.sourceID, AL_GAIN, currentStream_.volume);
    alSourcei(currentStream_.sourceID, AL_LOOPING, AL_FALSE);
    alSourcef(currentStream_.sourceID, AL_PITCH, currentStream_.playbackRate);
    alSourcePlay(currentStream_.sourceID);

    currentStream_.isPlaying = true;
    checkALError("playMusicStream");
}

void AudioManager::stopMusicStream()
{
    songEndedNaturally_ = false;
    if (currentStream_.sourceID == 0)
        return;

    alSourceStop(currentStream_.sourceID);

    ALint numQueued;
    alGetSourcei(currentStream_.sourceID, AL_BUFFERS_QUEUED, &numQueued);
    if (numQueued > 0)
    {
        std::vector<ALuint> tempBuffers(numQueued);
        alSourceUnqueueBuffers(currentStream_.sourceID, numQueued, tempBuffers.data());
    }

    alDeleteSources(1, &currentStream_.sourceID);
    if (!currentStream_.buffers.empty())
    {
        alDeleteBuffers(static_cast<ALsizei>(currentStream_.buffers.size()),
                        currentStream_.buffers.data());
    }

    closeStream(&currentStream_);

    currentStream_ = MusicStream{};
}

void AudioManager::pauseMusicStream()
{
    if (currentStream_.isPlaying && currentStream_.sourceID)
    {
        alSourcePause(currentStream_.sourceID);
        currentStream_.isPlaying = false;
        checkALError("pauseMusicStream");
    }
}

void AudioManager::resumeMusicStream()
{
    if (!currentStream_.isPlaying && currentStream_.sourceID)
    {
        alSourcePlay(currentStream_.sourceID);
        currentStream_.isPlaying = true;
        checkALError("resumeMusicStream");
    }
}

void AudioManager::forceStopCrossfade()
{
    if (!isCrossfading_) return;

    if (currentStream_.sourceID) {
        alSourceStop(currentStream_.sourceID);

        ALint queued = 0;
        alGetSourcei(currentStream_.sourceID, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0) {
            ALuint buf;
            alSourceUnqueueBuffers(currentStream_.sourceID, 1, &buf);
        }
        alDeleteSources(1, &currentStream_.sourceID);
        alDeleteBuffers((ALsizei)currentStream_.buffers.size(), currentStream_.buffers.data());
    }

    closeStream(&currentStream_);

    if (nextStream_.sourceID) {
        alSourceStop(nextStream_.sourceID);
        alDeleteSources(1, &nextStream_.sourceID);
        alDeleteBuffers((ALsizei)nextStream_.buffers.size(), nextStream_.buffers.data());
    }

    closeStream(&nextStream_);

    currentStream_ = MusicStream{};
    nextStream_ = MusicStream{};
    isCrossfading_ = false;
}

bool AudioManager::switchMusicStream(const std::string &filePath, float crossfadeDuration, float startTimeSeconds)
{
    if (crossfadeDuration <= 0.0f)
    {
        stopMusicStream();
        if (loadMusicStream(filePath, startTimeSeconds))
        {
            playMusicStream();
            return true;
        }
        return false;
    }

    if (isCrossfading_)
    {
        if (nextStream_.sourceID)
        {
            alSourceStop(nextStream_.sourceID);
            alDeleteSources(1, &nextStream_.sourceID);
        }
        if (!nextStream_.buffers.empty())
        {
            alDeleteBuffers(static_cast<ALsizei>(nextStream_.buffers.size()),
                            nextStream_.buffers.data());
        }
        closeStream(&nextStream_);
        nextStream_ = MusicStream{};
    }

    std::string ext = getFileExtension(filePath);

    if (ext == "ogg")
        nextStream_.fileType = FILE_TYPE_OGG;
    else if (ext == "wav")
        nextStream_.fileType = FILE_TYPE_WAV;
    else if (ext == "mp3")
        nextStream_.fileType = FILE_TYPE_MP3;
    else
    {
        GAME_LOG_ERROR("ERROR: Unsupported stream file format: " + ext);
        return false;
    }

    bool success = false;
    unsigned int channels = 0;
    unsigned int sampleRate = 0;

    switch (nextStream_.fileType)
    {

    case FILE_TYPE_OGG:
    {
        int error = 0;
        stb_vorbis *vorbis = stb_vorbis_open_filename(filePath.c_str(), &error, nullptr);
        if (!error && vorbis)
        {
            stb_vorbis_info info = stb_vorbis_get_info(vorbis);
            nextStream_.streamHandle = vorbis;
            channels = info.channels;
            sampleRate = info.sample_rate;
            nextStream_.totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
            success = true;
        }
        else
        {
            GAME_LOG_ERROR("ERROR: Failed to open OGG file: " + filePath);
            GAME_LOG_ERROR("stb_vorbis error code: " + std::to_string(error));
        }
        break;
    }
    case FILE_TYPE_WAV:
    {
        drwav *wav = new drwav();
        if (drwav_init_file(wav, filePath.c_str(), nullptr))
        {
            nextStream_.streamHandle = wav;
            channels = wav->channels;
            sampleRate = wav->sampleRate;
            nextStream_.totalSamples = (int)wav->totalPCMFrameCount;
            success = true;
        }
        else
        {
            GAME_LOG_ERROR("ERROR: Failed to open WAV file: " + filePath);
            delete wav;
        }
        break;
    }
    case FILE_TYPE_MP3:
    {
        drmp3 *mp3 = new drmp3();
        if (drmp3_init_file(mp3, filePath.c_str(), nullptr))
        {
            nextStream_.streamHandle = mp3;
            channels = mp3->channels;
            sampleRate = mp3->sampleRate;
            nextStream_.totalSamples = (int)drmp3_get_pcm_frame_count(mp3);
            success = true;
        }
        else
        {
            GAME_LOG_ERROR("ERROR: Failed to open MP3 file: " + filePath);
            delete mp3;
        }
        break;
    }
    default:
        break;
    }

    if (!success)
    {
        nextStream_ = MusicStream{};
        return false;
    }

    nextStream_.sampleRate = sampleRate;
    nextStream_.volume = currentStream_.volume;
    nextStream_.playbackRate = currentStream_.playbackRate;

    if (channels == 1)
        nextStream_.format = AL_FORMAT_MONO16;
    else if (channels == 2)
        nextStream_.format = AL_FORMAT_STEREO16;
    else
    {
        closeStream(&nextStream_);
        nextStream_.streamHandle = nullptr;
        return false;
    }

    std::uint64_t startFrameOffset = 0;
    if (startTimeSeconds > 0.0f)
    {
        startFrameOffset = (std::uint64_t)std::round(startTimeSeconds * sampleRate);

        if (nextStream_.totalSamples > 0 && startFrameOffset >= (std::uint64_t)nextStream_.totalSamples)
        {
            startFrameOffset = nextStream_.totalSamples - 1;
        }

        switch (nextStream_.fileType)
        {
        case FILE_TYPE_OGG:
            stb_vorbis_seek(static_cast<stb_vorbis *>(nextStream_.streamHandle), (int)startFrameOffset);
            break;
        case FILE_TYPE_WAV:
            drwav_seek_to_pcm_frame(static_cast<drwav *>(nextStream_.streamHandle), startFrameOffset);
            break;
        case FILE_TYPE_MP3:
            drmp3_seek_to_pcm_frame(static_cast<drmp3 *>(nextStream_.streamHandle), startFrameOffset);
            break;
        default:
            break;
        }
    }

    const int GLITCH_SKIP_FRAMES = 100;
    std::vector<short> skipBuffer(GLITCH_SKIP_FRAMES * channels);
    std::uint64_t framesSkipped = 0;

    switch (nextStream_.fileType)
    {
    case FILE_TYPE_OGG:

        framesSkipped = stb_vorbis_get_samples_short_interleaved(
            static_cast<stb_vorbis *>(nextStream_.streamHandle),
            channels,
            skipBuffer.data(),
            GLITCH_SKIP_FRAMES * channels);
        break;
    case FILE_TYPE_WAV:
        framesSkipped = drwav_read_pcm_frames_s16(
            static_cast<drwav *>(nextStream_.streamHandle),
            GLITCH_SKIP_FRAMES,
            skipBuffer.data());
        break;
    case FILE_TYPE_MP3:
        framesSkipped = drmp3_read_pcm_frames_s16(
            static_cast<drmp3 *>(nextStream_.streamHandle),
            GLITCH_SKIP_FRAMES,
            skipBuffer.data());
        break;
    default:
        break;
    }

    nextStream_.seekOffsetFrames = startFrameOffset;
    nextStream_.samplesRead = startFrameOffset + framesSkipped;
    nextStream_.totalSamplesProcessed = startFrameOffset + framesSkipped;

    alGenSources(1, &nextStream_.sourceID);

    if (nextStream_.sourceID == 0) {
        GAME_LOG_ERROR("FATAL ERROR: alGenSources failed to generate a valid next source ID (returned 0).");
        if (!nextStream_.buffers.empty())
            alDeleteBuffers(static_cast<ALsizei>(nextStream_.buffers.size()), nextStream_.buffers.data());
        closeStream(&nextStream_);
        nextStream_ = MusicStream{};
        return false;
    }

    nextStream_.buffers.resize(NUM_BUFFERS);
    alGenBuffers(NUM_BUFFERS, nextStream_.buffers.data());

    if (checkALError("switchMusicStream/Gen"))
    {

        if (nextStream_.sourceID)
            alDeleteSources(1, &nextStream_.sourceID);
        if (!nextStream_.buffers.empty())
            alDeleteBuffers(static_cast<ALsizei>(nextStream_.buffers.size()), nextStream_.buffers.data());
        closeStream(&nextStream_);
        nextStream_ = MusicStream{};
        return false;
    }

    for (ALuint bufferID : nextStream_.buffers)
    {
        if (!streamToBuffer(bufferID, &nextStream_))
            break;
    }

    alSourcef(nextStream_.sourceID, AL_GAIN, 0.0f);
    alSourcei(nextStream_.sourceID, AL_LOOPING, AL_FALSE);
    alSourcef(nextStream_.sourceID, AL_PITCH, nextStream_.playbackRate);
    alSourcePlay(nextStream_.sourceID);
    nextStream_.isPlaying = true;

    isCrossfading_ = true;
    crossfadeProgress_ = 0.0f;
    crossfadeDuration_ = crossfadeDuration;

    return true;
}

void AudioManager::updateStream()
{
    if (isCrossfading_)
    {
        float deltaTime = 1.0f / 60.0f;
        crossfadeProgress_ += deltaTime;

        float fadeAmount = std::clamp(crossfadeProgress_ / crossfadeDuration_, 0.0f, 1.0f);

        if (currentStream_.sourceID)
        {

            float currentVolume = currentStream_.volume * (1.0f - fadeAmount);
            alSourcef(currentStream_.sourceID, AL_GAIN, std::max(0.001f, currentVolume));
        }

        if (nextStream_.sourceID)
        {
            float nextVolume = nextStream_.volume * fadeAmount;
            alSourcef(nextStream_.sourceID, AL_GAIN, nextVolume);
        }

        if (fadeAmount >= 1.0f)
        {
            if (currentStream_.sourceID)
            {
                alSourceStop(currentStream_.sourceID);

                ALint numQueued;
                alGetSourcei(currentStream_.sourceID, AL_BUFFERS_QUEUED, &numQueued);
                if (numQueued > 0)
                {
                    std::vector<ALuint> tempBuffers(numQueued);
                    alSourceUnqueueBuffers(currentStream_.sourceID, numQueued, tempBuffers.data());
                }

                alDeleteSources(1, &currentStream_.sourceID);
                if (!currentStream_.buffers.empty())
                {
                    alDeleteBuffers(static_cast<ALsizei>(currentStream_.buffers.size()),
                                    currentStream_.buffers.data());
                }
            }

            closeStream(&currentStream_);

            currentStream_ = nextStream_;
            nextStream_ = MusicStream{};

            isCrossfading_ = false;
            crossfadeProgress_ = 0.0f;
            crossfadeDuration_ = 0.0f;
        }

        updateStreamBuffers(&currentStream_);
        updateStreamBuffers(&nextStream_);
    }
    else
    {

        updateStreamBuffers(&currentStream_);
    }
}

void AudioManager::updateStreamBuffers(MusicStream *stream)
{
    if (!stream || !stream->isPlaying || stream->sourceID == 0)
        return;

    ALint buffersProcessed = 0;
    alGetSourcei(stream->sourceID, AL_BUFFERS_PROCESSED, &buffersProcessed);

    while (buffersProcessed > 0)
    {
        ALuint bufferID;
        alSourceUnqueueBuffers(stream->sourceID, 1, &bufferID);
        buffersProcessed--;

        if (stream->totalSamplesProcessed < stream->totalSamples || stream->totalSamples == 0)
        {
            stream->totalSamplesProcessed += BUFFER_SIZE_SAMPLES;
        }

        if (stream->samplesRead < stream->totalSamples || stream->totalSamples == 0)
        {
            if (!streamToBuffer(bufferID, stream))
            {

                stream->samplesRead = stream->totalSamples;
            }
        }
    }

    ALint state;
    alGetSourcei(stream->sourceID, AL_SOURCE_STATE, &state);

    if (state != AL_PLAYING)
    {
        bool needsRestart = (stream->samplesRead < stream->totalSamples || stream->totalSamples == 0);

        if (needsRestart)
        {
            if (stream == &currentStream_ && isCrossfading_) {}
            else
            {
                alSourcePlay(stream->sourceID);
                checkALError("updateStreamBuffers/Restart");
            }
        }
        else
        {
            if (stream == &currentStream_)
            {
                if (!isCrossfading_)
                {
                    songEndedNaturally_ = true;
                }
            }
        }
    }
}