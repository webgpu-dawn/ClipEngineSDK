#include "RHIHelper.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace RHI {

wgpu::ShaderModule Helper::createShaderFromFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    return createShader(code.c_str(), path);
}

} // namespace RHI
