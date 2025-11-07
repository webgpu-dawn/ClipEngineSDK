#include "VideoSource.h"
#include "Decoder.h"
#include <chrono>
#include <thread>

VideoSource::VideoSource() {
}

VideoSource::~VideoSource() {
    close();
}

bool VideoSource::open(const std::string& filePath) {
    if (isOpen_) {
        close();
    }

    filePath_ = filePath;
    setupPlaybackDecoder();
    isOpen_ = true;
    return true;
}

void VideoSource::close() {
    if (!isOpen_) return;

    playbackDecoder_.reset();
    exportDecoder_.reset();

    std::lock_guard<std::mutex> lock(frameMutex_);
    frameData_ = VideoFrame{};

    isOpen_ = false;
    isExportMode_ = false;
}

bool VideoSource::hasNewFrame() const {
    std::lock_guard<std::mutex> lock(frameMutex_);
    return frameData_.hasNewFrame;
}

VideoFrame VideoSource::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frameMutex_);

    VideoFrame frame = frameData_;
    frameData_.hasNewFrame = false; // Mark as consumed

    return frame;
}

void VideoSource::setupPlaybackDecoder() {
    playbackDecoder_ = std::make_unique<Decoder>();
    playbackDecoder_->open_video(filePath_.c_str(), [this](AVFrame* frame) {
        if (!allowPlaybackDecoderUpdates_ || frame->format != AV_PIX_FMT_D3D11) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            return;
        }

        onFrameDecoded(
            (ID3D11Texture2D*)frame->data[0],
            (int)(intptr_t)frame->data[1]
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    });
}

void VideoSource::setupExportDecoder() {
    if (exportDecoder_) return;

    exportDecoder_ = std::make_unique<Decoder>();
    // Export decoder should loop to ensure we always have frames available
    // The export duration is controlled by VideoExporter, not by decoder
    exportDecoder_->open_video(filePath_.c_str(), [this](AVFrame* frame) {
        if (!allowExportDecoderUpdates_ || frame->format != AV_PIX_FMT_D3D11) {
            return;
        }

        onFrameDecoded(
            (ID3D11Texture2D*)frame->data[0],
            (int)(intptr_t)frame->data[1]
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }, true);  // loop = true to ensure continuous frame availability
}

void VideoSource::onFrameDecoded(ID3D11Texture2D* texture, int subIndex) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    frameData_.texture = texture;
    frameData_.subIndex = subIndex;
    frameData_.hasNewFrame = true;
}

void VideoSource::beginExportMode() {
    if (isExportMode_) return;

    isExportMode_ = true;

    // Pause playback decoder
    allowPlaybackDecoderUpdates_ = false;
    waitForFrames(100);

    // Clear stale frame data from playback decoder to prevent crashes
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_ = VideoFrame{};
    }

    // Setup and start export decoder
    allowExportDecoderUpdates_ = true;
    setupExportDecoder();
    waitForFrames(200);
}

void VideoSource::endExportMode() {
    if (!isExportMode_) return;

    // Stop export decoder
    allowExportDecoderUpdates_ = false;
    waitForFrames(200);

    // Clear frame data
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        frameData_ = VideoFrame{};
    }

    // Resume playback decoder
    allowPlaybackDecoderUpdates_ = true;
    waitForFrames(100);

    isExportMode_ = false;
}

void VideoSource::waitForFrames(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

double VideoSource::getDuration() const {
    if (playbackDecoder_) {
        return playbackDecoder_->getDuration();
    }
    return 0.0;
}

double VideoSource::getFrameRate() const {
    if (playbackDecoder_) {
        return playbackDecoder_->getFrameRate();
    }
    return 0.0;
}
