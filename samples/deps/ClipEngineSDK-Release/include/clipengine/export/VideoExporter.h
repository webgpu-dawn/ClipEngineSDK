#pragma once

#include "../utils/Common.h"
#include "../core/OffscreenRenderer.h"
#include "../core/CompositionEngine.h"
#include <string>
#include <functional>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * @brief Video exporter for rendering compositions to video files
 *
 * VideoExporter uses OffscreenRenderer to capture frames from CompositionEngine
 * and encodes them to video using FFmpeg. Supports various codecs and formats.
 *
 * Features:
 * - Render to H.264/H.265/VP9 and other codecs
 * - Configurable resolution, frame rate, and bitrate
 * - Progress callback for UI updates
 * - Automatic color space conversion
 *
 * Example usage:
 * @code
 * VideoExporter exporter;
 * VideoExportConfig config = {
 *     .outputPath = "output.mp4",
 *     .width = 1920,
 *     .height = 1080,
 *     .fps = 60,
 *     .bitrate = 10000000,  // 10 Mbps
 *     .codec = VideoCodec::H264
 * };
 *
 * if (exporter.initialize(config, engine.getDevice())) {
 *     exporter.beginExport(&engine, 0.0f, 10.0f);  // Export 0-10 seconds
 *
 *     while (!exporter.isFinished()) {
 *         exporter.exportFrame();
 *     }
 *
 *     exporter.finalize();
 * }
 * @endcode
 */

enum class VideoCodec {
    H264,       // x264 encoder (widely compatible)
    H265,       // x265 encoder (better compression)
    VP9,        // VP9 encoder (WebM)
    ProRes,     // ProRes encoder (high quality)
    Auto        // Auto-detect based on file extension
};

enum class VideoQualityPreset {
    UltraFast,
    SuperFast,
    VeryFast,
    Faster,
    Fast,
    Medium,
    Slow,
    Slower,
    VerySlow
};

struct VideoExportConfig {
    std::string outputPath;           // Output file path
    uint32_t width = 1920;            // Video width
    uint32_t height = 1080;           // Video height
    uint32_t fps = 60;                // Frames per second
    uint32_t bitrate = 10000000;      // Bitrate in bps (10 Mbps default)
    VideoCodec codec = VideoCodec::H264;
    VideoQualityPreset preset = VideoQualityPreset::Medium;
    bool hardwareAcceleration = true; // Use hardware encoder if available
};

class VideoExporter {
public:
    VideoExporter() = default;
    ~VideoExporter();

    /**
     * @brief Initialize the video exporter
     * @param config Export configuration
     * @param device WebGPU device
     * @return true if initialization succeeded
     */
    bool initialize(const VideoExportConfig& config, wgpu::Device device);

    /**
     * @brief Begin exporting video
     * @param engine CompositionEngine to render frames from
     * @param startTime Start time in seconds
     * @param endTime End time in seconds
     * @return true if export started successfully
     */
    bool beginExport(CompositionEngine* engine, float startTime, float endTime);

    /**
     * @brief Export a single frame
     * This should be called repeatedly until isFinished() returns true
     * @return true if frame was exported successfully
     */
    bool exportFrame();

    /**
     * @brief Finalize the export (write file trailer, cleanup)
     * @return true if finalization succeeded
     */
    bool finalize();

    /**
     * @brief Check if export is finished
     */
    bool isFinished() const { return currentTime_ >= endTime_; }

    /**
     * @brief Get export progress (0.0 to 1.0)
     */
    float getProgress() const {
        if (endTime_ <= startTime_) return 1.0f;
        return (currentTime_ - startTime_) / (endTime_ - startTime_);
    }

    /**
     * @brief Get current frame number
     */
    uint64_t getCurrentFrame() const { return currentFrame_; }

    /**
     * @brief Get total frame count
     */
    uint64_t getTotalFrames() const { return totalFrames_; }

    /**
     * @brief Set progress callback
     * Called after each frame is exported with progress (0.0 to 1.0)
     */
    void setProgressCallback(std::function<void(float)> callback) {
        progressCallback_ = callback;
    }

    /**
     * @brief Cancel the export
     */
    void cancel() { cancelled_ = true; }

    /**
     * @brief Check if export was cancelled
     */
    bool isCancelled() const { return cancelled_; }

    /**
     * @brief Simplified export method that handles all the details
     *
     * This method encapsulates the entire export workflow:
     * - Initializes exporter with config
     * - Begins export
     * - Runs the export loop with frame updates
     * - Handles window events
     * - Finalizes export
     *
     * @param engine CompositionEngine to render from
     * @param config Export configuration (with duration and fps)
     * @param updateFrameCallback Callback to update video frame before each export
     * @param shouldContinueCallback Callback to check if should continue (e.g. window events)
     * @return true if export completed successfully, false if failed or cancelled
     *
     * Example usage:
     * @code
     * VideoExporter exporter;
     * VideoExportConfig config = {
     *     .outputPath = "output.mp4",
     *     .width = 1920,
     *     .height = 1080,
     *     .fps = 30,
     *     .bitrate = 20000000
     * };
     *
     * exporter.setProgressCallback([](float p) {
     *     std::cout << "Progress: " << (int)(p * 100) << "%" << std::endl;
     * });
     *
     * float duration = 10.0f;  // Export 10 seconds
     * bool success = exporter.exportVideoSimple(
     *     &engine,
     *     config,
     *     duration,
     *     [&]() { updateVideoFrame(); },      // Update frame callback
     *     [&]() { return !windowClosed; }     // Should continue callback
     * );
     * @endcode
     */
    bool exportVideoSimple(
        CompositionEngine* engine,
        const VideoExportConfig& config,
        float duration,
        std::function<void()> updateFrameCallback = nullptr,
        std::function<bool()> shouldContinueCallback = nullptr
    );

private:
    // FFmpeg context
    AVFormatContext* formatContext_ = nullptr;
    AVCodecContext* codecContext_ = nullptr;
    AVStream* videoStream_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    SwsContext* swsContext_ = nullptr;

    // Rendering context
    std::unique_ptr<OffscreenRenderer> offscreenRenderer_;
    CompositionEngine* engine_ = nullptr;
    wgpu::Device device_;

    // Export state
    VideoExportConfig config_;
    float startTime_ = 0.0f;
    float endTime_ = 0.0f;
    float currentTime_ = 0.0f;
    uint64_t currentFrame_ = 0;
    uint64_t totalFrames_ = 0;
    bool cancelled_ = false;

    // Frame buffer for pixel readback
    std::vector<uint8_t> frameBuffer_;

    // Progress callback
    std::function<void(float)> progressCallback_;

    // Helper methods
    bool initializeFFmpeg();
    bool openOutputFile();
    bool initializeEncoder();
    void cleanup();

    AVCodecID getCodecID(VideoCodec codec, const std::string& filename);
    const char* getCodecName(VideoCodec codec);
    const char* getPresetName(VideoQualityPreset preset);

    bool encodeFrame(const uint8_t* pixelData);
    bool writePacket(AVPacket* pkt);
};
