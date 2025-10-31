#include "VideoExporter.h"
#include "../utils/CeLogger.h"
#include <iostream>

VideoExporter::~VideoExporter() {
    cleanup();
}

bool VideoExporter::initialize(const VideoExportConfig& config, wgpu::Device device) {
    if (!device) {
        LOG_ERROR("Invalid WebGPU device");
        return false;
    }

    config_ = config;
    device_ = device;

    // Validate config
    if (config_.outputPath.empty()) {
        LOG_ERROR("Output path is empty");
        return false;
    }

    if (config_.width == 0 || config_.height == 0) {
        LOG_ERROR("Invalid dimensions: {}x{}", config_.width, config_.height);
        return false;
    }

    if (config_.fps == 0) {
        LOG_ERROR("Invalid FPS: {}", config_.fps);
        return false;
    }

    // Initialize OffscreenRenderer
    // Use BGRA8Unorm to match the window surface format and render pipelines
    offscreenRenderer_ = std::make_unique<OffscreenRenderer>();
    if (!offscreenRenderer_->initialize(device_, wgpu::TextureFormat::BGRA8Unorm, config_.width, config_.height)) {
        LOG_ERROR("Failed to initialize OffscreenRenderer");
        return false;
    }

    // Allocate frame buffer
    frameBuffer_.resize(config_.width * config_.height * 4);

    // Initialize FFmpeg
    if (!initializeFFmpeg()) {
        LOG_ERROR("Failed to initialize FFmpeg");
        cleanup();
        return false;
    }

    LOG_INFO("VideoExporter initialized: {}x{} @ {} fps, output: {}",
             config_.width, config_.height, config_.fps, config_.outputPath);

    return true;
}

bool VideoExporter::initializeFFmpeg() {
    // Open output file
    if (!openOutputFile()) {
        return false;
    }

    // Initialize encoder
    if (!initializeEncoder()) {
        return false;
    }

    // Write header
    int ret = avformat_write_header(formatContext_, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to write format header: {}", errbuf);
        return false;
    }

    // Allocate frame
    frame_ = av_frame_alloc();
    if (!frame_) {
        LOG_ERROR("Failed to allocate AVFrame");
        return false;
    }

    frame_->format = codecContext_->pix_fmt;
    frame_->width = config_.width;
    frame_->height = config_.height;

    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to allocate frame buffer: {}", errbuf);
        return false;
    }

    // Allocate packet
    packet_ = av_packet_alloc();
    if (!packet_) {
        LOG_ERROR("Failed to allocate AVPacket");
        return false;
    }

    // Initialize swscale context for color conversion (BGRA -> YUV420P)
    // Note: BGRA8Unorm maps to AV_PIX_FMT_BGRA
    swsContext_ = sws_getContext(
        config_.width, config_.height, AV_PIX_FMT_BGRA,
        config_.width, config_.height, codecContext_->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsContext_) {
        LOG_ERROR("Failed to create SwsContext");
        return false;
    }

    return true;
}

bool VideoExporter::openOutputFile() {
    int ret = avformat_alloc_output_context2(&formatContext_, nullptr, nullptr, config_.outputPath.c_str());
    if (ret < 0 || !formatContext_) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to create output context: {}", errbuf);
        return false;
    }

    // Open output file
    if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext_->pb, config_.outputPath.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_ERROR("Failed to open output file: {}", errbuf);
            return false;
        }
    }

    return true;
}

