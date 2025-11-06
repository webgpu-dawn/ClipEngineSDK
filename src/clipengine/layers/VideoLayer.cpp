#include "VideoLayer.h"
#include <iostream>

namespace clipengine {

VideoLayer::VideoLayer() : VideoLayer(VideoFormat::NV12) {}

VideoLayer::VideoLayer(VideoFormat format) : videoFormat_(format) {
    std::cout << "[VideoLayer] Created" << std::endl;
}

VideoLayer::~VideoLayer() {
    cleanupTextures();
}

bool VideoLayer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;

    // TODO: Create WebGPU resources (textures, samplers, pipelines)
    std::cout << "[VideoLayer] Initialized (stub implementation)" << std::endl;

    return true;
}

wgpu::TextureView VideoLayer::render(float time) {
    // TODO: Render video frame to texture
    // For now, return null (no rendering)
    return nullptr;
}

void VideoLayer::update(float deltaTime) {
    // TODO: Update video frame if playing
}

glm::vec2 VideoLayer::getSize() const {
    return glm::vec2(static_cast<float>(width_), static_cast<float>(height_));
}

bool VideoLayer::loadVideo(const std::string& path) {
    // TODO: Load video using FFmpeg or other decoder
    std::cout << "[VideoLayer] loadVideo called for: " << path << std::endl;
    return false;  // Not implemented yet
}

bool VideoLayer::updateFrame(ID3D11Texture2D* texture, int arrayIndex) {
    // TODO: Update frame from D3D11 texture
    return false;
}

bool VideoLayer::updateFrame(const void* data, uint32_t width, uint32_t height, VideoFormat format) {
    width_ = width;
    height_ = height;
    videoFormat_ = format;

    // TODO: Upload frame data to WebGPU texture
    return false;
}

void VideoLayer::setVideoFormat(VideoFormat format) {
    if (videoFormat_ != format) {
        videoFormat_ = format;
        pipelineNeedsRebuild_ = true;
    }
}

void VideoLayer::setProjection(VideoProjection projection) {
    if (projection_ != projection) {
        projection_ = projection;
        pipelineNeedsRebuild_ = true;
    }
}

void VideoLayer::setRotation(float yaw, float pitch) {
    panoramaParams_.yaw = yaw;
    panoramaParams_.pitch = pitch;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::setZoom(float zoom) {
    panoramaParams_.zoom = zoom;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::setAspect(float aspect) {
    panoramaParams_.aspect = aspect;
    panoramaUniformsDirty_ = true;
}

void VideoLayer::createShaderForProjection() {
    // TODO: Create appropriate shader for current projection mode
}

void VideoLayer::updatePanoramaUniforms() {
    // TODO: Update uniform buffer with panorama parameters
    panoramaUniformsDirty_ = false;
}

void VideoLayer::cleanupTextures() {
    // TODO: Release WebGPU resources
}

} // namespace clipengine
