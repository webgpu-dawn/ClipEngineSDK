#include <webgpu/webgpu.h>
#include <iostream>

int main(int argc, char** argv)
{
    // Create WebGPU instance
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);

    // Check WebGPU instance
    if(!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    std::cout << "WebGPU instance : " << instance << std::endl;
    
    // Destroy WebGPU instance
    wgpuInstanceRelease(instance);
    return 0;
}