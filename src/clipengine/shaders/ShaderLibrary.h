#pragma once

#include "ShaderConfig.h"
#include <string>
#include <vector>
#include <map>

// ============================================================================
// Shader Type Definitions
// ============================================================================

/**
 * @brief Predefined shader types in ClipEngine
 *
 * Each shader type corresponds to a specific rendering mode and video format.
 * Shaders are loaded from .wgsl files at runtime for maximum flexibility.
 */
enum class ShaderType {
    // Planar rendering (normal video playback)
    PlanarNV12,      ///< Planar NV12 format (Y + UV planes)
    PlanarRGBA,      ///< Planar RGBA format (single texture)
    PlanarI420,      ///< Planar I420 format (Y + U + V planes)

    // Panorama rendering (360° equirectangular video)
    PanoramaNV12,    ///< 360° panorama with NV12 format
    PanoramaRGBA,    ///< 360° panorama with RGBA format

    // Little Planet rendering (stereographic projection)
    LittlePlanetNV12,  ///< Little planet effect with NV12 format
    LittlePlanetRGBA,  ///< Little planet effect with RGBA format

    // Crystal Ball rendering (inverse stereographic/fisheye)
    CrystalBallNV12,   ///< Crystal ball effect with NV12 format
    CrystalBallRGBA,   ///< Crystal ball effect with RGBA format

    // Custom shader (user-defined)
    Custom           ///< Custom shader loaded from file
};

// ============================================================================
// Shader Library - Unified Shader Management System
// ============================================================================

/**
 * @brief Central shader management system for ClipEngine
 *
 * ShaderLibrary provides a unified interface for creating, loading, and
 * configuring shaders. It handles:
 * - Shader file loading from disk
 * - Binding configuration management
 * - Shader caching and reuse
 * - Custom shader registration
 *
 * Example usage:
 * @code
 * // Load a predefined shader
 * auto config = ShaderLibrary::create(ShaderType::PanoramaNV12);
 *
 * // Get binding configuration
 * auto bindings = ShaderLibrary::getBindings(ShaderType::PlanarRGBA);
 *
 * // Load a custom shader
 * auto customConfig = ShaderLibrary::loadCustom(
 *     "MyShader",
 *     "my_shader.wgsl",
 *     customBindings
 * );
 * @endcode
 */
class ShaderLibrary {
public:
    // ========================================================================
    // Primary API - Shader Creation
    // ========================================================================

    /**
     * @brief Load shader from external JSON configuration file
     *
     * This is the primary method for loading shaders from external files.
     * The JSON file should contain shader metadata and optional uniform configuration.
     *
     * Default bindings are automatically added:
     * - Binding 0: Sampler (fragment stage)
     * - Binding 1: Texture (fragment stage)
     *
     * JSON format (minimal):
     * {
     *   "name": "Shader Name",
     *   "shader": "shader.wgsl"
     * }
     *
     * JSON format (with uniforms):
     * {
     *   "name": "Shader Name",
     *   "shader": "shader.wgsl",
     *   "uniforms": [
     *     {
     *       "binding": 2,
     *       "size": 16
     *     }
     *   ]
     * }
     *
     * @param jsonPath Path to JSON configuration file
     * @return Complete shader configuration ready to use
     * @throws std::runtime_error if file cannot be loaded or parsed
     */
    static ShaderConfig loadFromFile(const std::string& jsonPath);

    /**
     * @brief Create a shader configuration from predefined type
     *
     * This method loads built-in shaders from the SDK's shader directory.
     * For external shaders, use loadFromFile() instead.
     *
     * @param type Shader type to create
     * @return Complete shader configuration ready to use
     * @throws std::runtime_error if shader file cannot be loaded
     */
    static ShaderConfig create(ShaderType type);

    /**
     * @brief Load a custom shader from file
     *
     * Use this to create shaders not included in the standard library.
     *
     * @param name Display name for the shader
     * @param filename Shader file name (with .wgsl extension)
     * @param bindings Binding configuration for the shader
     * @return Complete shader configuration
     * @throws std::runtime_error if shader file cannot be loaded
     */
    static ShaderConfig loadCustom(
        const std::string& name,
        const std::string& filename,
        const std::vector<ShaderBindingDesc>& bindings
    );

    // ========================================================================
    // Binding Configuration
    // ========================================================================

    /**
     * @brief Get binding configuration for a shader type
     *
     * Returns the WebGPU binding layout for the specified shader type.
     * Useful when you need to inspect or manually configure bindings.
     *
     * @param type Shader type
     * @return Vector of binding descriptions
     */
    static std::vector<ShaderBindingDesc> getBindings(ShaderType type);

    /**
     * @brief Create binding configuration for planar NV12 format
     * @return Bindings: sampler + Y texture + UV texture
     */
    static std::vector<ShaderBindingDesc> createPlanarNV12Bindings();

    /**
     * @brief Create binding configuration for planar RGBA format
     * @return Bindings: sampler + texture
     */
    static std::vector<ShaderBindingDesc> createPlanarRGBABindings();

    /**
     * @brief Create binding configuration for panorama NV12 format
     * @return Bindings: sampler + Y texture + UV texture + uniforms
     */
    static std::vector<ShaderBindingDesc> createPanoramaNV12Bindings();

    /**
     * @brief Create binding configuration for panorama RGBA format
     * @return Bindings: sampler + texture + uniforms
     */
    static std::vector<ShaderBindingDesc> createPanoramaRGBABindings();

    // ========================================================================
    // Shader Information
    // ========================================================================

    /**
     * @brief Get the filename for a shader type
     * @param type Shader type
     * @return Shader filename (e.g., "panorama_nv12.wgsl")
     */
    static std::string getFilename(ShaderType type);

    /**
     * @brief Get the display name for a shader type
     * @param type Shader type
     * @return Human-readable name (e.g., "Panorama NV12 Shader")
     */
    static std::string getName(ShaderType type);

private:
    // Internal helper functions
    static std::string loadShaderFile(const std::string& filename);
    static ShaderConfig buildShaderConfig(
        const std::string& name,
        const std::string& source,
        const std::vector<ShaderBindingDesc>& bindings
    );
};
