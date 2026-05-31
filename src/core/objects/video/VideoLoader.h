#ifndef MERIDIAN_VIDEOLOADER_H
#define MERIDIAN_VIDEOLOADER_H

#include <string>
#include <vector>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

struct_decoded_frame {
    uint8_t* data;
    int width;
    int height;
    double pts;
};

class VideoLoader {
public:
    VideoLoader();
    ~VideoLoader();

    bool load(const std::string& path);
    void decodeNextFrame();
    bool hasFrame() const;

    uint8_t* getFrameData() const { return frameData; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getFrameStride() const { return frameStrideBytes; }
    double getFramePTS() const { return currentFramePTS; }

private:
    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwsContext* swsCtx = nullptr;

    int videoStream = -1;
    int width = 0;
    int height = 0;
    int frameStrideBytes = 0;
    uint8_t* frameData = nullptr;
    double currentFramePTS = 0.0;
    bool hasCurrentFrame = false;

    AVFrame* frame = nullptr;
    AVFrame* frameRGB = nullptr;
    AVPacket* packet = nullptr;
};

#endif // MERIDIAN_VIDEOLOADER_H