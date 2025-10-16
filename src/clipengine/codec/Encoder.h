#pragma once

#include <memory>
#include <cstdint>

namespace ClipEngine {

// Video/Audio Encoder Interface (Placeholder for future implementation)
//
// This module will handle encoding of various media formats:
// - H.264/H.265 video encoding
// - VP9/AV1 video encoding
// - AAC/Opus audio encoding
// - Hardware-accelerated encoding (NVENC, QSV, etc.)

enum class EncoderPreset {
    UltraFast,
    SuperFast,
    VeryFast,
    Faster,
    Fast,
    Medium,
    Slow,
    Slower,
    VerySlow,
    Placebo
};

struct EncoderConfig {
    CodecType codec = CodecType::Unknown;
    bool useHardwareAccel = true;
    EncoderPreset preset = EncoderPreset::Medium;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 30;
    uint32_t bitrate = 5000000; // 5 Mbps
};

class IEncoder {
public:
    virtual ~IEncoder() = default;

    virtual bool initialize(const EncoderConfig& config) = 0;
    virtual bool encode(const uint8_t* data, size_t size, int64_t timestamp) = 0;
    virtual bool flush() = 0;
    virtual CodecType getCodecType() const = 0;
};

} // namespace ClipEngine
