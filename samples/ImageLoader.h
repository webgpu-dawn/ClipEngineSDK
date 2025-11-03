#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

/**
 * @brief Helper class for loading images and creating D3D11 textures
 *
 * Supports multiple image formats via stb_image library:
 * - JPEG (baseline and progressive)
 * - PNG (1/2/4/8/16-bit per channel)
 * - BMP (non-1bpp, non-RLE)
 * - TGA, PSD, GIF, HDR, PIC, PNM
 *
 * Images are converted to RGBA format for GPU upload.
 */
class ImageLoader {
public:
    /**
     * @brief Load an image file and create a D3D11 texture
     *
     * Creates its own D3D11 device for texture creation.
     *
     * @param filename Path to the image file (supports .jpg, .png, .bmp, .tga, .psd, .gif, .hdr, .pic, .pnm)
     * @param outTexture Output texture (RGBA format)
     * @param outWidth Output image width
     * @param outHeight Output image height
     * @return true if successful, false otherwise
     */
    static bool loadImage(
        const std::string& filename,
        ComPtr<ID3D11Texture2D>& outTexture,
        uint32_t& outWidth,
        uint32_t& outHeight
    );

private:
    ImageLoader() = delete;
};
