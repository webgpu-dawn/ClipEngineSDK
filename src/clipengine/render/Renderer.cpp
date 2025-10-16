#include "Renderer.h"
#include "VideoRenderer.h"

namespace ClipEngine {

std::unique_ptr<IMediaRenderer> RendererFactory::createVideoRenderer() {
    return std::make_unique<VideoRenderer>();
}

} // namespace ClipEngine
