#pragma once

#include "ShaderEffect.h"
#include <string>
#include <memory>

namespace clipengine {

/**
 * @brief Loads shader effects from configuration files
 *
 * Supports loading effects from .effect configuration files that specify:
 * - Effect name
 * - Shader file paths (vertex and fragment)
 * - Parameters with defaults, ranges, and descriptions
 * - Input binding settings
 */
class EffectLoader {
public:
    /**
     * @brief Load a shader effect from a configuration file
     * @param configPath Path to .effect configuration file
     * @return Unique pointer to loaded ShaderEffect, or nullptr on error
     */
    static std::unique_ptr<ShaderEffect> loadFromFile(const std::string& configPath);

private:
    struct EffectConfig {
        std::string name;
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        bool inputBinding = false;
        std::vector<ShaderParam> parameters;
    };

    static EffectConfig parseConfigFile(const std::string& filepath);
    static std::string readTextFile(const std::string& filepath);
    static std::string trim(const std::string& str);
    static ShaderConfig createShaderConfig(const EffectConfig& config);
};

} // namespace clipengine
