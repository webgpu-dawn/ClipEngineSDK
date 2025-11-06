#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace clipengine {

/**
 * @brief 2D Transform for layers
 *
 * Provides position, rotation, scale, and anchor point transformations.
 * Similar to transform controls in After Effects/Premiere Pro.
 *
 * Transform Order: Anchor → Scale → Rotate → Translate
 */
struct Transform2D {
    // Position in pixels or normalized coordinates
    glm::vec2 position = {0.0f, 0.0f};

    // Rotation in degrees (0-360)
    float rotation = 0.0f;

    // Scale factors (1.0 = 100%)
    glm::vec2 scale = {1.0f, 1.0f};

    // Anchor point in normalized coordinates (0.5, 0.5 = center)
    glm::vec2 anchor = {0.5f, 0.5f};

    // Opacity (0.0 = transparent, 1.0 = opaque)
    float opacity = 1.0f;

    /**
     * @brief Build transformation matrix
     *
     * Applies transformations in this order:
     * 1. Translate to anchor point (origin)
     * 2. Apply scale
     * 3. Apply rotation
     * 4. Translate to target position
     *
     * @param layerSize Size of the layer in pixels (for anchor point calculation)
     * @return 4x4 transformation matrix
     */
    glm::mat4 toMatrix(const glm::vec2& layerSize = {1.0f, 1.0f}) const;

    /**
     * @brief Check if this transform has been modified
     */
    bool isDirty() const {
        return dirty_;
    }

    /**
     * @brief Mark transform as clean (used after update)
     */
    void markClean() {
        dirty_ = false;
    }

    /**
     * @brief Mark transform as dirty (needs update)
     */
    void markDirty() {
        dirty_ = true;
    }

private:
    mutable bool dirty_ = true;
};

} // namespace clipengine