bool VideoExporter::initializeEncoder() {
    // Get codec ID
    AVCodecID codecID = getCodecID(config_.codec, config_.outputPath);

    // Find encoder
    const AVCodec* codec = nullptr;

    if (config_.hardwareAcceleration) {
        // Try hardware encoder first
        if (codecID == AV_CODEC_ID_H264) {
            codec = avcodec_find_encoder_by_name("h264_nvenc");  // NVIDIA
            if (!codec) codec = avcodec_find_encoder_by_name("h264_qsv");   // Intel
            if (!codec) codec = avcodec_find_encoder_by_name("h264_amf");   // AMD
        } else if (codecID == AV_CODEC_ID_HEVC) {
            codec = avcodec_find_encoder_by_name("hevc_nvenc");
            if (!codec) codec = avcodec_find_encoder_by_name("hevc_qsv");
            if (!codec) codec = avcodec_find_encoder_by_name("hevc_amf");
        }

        if (codec) {
            LOG_INFO("Using hardware encoder: {}", codec->name);
        }
    }

    // Fallback to software encoder
    if (!codec) {
        codec = avcodec_find_encoder(codecID);
        if (codec) {
            LOG_INFO("Using software encoder: {}", codec->name);
        }
    }

    if (!codec) {
        LOG_ERROR("Failed to find encoder for codec ID: {}", static_cast<int>(codecID));
        return false;
    }

    // Create video stream
    videoStream_ = avformat_new_stream(formatContext_, nullptr);
    if (!videoStream_) {
        LOG_ERROR("Failed to create video stream");
        return false;
    }

    videoStream_->id = formatContext_->nb_streams - 1;

    // Allocate codec context
    codecContext_ = avcodec_alloc_context3(codec);
    if (!codecContext_) {
        LOG_ERROR("Failed to allocate codec context");
        return false;
    }

    // Set codec parameters
    codecContext_->codec_id = codecID;
    codecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext_->width = config_.width;
    codecContext_->height = config_.height;
    codecContext_->time_base = {1, static_cast<int>(config_.fps)};
    codecContext_->framerate = {static_cast<int>(config_.fps), 1};
    codecContext_->bit_rate = config_.bitrate;
    codecContext_->gop_size = config_.fps * 2;  // GOP size = 2 seconds
    codecContext_->max_b_frames = 2;
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;

    // Set quality preset
    av_opt_set(codecContext_->priv_data, "preset", getPresetName(config_.preset), 0);

    // Some formats want stream headers to be separate
    if (formatContext_->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Open codec
    int ret = avcodec_open2(codecContext_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to open codec: {}", errbuf);
        return false;
    }

    // Copy codec parameters to stream
    ret = avcodec_parameters_from_context(videoStream_->codecpar, codecContext_);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to copy codec parameters: {}", errbuf);
        return false;
    }

    videoStream_->time_base = codecContext_->time_base;

    return true;
}

bool VideoExporter::beginExport(CompositionEngine* engine, float startTime, float endTime) {
    if (!engine) {
        LOG_ERROR("Invalid CompositionEngine");
        return false;
    }

    engine_ = engine;
    startTime_ = startTime;
    endTime_ = endTime;
    currentTime_ = startTime;
    currentFrame_ = 0;
    cancelled_ = false;

    // Calculate total frames
    totalFrames_ = static_cast<uint64_t>((endTime_ - startTime_) * config_.fps);

    LOG_INFO("Beginning export: {} to {} seconds ({} frames)", startTime_, endTime_, totalFrames_);

    return true;
}

bool VideoExporter::exportFrame() {
    if (!engine_ || !offscreenRenderer_ || cancelled_) {
        return false;
    }

    if (isFinished()) {
        return false;
    }

    // Update engine to current time
    float deltaTime = 1.0f / config_.fps;
    engine_->update(deltaTime);

    // Render to offscreen texture
    engine_->render(offscreenRenderer_->getTargetView());

    // Read pixels from GPU
    if (!offscreenRenderer_->readPixelsSync(frameBuffer_.data(), frameBuffer_.size())) {
        LOG_ERROR("Failed to read pixels from GPU");
        return false;
    }

    // Encode frame
    if (!encodeFrame(frameBuffer_.data())) {
        LOG_ERROR("Failed to encode frame {}", currentFrame_);
        return false;
    }

    // Update state
    currentFrame_++;
    currentTime_ += deltaTime;

    // Call progress callback
    if (progressCallback_) {
        progressCallback_(getProgress());
    }

    return true;
}

