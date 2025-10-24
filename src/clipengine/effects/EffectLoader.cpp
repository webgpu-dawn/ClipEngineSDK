#include "EffectLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace clipengine {

std::unique_ptr<ShaderEffect> EffectLoader::loadFromFile(const std::string& configPath) {
    try {
        // Parse configuration file
        EffectConfig config = parseConfigFile(configPath);

        // Create shader configuration
        ShaderConfig shaderConfig = createShaderConfig(config);

        // Create effect
        auto effect = std::make_unique<ShaderEffect>(config.name, shaderConfig, config.parameters);

        // Apply default parameter values from config
        for (const auto& param : config.parameters) {
            effect->setParam(param.name, param.defaultValue);
        }

        // Enable input binding if specified
        if (config.inputBinding) {
            effect->enableInputBinding(true);
        }

        return effect;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load effect from " << configPath << ": " << e.what() << std::endl;
        return nullptr;
    }
}

EffectLoader::EffectConfig EffectLoader::parseConfigFile(const std::string& filepath) {
    EffectConfig config;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    std::string line;
    std::string currentSection;
    ShaderParam currentParam;
    bool inParameter = false;

    while (std::getline(file, line)) {
        line = trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check for section headers
        if (line[0] == '[' && line.back() == ']') {
            // Save previous parameter if any
            if (inParameter) {
                config.parameters.push_back(currentParam);
                currentParam = ShaderParam();
            }

            currentSection = line.substr(1, line.length() - 2);
            inParameter = (currentSection == "Parameter");
            continue;
        }

        // Parse key-value pairs
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        // Parse based on current section
        if (currentSection == "Effect") {
            if (key == "name") {
                config.name = value;
            }
            else if (key == "vertexShader") {
                config.vertexShaderPath = value;
            }
            else if (key == "fragmentShader") {
                config.fragmentShaderPath = value;
            }
            else if (key == "inputBinding") {
                config.inputBinding = (value == "true" || value == "1");
            }
        }
        else if (currentSection == "Parameter") {
            if (key == "name") {
                currentParam.name = value;
            }
            else if (key == "type") {
                // Type will be handled when setting default value
            }
            else if (key == "default") {
                currentParam.defaultValue = std::stof(value);
            }
            else if (key == "min") {
                currentParam.minValue = std::stof(value);
            }
            else if (key == "max") {
                currentParam.maxValue = std::stof(value);
            }
            else if (key == "description") {
                currentParam.description = value;
            }
        }
    }

    // Save last parameter if any
    if (inParameter) {
        config.parameters.push_back(currentParam);
    }

    return config;
}

std::string EffectLoader::readTextFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open shader file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string EffectLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

ShaderConfig EffectLoader::createShaderConfig(const EffectConfig& config) {
    ShaderConfig shaderConfig;
    shaderConfig.name = config.name;

    // Read shader source files
    shaderConfig.vertexShaderSource = readTextFile(config.vertexShaderPath);
    shaderConfig.fragmentShaderSource = readTextFile(config.fragmentShaderPath);

    // Set up bindings (standard layout for all effects)
    uint32_t bindingIndex = 0;

    // Binding 0: Sampler
    shaderConfig.bindings.push_back({
        bindingIndex++,
        wgpu::ShaderStage::Fragment,
        ShaderBindingDesc::Type::Sampler
    });

    // Binding 1: Input texture
    shaderConfig.bindings.push_back({
        bindingIndex++,
        wgpu::ShaderStage::Fragment,
        ShaderBindingDesc::Type::Texture,
        wgpu::SamplerBindingType::Filtering,
        wgpu::TextureSampleType::Float
    });

    // Bindings 2+: Uniform buffers (one per 4 parameters)
    size_t paramCount = config.parameters.size();
    size_t uniformBufferCount = (paramCount + 3) / 4;  // Ceil division

    for (size_t i = 0; i < uniformBufferCount; i++) {
        shaderConfig.bindings.push_back({
            bindingIndex++,
            wgpu::ShaderStage::Fragment,
            ShaderBindingDesc::Type::Buffer,
            wgpu::SamplerBindingType::Filtering,
            wgpu::TextureSampleType::Float,
            wgpu::TextureViewDimension::e2D,
            wgpu::BufferBindingType::Uniform,
            false,
            16  // vec4 size
        });
    }

    // Set up vertex attributes (standard fullscreen quad)
    shaderConfig.vertexAttributes = {
        {wgpu::VertexFormat::Float32x2, 0, 0},  // position
        {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}  // uv
    };
    shaderConfig.vertexStride = sizeof(float) * 4;

    return shaderConfig;
}

} // namespace clipengine
