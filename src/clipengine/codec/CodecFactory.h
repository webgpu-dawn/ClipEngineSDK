#pragma once

#include "Decoder.h"
#include "Encoder.h"
#include <memory>

namespace ClipEngine {

// Codec Factory (Placeholder for future implementation)
//
// This factory will create appropriate codec instances based on the requested type

class CodecFactory {
public:
    // Create decoder for specified codec type
    static std::unique_ptr<IDecoder> createDecoder(CodecType codec);

    // Create encoder for specified codec type
    static std::unique_ptr<IEncoder> createEncoder(CodecType codec);

    // Query available codecs
    static bool isCodecSupported(CodecType codec);
    static bool isHardwareAccelAvailable(CodecType codec);
};

} // namespace ClipEngine
