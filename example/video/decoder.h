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

using AVFrameCallback = std::function<void(AVFrame* frame)>;

class Decoder
{
public:
    void open_video(const char* file_path, AVFrameCallback callback);

private:
    AVFormatContext* fmt_ctx_ = nullptr;

    std::map<int, AVCodecContext*> codec_map_;
};