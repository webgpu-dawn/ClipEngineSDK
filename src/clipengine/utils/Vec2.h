#pragma once

namespace clipengine {

/**
 * @brief Simple 2D vector for public API
 *
 * This type is used in public interfaces to avoid exposing glm dependency.
 * Internally converts to/from glm::vec2 for calculations.
 */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

} // namespace clipengine
