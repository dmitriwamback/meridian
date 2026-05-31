#include "VideoLoader.h"
#include <stdexcept>
#include <iostream>

VideoLoader::VideoLoader() {
    avformat_network_init();
}

VideoLoader::~VideoLoader() {
    if (packet) av_packet_free(&packet);
    if (frameRGB) av_frame_free(&frameRGB);
    if (frame) av_frame_free(&frame);
    if (swsCtx) sws_freeContext(swsCtx);
    if (codecCtx) avcodec_free_context(&codecCtx);
    if (formatCtx) avformat_close_input(&formatCtx);
    if (frameData) {
        av_free(frameData);
        frameData = nullptr;
    }
}

bool VideoLoader::load(const std::string& path) {
    formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, path.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open video: " << path << "\n";
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        return false;
    }

    for (uint32_t i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1) {
        std::cerr << "No video stream found\n";
        return false;
    }

    AVCodecParameters* codecParams = formatCtx->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec\n";
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec\n";
        return false;
    }

    width = codecCtx->width;
    height = codecCtx->height;

    frameStrideBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    frameData = (uint8_t*)av_malloc(frameStrideBytes);

    frame = av_frame_alloc();
    frameRGB = av_frame_alloc();
    packet = av_packet_alloc();

    uint8_t* buffer = (uint8_t*)av_malloc(frameStrideBytes);
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize,
                        buffer, AV_PIX_FMT_RGB24, width, height, 1);

    swsCtx = sws_getContext(width, height, codecCtx->pix_fmt,
                           width, height, AV_PIX_FMT_RGB24,
                           SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "Failed to create SwsContext\n";
        return false;
    }

    return true;
}

void VideoLoader::decodeNextFrame() {
    if (!hasCurrentFrame) {
        av_frame_unref(frame);
    }
    hasCurrentFrame = false;

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index != videoStream) {
            av_packet_unref(packet);
            continue;
        }

        if (avcodec_send_packet(codecCtx, packet) < 0) {
            av_packet_unref(packet);
            continue;
        }
        av_packet_unref(packet);

        if (avcodec_receive_frame(codecCtx, frame) < 0) {
            continue;
        }

        sws_scale(swsCtx, frame->data, frame->linesize,
                 0, height, frameRGB->data, frameRGB->linesize);

        memcpy(frameData, frameRGB->data[0], frameStrideBytes);

        if (frame->pts != AV_NOPTS_VALUE) {
            currentFramePTS = frame->pts * av_q2d(formatCtx->streams[videoStream]->time_base);
        } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            currentFramePTS = frame->best_effort_timestamp * av_q2d(formatCtx->streams[videoStream]->time_base);
        }

        hasCurrentFrame = true;
        break;
    }
}

bool VideoLoader::hasFrame() const {
    return hasCurrentFrame;
}