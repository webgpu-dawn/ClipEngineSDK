#pragma once

namespace clipengine {

/**
 * @brief Simple 3D vector for public API
 *
 * This type is used in public interfaces to avoid exposing glm dependency.
 * Internally converts to/from glm::vec3 for calculations.
 */
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

} // namespace clipengine
