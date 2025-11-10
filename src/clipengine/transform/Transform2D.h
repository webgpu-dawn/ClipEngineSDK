#pragma once

#include "clipengine/utils/Vec2.h"

namespace clipengine {

/**
 * @brief 2D Transform for layers
 *
 * Provides position, rotation, scale, and anchor point transformations.
 * Similar to transform controls in After Effects/Premiere Pro.
 *
 * Transform Order: Anchor → Scale → Rotate → Translate
 *
 * Note: Internal matrix conversion is handled internally by the rendering system.
 * Users only need to set position, rotation, scale, and other public properties.
 */
struct Transform2D {
    // Position in pixels or normalized coordinates
    Vec2 position = {0.0f, 0.0f};

    // Rotation in degrees (0-360)
    float rotation = 0.0f;

    // Scale factors (1.0 = 100%)
    Vec2 scale = {1.0f, 1.0f};

    // Anchor point in normalized coordinates (0.5, 0.5 = center)
    Vec2 anchor = {0.5f, 0.5f};

    // Opacity (0.0 = transparent, 1.0 = opaque)
    float opacity = 1.0f;

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
