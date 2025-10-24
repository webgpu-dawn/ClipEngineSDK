#include "ShaderLibrary.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

// ============================================================================
// JSON Parsing Helpers
// ============================================================================

namespace {
    // Simple JSON parser for shader configuration
    struct JsonValue {
        std::string stringValue;
        int intValue = 0;
        std::vector<JsonValue> arrayValue;
        std::map<std::string, JsonValue> objectValue;

        enum Type { String, Int, Array, Object } type;
    };

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\"");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r\"");
        return str.substr(first, last - first + 1);
    }

    JsonValue parseJsonValue(const std::string& json, size_t& pos);

    JsonValue parseJsonObject(const std::string& json, size_t& pos) {
        JsonValue obj;
        obj.type = JsonValue::Object;

        pos++; // Skip '{'
        while (pos < json.length() && json[pos] != '}') {
            // Skip whitespace
            while (pos < json.length() && std::isspace(json[pos])) pos++;
            if (json[pos] == '}') break;

            // Parse key
            if (json[pos] != '"') throw std::runtime_error("Expected '\"' for object key");
            pos++;
            size_t keyStart = pos;
            while (pos < json.length() && json[pos] != '"') pos++;
            std::string key = json.substr(keyStart, pos - keyStart);
            pos++; // Skip closing '"'

            // Skip ':' and whitespace
            while (pos < json.length() && (std::isspace(json[pos]) || json[pos] == ':')) pos++;

            // Parse value
            obj.objectValue[key] = parseJsonValue(json, pos);

            // Skip comma
            while (pos < json.length() && (std::isspace(json[pos]) || json[pos] == ',')) pos++;
        }
        pos++; // Skip '}'

        return obj;
    }

    JsonValue parseJsonArray(const std::string& json, size_t& pos) {
        JsonValue arr;
        arr.type = JsonValue::Array;

        pos++; // Skip '['
        while (pos < json.length() && json[pos] != ']') {
            while (pos < json.length() && std::isspace(json[pos])) pos++;
            if (json[pos] == ']') break;

            arr.arrayValue.push_back(parseJsonValue(json, pos));

            while (pos < json.length() && (std::isspace(json[pos]) || json[pos] == ',')) pos++;
        }
        pos++; // Skip ']'

        return arr;
    }

    JsonValue parseJsonValue(const std::string& json, size_t& pos) {
        while (pos < json.length() && std::isspace(json[pos])) pos++;

        if (json[pos] == '{') {
            return parseJsonObject(json, pos);
        } else if (json[pos] == '[') {
            return parseJsonArray(json, pos);
        } else if (json[pos] == '"') {
            JsonValue str;
            str.type = JsonValue::String;
            pos++; // Skip opening '"'
            size_t start = pos;
            while (pos < json.length() && json[pos] != '"') pos++;
            str.stringValue = json.substr(start, pos - start);
            pos++; // Skip closing '"'
            return str;
        } else if (std::isdigit(json[pos]) || json[pos] == '-') {
            JsonValue num;
            num.type = JsonValue::Int;
            size_t start = pos;
            while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '-')) pos++;
            num.intValue = std::stoi(json.substr(start, pos - start));
            return num;
        }

        throw std::runtime_error("Invalid JSON value");
    }

    JsonValue parseJson(const std::string& json) {
        size_t pos = 0;
        return parseJsonValue(json, pos);
    }
}

// ============================================================================
// Shader File Loading
// ============================================================================

std::string ShaderLibrary::loadShaderFile(const std::string& filename) {
    // Try multiple search paths
    std::vector<std::string> searchPaths = {
        "shaders/" + filename,
        "../share/clipengine/shaders/" + filename,
        "../../share/clipengine/shaders/" + filename
    };

    for (const auto& path : searchPaths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    throw std::runtime_error("Failed to load shader file: " + filename);
}

// ============================================================================
// Shader Information
// ============================================================================

std::string ShaderLibrary::getFilename(ShaderType type) {
    switch (type) {
        case ShaderType::PlanarNV12:    return "planar_nv12.wgsl";
        case ShaderType::PlanarRGBA:    return "planar_rgba.wgsl";
        case ShaderType::PlanarI420:    return "planar_i420.wgsl";
        case ShaderType::PanoramaNV12:  return "panorama_nv12.wgsl";
        case ShaderType::PanoramaRGBA:  return "panorama_rgba.wgsl";
        default:                        return "";
    }
}

std::string ShaderLibrary::getName(ShaderType type) {
    switch (type) {
        case ShaderType::PlanarNV12:    return "Planar NV12 Video Shader";
        case ShaderType::PlanarRGBA:    return "Planar RGBA Texture Shader";
        case ShaderType::PlanarI420:    return "Planar I420 Video Shader";
        case ShaderType::PanoramaNV12:  return "Panorama NV12 Shader (360°)";
        case ShaderType::PanoramaRGBA:  return "Panorama RGBA Shader (360°)";
        default:                        return "Custom Shader";
    }
}

// ============================================================================
// Binding Configuration
// ============================================================================

std::vector<ShaderBindingDesc> ShaderLibrary::getBindings(ShaderType type) {
    switch (type) {
        case ShaderType::PlanarNV12:    return createPlanarNV12Bindings();
        case ShaderType::PlanarRGBA:    return createPlanarRGBABindings();
        case ShaderType::PanoramaNV12:  return createPanoramaNV12Bindings();
        case ShaderType::PanoramaRGBA:  return createPanoramaRGBABindings();
        default:                        return {};
    }
}

std::vector<ShaderBindingDesc> ShaderLibrary::createPlanarNV12Bindings() {
    return {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        }
    };
}