bool VideoExporter::encodeFrame(const uint8_t* pixelData) {
    // Make frame writable
    int ret = av_frame_make_writable(frame_);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to make frame writable: {}", errbuf);
        return false;
    }

    // Convert RGBA to YUV420P using swscale
    const uint8_t* srcData[1] = { pixelData };
    int srcLinesize[1] = { static_cast<int>(config_.width * 4) };

    sws_scale(
        swsContext_,
        srcData, srcLinesize,
        0, config_.height,
        frame_->data, frame_->linesize
    );

    // Set frame PTS
    frame_->pts = currentFrame_;

    // Send frame to encoder
    ret = avcodec_send_frame(codecContext_, frame_);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to send frame to encoder: {}", errbuf);
        return false;
    }

    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            LOG_ERROR("Failed to receive packet from encoder: {}", errbuf);
            return false;
        }

        // Write packet
        if (!writePacket(packet_)) {
            av_packet_unref(packet_);
            return false;
        }

        av_packet_unref(packet_);
    }

    return true;
}

bool VideoExporter::writePacket(AVPacket* pkt) {
    // Rescale packet timestamps
    av_packet_rescale_ts(pkt, codecContext_->time_base, videoStream_->time_base);
    pkt->stream_index = videoStream_->index;

    // Write packet
    int ret = av_interleaved_write_frame(formatContext_, pkt);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_ERROR("Failed to write packet: {}", errbuf);
        return false;
    }

    return true;
}

bool VideoExporter::finalize() {
    if (!formatContext_) {
        return false;
    }

    // Flush encoder
    if (codecContext_) {
        avcodec_send_frame(codecContext_, nullptr);

        int ret = 0;
        while (ret >= 0) {
            ret = avcodec_receive_packet(codecContext_, packet_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }

            writePacket(packet_);
            av_packet_unref(packet_);
        }
    }

    // Write trailer
    av_write_trailer(formatContext_);

    LOG_INFO("Video export completed: {} frames written to {}", currentFrame_, config_.outputPath);

    cleanup();
    return true;
}

void VideoExporter::cleanup() {
    if (swsContext_) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }

    if (frame_) {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }

    if (packet_) {
        av_packet_free(&packet_);
        packet_ = nullptr;
    }

    if (codecContext_) {
        avcodec_free_context(&codecContext_);
        codecContext_ = nullptr;
    }

    if (formatContext_) {
        if (!(formatContext_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&formatContext_->pb);
        }
        avformat_free_context(formatContext_);
        formatContext_ = nullptr;
    }

    offscreenRenderer_.reset();
}

AVCodecID VideoExporter::getCodecID(VideoCodec codec, const std::string& filename) {
    if (codec == VideoCodec::Auto) {
        // Auto-detect based on file extension
        if (filename.ends_with(".mp4") || filename.ends_with(".m4v")) {
            return AV_CODEC_ID_H264;
        } else if (filename.ends_with(".mkv")) {
            return AV_CODEC_ID_H265;
        } else if (filename.ends_with(".webm")) {
            return AV_CODEC_ID_VP9;
        } else if (filename.ends_with(".mov")) {
            return AV_CODEC_ID_PRORES;
        }
        // Default to H.264
        return AV_CODEC_ID_H264;
    }

    switch (codec) {
        case VideoCodec::H264: return AV_CODEC_ID_H264;
        case VideoCodec::H265: return AV_CODEC_ID_H265;
        case VideoCodec::VP9: return AV_CODEC_ID_VP9;
        case VideoCodec::ProRes: return AV_CODEC_ID_PRORES;
        default: return AV_CODEC_ID_H264;
    }
}

const char* VideoExporter::getCodecName(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264: return "H.264";
        case VideoCodec::H265: return "H.265";
        case VideoCodec::VP9: return "VP9";
        case VideoCodec::ProRes: return "ProRes";
        case VideoCodec::Auto: return "Auto";
        default: return "Unknown";
    }
}

const char* VideoExporter::getPresetName(VideoQualityPreset preset) {
    switch (preset) {
        case VideoQualityPreset::UltraFast: return "ultrafast";
        case VideoQualityPreset::SuperFast: return "superfast";
        case VideoQualityPreset::VeryFast: return "veryfast";
        case VideoQualityPreset::Faster: return "faster";
        case VideoQualityPreset::Fast: return "fast";
        case VideoQualityPreset::Medium: return "medium";
        case VideoQualityPreset::Slow: return "slow";
        case VideoQualityPreset::Slower: return "slower";
        case VideoQualityPreset::VerySlow: return "veryslow";
        default: return "medium";
    }
}
