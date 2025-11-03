#include "Decoder.h"
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>

void Decoder::open_video(const char* file_path, AVFrameCallback callback, bool loop)
{
    stopRequested_ = false;
    std::thread t([=, this](){
        int res = avformat_open_input(&fmt_ctx_, file_path, nullptr, nullptr);
        avformat_find_stream_info(fmt_ctx_, NULL);

        // Get video duration in seconds
        if (fmt_ctx_->duration != AV_NOPTS_VALUE) {
            duration_ = (double)fmt_ctx_->duration / AV_TIME_BASE;
        }

        int v_stream_idx = 0;
        int a_stream_idx = 0;
        AVCodecContext*  v_codec_ctx;
        AVCodecContext*  a_codec_ctx;

        double avg_frame_rate;
        
        for(int i = 0; i < fmt_ctx_->nb_streams; i++) {
            auto stream = fmt_ctx_->streams[i];
            AVCodec* codec = (AVCodec*)avcodec_find_decoder(stream->codecpar->codec_id);
            switch(codec->type) {
                case AVMEDIA_TYPE_VIDEO:
                    {
                        v_stream_idx = i;
                        avg_frame_rate = (double)stream->avg_frame_rate.den / stream->avg_frame_rate.num;

                        // Get frame rate (fps)
                        if (stream->avg_frame_rate.den != 0) {
                            frameRate_ = (double)stream->avg_frame_rate.num / stream->avg_frame_rate.den;
                        }

                        // Get total number of frames
                        if (stream->nb_frames > 0) {
                            totalFrames_ = stream->nb_frames;
                        } else if (duration_ > 0.0 && frameRate_ > 0.0) {
                            // Estimate from duration and frame rate
                            totalFrames_ = static_cast<int64_t>(duration_ * frameRate_);
                        }

                        v_codec_ctx = avcodec_alloc_context3(codec);
                        avcodec_parameters_to_context(v_codec_ctx, stream->codecpar);
                        avcodec_open2(v_codec_ctx, codec, NULL);

                        codec_map_[i] = v_codec_ctx;

                        AVBufferRef* hw_device_ctx = nullptr;
                        av_hwdevice_ctx_create(&hw_device_ctx, AVHWDeviceType::AV_HWDEVICE_TYPE_D3D11VA, NULL, NULL, NULL);
                        if(hw_device_ctx) {
                            v_codec_ctx->hw_device_ctx = hw_device_ctx;
                        }else {
                        }
                    }
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    break;
                default:
                    break;
            }
        }

        int width = v_codec_ctx->width;
        int height= v_codec_ctx->height;


        AVPacket* pkt = av_packet_alloc();

        do {
            // Read frames from video
            while(av_read_frame(fmt_ctx_, pkt) >= 0) {
                if (stopRequested_) {
                    break;
                }

                auto codec_ctx = codec_map_[pkt->stream_index];
                if(pkt->stream_index == v_stream_idx && avcodec_send_packet(codec_ctx, pkt) == 0) {
                    AVFrame* frame = av_frame_alloc();
                    if(avcodec_receive_frame(codec_ctx, frame) == 0 && callback) {
                        callback(frame);
                    }
                    av_frame_free(&frame);
                }
                av_packet_unref(pkt);
            }

            // If looping is enabled and not stopped, seek back to start
            if (loop && !stopRequested_) {
                // Flush codec buffers before seeking
                avcodec_flush_buffers(v_codec_ctx);

                // Seek to beginning
                av_seek_frame(fmt_ctx_, v_stream_idx, 0, AVSEEK_FLAG_BACKWARD);
            }
        } while (loop && !stopRequested_);

        av_packet_free(&pkt);

        for(auto& kv : codec_map_) {
            avcodec_free_context(&kv.second);
        }
        
        avcodec_free_context(&v_codec_ctx);
        avformat_close_input(&fmt_ctx_);
    });
    t.detach();
}

void Decoder::stop() {
    stopRequested_ = true;
}

double Decoder::getDuration() const {
    return duration_;
}

double Decoder::getFrameRate() const {
    return frameRate_;
}

int64_t Decoder::getTotalFrames() const {
    return totalFrames_;
}