std::vector<ShaderBindingDesc> ShaderLibrary::createPlanarRGBABindings() {
    return {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        }
    };
}

std::vector<ShaderBindingDesc> ShaderLibrary::createPanoramaNV12Bindings() {
    return {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 3,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Buffer,
            .bufferType = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = false,
            .minBindingSize = 16
        }
    };
}

std::vector<ShaderBindingDesc> ShaderLibrary::createPanoramaRGBABindings() {
    return {
        ShaderBindingDesc{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Sampler,
            .samplerType = wgpu::SamplerBindingType::Filtering
        },
        ShaderBindingDesc{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Texture,
            .textureSampleType = wgpu::TextureSampleType::Float,
            .textureViewDimension = wgpu::TextureViewDimension::e2D
        },
        ShaderBindingDesc{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .type = ShaderBindingDesc::Type::Buffer,
            .bufferType = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = false,
            .minBindingSize = 16
        }
    };
}

// ============================================================================
// Shader Configuration Building
// ============================================================================

ShaderConfig ShaderLibrary::buildShaderConfig(
    const std::string& name,
    const std::string& source,
    const std::vector<ShaderBindingDesc>& bindings
) {
    ShaderConfig config;
    config.name = name;

    // Both vertex and fragment shaders are in the same file
    config.vertexShaderSource = source;
    config.fragmentShaderSource = source;

    // Set bindings
    config.bindings = bindings;

    // Standard vertex attributes (position + uv)
    config.vertexAttributes = {
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0
        },
        ShaderConfig::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = sizeof(float) * 2,
            .shaderLocation = 1
        }
    };
    config.vertexStride = sizeof(float) * 4;

    return config;
}

// ============================================================================
// Primary API Implementation
// ============================================================================

ShaderConfig ShaderLibrary::loadFromFile(const std::string& jsonPath) {
    // Read JSON file
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile.is_open()) {
        throw std::runtime_error("Failed to open shader config file: " + jsonPath);
    }

    std::stringstream buffer;
    buffer << jsonFile.rdbuf();
    std::string jsonContent = buffer.str();

    // Parse JSON
    JsonValue root = parseJson(jsonContent);

    // Extract shader name
    std::string shaderName = "Custom Shader";
    if (root.objectValue.count("name")) {
        shaderName = root.objectValue.at("name").stringValue;
    }

    // Extract shader file path
    std::string shaderFile;
    if (root.objectValue.count("shader")) {
        shaderFile = root.objectValue.at("shader").stringValue;
    } else {
        throw std::runtime_error("JSON config missing 'shader' field");
    }

    // Resolve shader file path (relative to JSON file or absolute)
    fs::path jsonFilePath(jsonPath);
    fs::path shaderFilePath(shaderFile);

    if (shaderFilePath.is_relative()) {
        shaderFilePath = jsonFilePath.parent_path() / shaderFilePath;
    }

    // Load shader source
    std::ifstream shaderSourceFile(shaderFilePath);
    if (!shaderSourceFile.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + shaderFilePath.string());
    }

    std::stringstream shaderBuffer;
    shaderBuffer << shaderSourceFile.rdbuf();
    std::string shaderSource = shaderBuffer.str();

    // Build bindings: always add default sampler and texture
    std::vector<ShaderBindingDesc> bindings;

    // Default binding 0: Sampler
    bindings.push_back(ShaderBindingDesc{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Fragment,
        .type = ShaderBindingDesc::Type::Sampler,
        .samplerType = wgpu::SamplerBindingType::Filtering
    });

    // Default binding 1: Texture
    bindings.push_back(ShaderBindingDesc{
        .binding = 1,
        .visibility = wgpu::ShaderStage::Fragment,
        .type = ShaderBindingDesc::Type::Texture,
        .textureSampleType = wgpu::TextureSampleType::Float,
        .textureViewDimension = wgpu::TextureViewDimension::e2D
    });

    // Add uniform buffers from JSON config
    if (root.objectValue.count("uniforms")) {
        const auto& uniformsArray = root.objectValue.at("uniforms");
        for (const auto& uniformValue : uniformsArray.arrayValue) {
            ShaderBindingDesc uniformDesc = {};
            uniformDesc.type = ShaderBindingDesc::Type::Buffer;
            uniformDesc.visibility = wgpu::ShaderStage::Fragment;
            uniformDesc.bufferType = wgpu::BufferBindingType::Uniform;
            uniformDesc.hasDynamicOffset = false;

            if (uniformValue.objectValue.count("binding")) {
                uniformDesc.binding = uniformValue.objectValue.at("binding").intValue;
            }

            if (uniformValue.objectValue.count("size")) {
                uniformDesc.minBindingSize = uniformValue.objectValue.at("size").intValue;
            }

            bindings.push_back(uniformDesc);
        }
    }

    // Build shader config
    return buildShaderConfig(shaderName, shaderSource, bindings);
}

ShaderConfig ShaderLibrary::create(ShaderType type) {
    std::string filename = getFilename(type);
    std::string name = getName(type);
    std::vector<ShaderBindingDesc> bindings = getBindings(type);

    std::string source = loadShaderFile(filename);
    return buildShaderConfig(name, source, bindings);
}

ShaderConfig ShaderLibrary::loadCustom(
    const std::string& name,
    const std::string& filename,
    const std::vector<ShaderBindingDesc>& bindings
) {
    std::string source = loadShaderFile(filename);
    return buildShaderConfig(name, source, bindings);
}
