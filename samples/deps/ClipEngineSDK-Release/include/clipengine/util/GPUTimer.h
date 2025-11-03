#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * @brief Simple GPU timer using WebGPU timestamp queries
 *
 * Measures actual GPU execution time for operations like copy and render.
 */
class GPUTimer {
public:
    explicit GPUTimer(wgpu::Device device);
    ~GPUTimer();

    // Initialize timestamp query support
    bool initialize();

    // Begin timing a GPU operation
    void beginQuery(wgpu::CommandEncoder encoder, const std::string& label);

    // End timing a GPU operation
    void endQuery(wgpu::CommandEncoder encoder);

    // Resolve timestamp queries and read results
    void resolveQueries(wgpu::CommandEncoder encoder);

    // Get timing results (in milliseconds)
    void readResults();

    // Get the last timing for a specific label
    double getTime(const std::string& label) const;

    // Print all timing results
    void printResults() const;

    // Reset all queries
    void reset();

    bool isSupported() const { return supported_; }

private:
    wgpu::Device device_;
    bool supported_ = false;

    static constexpr uint32_t MAX_QUERIES = 256;
    uint32_t queryIndex_ = 0;

    wgpu::QuerySet querySet_;
    wgpu::Buffer queryBuffer_;
    wgpu::Buffer readbackBuffer_;

    struct TimingData {
        std::string label;
        uint32_t startIndex;
        uint32_t endIndex;
        double timeMs;
    };

    std::vector<TimingData> timings_;
    std::unordered_map<std::string, double> results_;
};
