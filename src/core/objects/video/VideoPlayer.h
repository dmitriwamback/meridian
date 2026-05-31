//
// Created by Dmitri Wamback on 2026-05-31.
//

#ifndef MERIDIAN_VIDEOPLAYER_H
#define MERIDIAN_VIDEOPLAYER_H

#include "VideoLoader.h"
#include <vulkan/vulkan.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool load(const std::string& path);
    void play(bool loop = true);
    void stop();
    void pause();
    void resume();

    // Vulkan texture update
    void UpdateTexture(VkDevice device, VkPhysicalDevice physicalDevice,
                      VkImage image, VkDeviceMemory imageMemory);

    int getWidth() const { return loader.getWidth(); }
    int getHeight() const { return loader.getHeight(); }
    double getAudioClock() const { return audioClock.load(); }
    double getVideoPTS() const { return loader.getFramePTS(); }

    bool isPlaying() const { return isRunning.load(); }

private:
    VideoLoader loader;

    std::atomic<bool> isRunning;
    std::atomic<bool> isPaused;
    std::atomic<bool> shouldLoop;
    std::atomic<double> audioClock;

    std::thread videoThread;
    std::thread audioThread;
    std::mutex frameMutex;
    std::condition_variable frameCV;

    // OpenAL
    ALCdevice* alDevice = nullptr;
    ALCcontext* alContext = nullptr;
    ALuint alSource = 0;
    ALuint alBuffers[4];
    int bufferSamples[4];

    static const int OUT_CHANNELS = 2;
    static const int OUT_SAMPLE_RATE = 44100;
    static const int BUFFER_FRAMES = 4096;

    // Video sync
    double audioBasePTS = 0.0;
    std::mutex audioMutex;

    void videoLoop();
    void audioLoop();

    void initOpenAL();
    void cleanupOpenAL();
};

#endif // MERIDIAN_VIDEOPLAYER_H