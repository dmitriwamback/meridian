//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_VIDEOLOADER_H
#define MERIDIAN_VIDEOLOADER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <memory>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

struct VideoMetadata {
    std::string filePath;
    int width = 0;
    int height = 0;
    double duration = 0.0;
    bool hasVideo = false;
    bool hasAudio = false;
};

class VideoLoader {
public:
    VideoLoader();
    ~VideoLoader();

    // Non-copyable
    VideoLoader(const VideoLoader&) = delete;
    VideoLoader& operator=(const VideoLoader&) = delete;

    // Moveable
    VideoLoader(VideoLoader&&) = default;
    VideoLoader& operator=(VideoLoader&&) = default;

    // Load video from file
    [[nodiscard]] bool Load(const std::string& filePath);

    // Playback control
    void Play(bool loop = true);
    void Stop();

    // Query state
    [[nodiscard]] bool IsLoaded() const { return loaded; }
    [[nodiscard]] bool IsPlaying() const { return isRunning.load(); }

    // Get metadata
    [[nodiscard]] const VideoMetadata& GetMetadata() const { return metadata; }
    [[nodiscard]] int GetWidth() const { return width; }
    [[nodiscard]] int GetHeight() const { return height; }
    [[nodiscard]] float GetAspectRatio() const;
    [[nodiscard]] double GetDuration() const { return metadata.duration; }
    [[nodiscard]] double GetCurrentTime() const { return audioClock.load(); }

    // Get audio clock for sync
    [[nodiscard]] double GetAudioClock() const { return audioClock.load(); }

    // Get raw frame data (thread-safe)
    [[nodiscard]] uint8_t* GetFrameData() const;

    // Cleanup
    void Cleanup();

private:
    // Audio/Video threads
    void audioLoop();
    void videoLoop(bool loop);

    // OpenAL setup
    void InitOpenAL();
    void CleanupOpenAL();

    // Metadata
    VideoMetadata metadata;
    std::string filePath;

    // Video state
    std::atomic<bool> isRunning{false};
    std::atomic<bool> shouldLoop{false};
    std::atomic<double> audioClock{0.0};
    std::atomic<double> audioBasePTS{0.0};
    std::atomic<int64_t> totalSamplesQueued{0};
    std::atomic<int64_t> totalSamplesPlayed{0};

    std::mutex frameMutex;
    uint8_t* frameData = nullptr;
    int width = 0;
    int height = 0;
    int frameStrideBytes = 0;

    // Threads
    std::thread audioThread;
    std::thread videoThread;

    // OpenAL
    ALCdevice* alDevice = nullptr;
    ALCcontext* alContext = nullptr;
    ALuint alSource = 0;
    ALuint alBuffers[4] = {0};
    int bufferSamples[4] = {0};

    // Audio constants
    static constexpr int OUT_CHANNELS = 2;
    static constexpr int OUT_SAMPLE_RATE = 44100;
    static constexpr int BUFFER_FRAMES = 4096;
    static constexpr int BUFFER_BYTES = BUFFER_FRAMES * OUT_CHANNELS * 2;
};

#endif // MERIDIAN_VIDEOLOADER_H