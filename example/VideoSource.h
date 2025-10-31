#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <atomic>

struct ID3D11Texture2D;
class Decoder;

/**
 * @brief Frame data structure for thread-safe frame transfer
 */
struct VideoFrame {
    ID3D11Texture2D* texture = nullptr;
    int subIndex = 0;
    bool hasNewFrame = false;
};

/**
 * @brief Video source that manages video decoding and frame delivery
 *
 * This class encapsulates:
 * - Video decoder management
 * - Thread-safe frame data transfer
 * - Playback/export mode switching
 * - Frame update notifications
 *
 * Example usage:
 * @code
 * auto videoSource = std::make_unique<VideoSource>();
 * videoSource->open("video.mp4");
 *
 * // In render loop:
 * if (videoSource->hasNewFrame()) {
 *     auto frame = videoSource->getLatestFrame();
 *     videoRenderer->updateFrame(frame.texture, frame.subIndex);
 * }
 *
 * // For export:
 * videoSource->beginExportMode();
 * // ... do export ...
 * videoSource->endExportMode();
 * @endcode
 */
class VideoSource {
public:
    VideoSource();
    ~VideoSource();

    /**
     * @brief Open a video file for decoding
     * @param filePath Path to video file
     * @return true if video opened successfully
     */
    bool open(const std::string& filePath);

    /**
     * @brief Close the video source
     */
    void close();

    /**
     * @brief Check if video source is open
     */
    bool isOpen() const { return isOpen_; }

    /**
     * @brief Check if a new frame is available
     */
    bool hasNewFrame() const;

    /**
     * @brief Get the latest frame (thread-safe)
     * @return VideoFrame with texture and subIndex
     */
    VideoFrame getLatestFrame();

    /**
     * @brief Begin export mode
     *
     * In export mode:
     * - Playback decoder is paused
     * - Export decoder takes over frame delivery
     * - Ensures clean frame state for export
     */
    void beginExportMode();

    /**
     * @brief End export mode and resume playback
     */
    void endExportMode();

    /**
     * @brief Check if currently in export mode
     */
    bool isExportMode() const { return isExportMode_; }

    /**
     * @brief Wait for frames to be processed (for synchronization)
     * @param milliseconds Time to wait
     */
    void waitForFrames(int milliseconds);

    /**
     * @brief Get the file path of opened video
     */
    const std::string& getFilePath() const { return filePath_; }

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

private:
    void setupPlaybackDecoder();
    void setupExportDecoder();
    void onFrameDecoded(ID3D11Texture2D* texture, int subIndex);

    std::string filePath_;
    bool isOpen_ = false;
    bool isExportMode_ = false;

    // Dual decoder setup for playback and export
    std::unique_ptr<Decoder> playbackDecoder_;
    std::unique_ptr<Decoder> exportDecoder_;

    // Thread-safe frame data
    VideoFrame frameData_;
    mutable std::mutex frameMutex_;

    // Mode control flags
    std::atomic<bool> allowPlaybackDecoderUpdates_{true};
    std::atomic<bool> allowExportDecoderUpdates_{false};
};
