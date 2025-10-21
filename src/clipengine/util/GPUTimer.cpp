#include "GPUTimer.h"
#include <iostream>
#include <iomanip>

GPUTimer::GPUTimer(wgpu::Device device)
    : device_(device) {
}

GPUTimer::~GPUTimer() {
    if (querySet_) querySet_ = nullptr;
    if (queryBuffer_) queryBuffer_ = nullptr;
    if (readbackBuffer_) readbackBuffer_ = nullptr;
}

bool GPUTimer::initialize() {
    // Check if timestamp-query feature is supported
    // Note: This requires the device to be created with timestamp-query feature enabled

    // Create query set for timestamp queries
    wgpu::QuerySetDescriptor querySetDesc = {};
    querySetDesc.type = wgpu::QueryType::Timestamp;
    querySetDesc.count = MAX_QUERIES;

    querySet_ = device_.CreateQuerySet(&querySetDesc);
    if (!querySet_) {
        std::cerr << "Failed to create timestamp query set. Make sure device has 'timestamp-query' feature enabled." << std::endl;
        return false;
    }

    // Create buffer to receive query results
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = MAX_QUERIES * sizeof(uint64_t);
    bufferDesc.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
    queryBuffer_ = device_.CreateBuffer(&bufferDesc);

    // Create readback buffer
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    readbackBuffer_ = device_.CreateBuffer(&bufferDesc);

    supported_ = true;
    return true;
}

void GPUTimer::beginQuery(wgpu::CommandEncoder encoder, const std::string& label) {
    if (!supported_ || queryIndex_ >= MAX_QUERIES - 1) return;

    TimingData timing;
    timing.label = label;
    timing.startIndex = queryIndex_++;
    timing.endIndex = 0; // Will be set in endQuery
    timing.timeMs = 0.0;

    encoder.WriteTimestamp(querySet_, timing.startIndex);

    timings_.push_back(timing);
}

void GPUTimer::endQuery(wgpu::CommandEncoder encoder) {
    if (!supported_ || timings_.empty() || queryIndex_ >= MAX_QUERIES) return;

    auto& timing = timings_.back();
    timing.endIndex = queryIndex_++;

    encoder.WriteTimestamp(querySet_, timing.endIndex);
}

void GPUTimer::resolveQueries(wgpu::CommandEncoder encoder) {
    if (!supported_ || queryIndex_ == 0) return;

    // Resolve all queries to the query buffer
    encoder.ResolveQuerySet(querySet_, 0, queryIndex_, queryBuffer_, 0);

    // Copy to readback buffer
    encoder.CopyBufferToBuffer(queryBuffer_, 0, readbackBuffer_, 0, queryIndex_ * sizeof(uint64_t));
}

void GPUTimer::readResults() {
    if (!supported_ || queryIndex_ == 0) return;

    // Map readback buffer
    bool mapped = false;
    readbackBuffer_.MapAsync(
        wgpu::MapMode::Read,
        0,
        queryIndex_ * sizeof(uint64_t),
        wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::MapAsyncStatus status, wgpu::StringView message) {
            mapped = (status == wgpu::MapAsyncStatus::Success);
        }
    );

    // Wait for mapping to complete
    while (!mapped) {
        device_.Tick();
    }

    // Read timestamp data
    const uint64_t* timestamps = static_cast<const uint64_t*>(
        readbackBuffer_.GetConstMappedRange(0, queryIndex_ * sizeof(uint64_t))
    );

    if (timestamps) {
        // Get timestamp period (nanoseconds per tick)
        // Note: This is device-specific, typically 1.0 for most GPUs
        constexpr double timestampPeriod = 1.0; // nanoseconds

        // Calculate timing for each operation
        for (auto& timing : timings_) {
            uint64_t startTime = timestamps[timing.startIndex];
            uint64_t endTime = timestamps[timing.endIndex];

            // Convert to milliseconds
            timing.timeMs = (endTime - startTime) * timestampPeriod / 1000000.0;
            results_[timing.label] = timing.timeMs;
        }
    }

    readbackBuffer_.Unmap();
}

double GPUTimer::getTime(const std::string& label) const {
    auto it = results_.find(label);
    if (it != results_.end()) {
        return it->second;
    }
    return 0.0;
}

void GPUTimer::printResults() const {
    if (results_.empty()) {
        std::cout << "No GPU timing data available." << std::endl;
        return;
    }

    std::cout << "\n=== GPU Timing Results ===" << std::endl;
    std::cout << std::left << std::setw(20) << "Operation"
              << std::right << std::setw(12) << "Time (ms)" << std::endl;
    std::cout << std::string(32, '-') << std::endl;

    for (const auto& pair : results_) {
        std::cout << std::left << std::setw(20) << pair.first
                  << std::right << std::setw(12) << std::fixed << std::setprecision(3)
                  << pair.second << std::endl;
    }

    std::cout << std::string(32, '-') << std::endl;
}

void GPUTimer::reset() {
    queryIndex_ = 0;
    timings_.clear();
    results_.clear();
}
