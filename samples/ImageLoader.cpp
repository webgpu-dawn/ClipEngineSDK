#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb_image.h"

#include <iostream>
#include <dxgi.h>

bool ImageLoader::loadImage(
    const std::string& filename,
    ComPtr<ID3D11Texture2D>& outTexture,
    uint32_t& outWidth,
    uint32_t& outHeight
) {
    // Create D3D11 device for texture creation
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Use default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware device
        nullptr,                    // No software rasterizer
        0,                          // No special flags
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );

    if (FAILED(hr)) {
        std::cerr << "[ImageLoader] Failed to create D3D11 device, HRESULT: 0x"
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    // Load image using stb_image
    int width, height, channels;
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4);  // Force RGBA

    if (!imageData) {
        std::cerr << "[ImageLoader] Failed to load image: " << filename << std::endl;
        std::cerr << "[ImageLoader] Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }

    std::cout << "[ImageLoader] Loaded image: " << filename << std::endl;
    std::cout << "[ImageLoader]   Size: " << width << "x" << height << std::endl;
    std::cout << "[ImageLoader]   Channels: " << channels << " (converted to RGBA)" << std::endl;

    // Create D3D11 texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4;  // RGBA = 4 bytes per pixel
    initData.SysMemSlicePitch = 0;

    hr = device->CreateTexture2D(&texDesc, &initData, outTexture.GetAddressOf());

    // Free image data
    stbi_image_free(imageData);

    if (FAILED(hr)) {
        std::cerr << "[ImageLoader] Failed to create D3D11 texture, HRESULT: 0x"
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    outWidth = width;
    outHeight = height;

    std::cout << "[ImageLoader] Texture created successfully" << std::endl;
    return true;
}
