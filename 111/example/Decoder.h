#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
}

#include <map>
#include <functional>
#include <atomic>

using AVFrameCallback = std::function<void(AVFrame* frame)>;

class Decoder
{
public:
    void open_video(const char* file_path, AVFrameCallback callback, bool loop = true);
    void stop();

    /**
     * @brief Get video duration in seconds
     * @return Duration in seconds, or 0.0 if not available
     */
    double getDuration() const;

    /**
     * @brief Get video frame rate (fps)
     * @return Frame rate in fps, or 0.0 if not available
     */
    double getFrameRate() const;

    /**
     * @brief Get total number of frames in video
     * @return Total frame count, or 0 if not available
     */
    int64_t getTotalFrames() const;

private:
    AVFormatContext* fmt_ctx_ = nullptr;
    std::map<int, AVCodecContext*> codec_map_;
    std::atomic<bool> stopRequested_{false};
    double duration_ = 0.0;
    double frameRate_ = 0.0;
    int64_t totalFrames_ = 0;
};