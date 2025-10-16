#pragma once

#include <memory>
#include <cstdint>

namespace ClipEngine {

// Video/Audio Decoder Interface (Placeholder for future implementation)
//
// This module will handle decoding of various media formats:
// - H.264/H.265 video decoding
// - VP9/AV1 video decoding
// - AAC/MP3/Opus audio decoding
// - Hardware-accelerated decoding (NVDEC, D3D11, etc.)

enum class CodecType {
    Unknown,
    H264,
    H265,
    VP9,
    AV1,
    AAC,
    MP3,
    Opus
};

struct DecoderConfig {
    CodecType codec = CodecType::Unknown;
    bool useHardwareAccel = true;
    uint32_t maxWidth = 4096;
    uint32_t maxHeight = 4096;
};

class IDecoder {
public:
    virtual ~IDecoder() = default;

    virtual bool initialize(const DecoderConfig& config) = 0;
    virtual bool decode(const uint8_t* data, size_t size, int64_t timestamp) = 0;
    virtual bool flush() = 0;
    virtual CodecType getCodecType() const = 0;
};

} // namespace ClipEngine
