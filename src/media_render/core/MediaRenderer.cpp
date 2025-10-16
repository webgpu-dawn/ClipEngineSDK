#include "MediaRenderer.h"
#include "../renderers/VideoRenderer.h"

namespace MediaRender {

std::unique_ptr<IMediaRenderer> RendererFactory::createVideoRenderer() {
    return std::make_unique<VideoRenderer>();
}

} // namespace MediaRender
