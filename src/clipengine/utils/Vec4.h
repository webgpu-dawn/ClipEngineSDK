#pragma once

namespace clipengine {

/**
 * @brief Simple 4D vector for public API (commonly used for RGBA colors)
 *
 * This type is used in public interfaces to avoid exposing glm dependency.
 * Internally converts to/from glm::vec4 for calculations.
 */
struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vec4() = default;
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    // Convenience aliases for color access
    float& r() { return x; }
    float& g() { return y; }
    float& b() { return z; }
    float& a() { return w; }

    const float& r() const { return x; }
    const float& g() const { return y; }
    const float& b() const { return z; }
    const float& a() const { return w; }
};

} // namespace clipengine
