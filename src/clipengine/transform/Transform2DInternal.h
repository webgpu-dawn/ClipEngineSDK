#pragma once

#include "Transform2D.h"
#include <glm/glm.hpp>

namespace clipengine {

/**
 * @brief Internal helper to convert Transform2D to glm::mat4
 *
 * This function is for internal use by the rendering system only.
 * Not exposed in the public SDK API.
 *
 * @param transform The 2D transform to convert
 * @param layerSize Size of the layer in pixels (for anchor point calculation)
 * @return Transformation matrix
 */
inline glm::mat4 Transform2DToMatrix(const Transform2D& transform, const Vec2& layerSize = {1.0f, 1.0f}) {
    glm::mat4 mat = glm::mat4(1.0f);

    // Convert to glm types for calculations
    glm::vec2 glmPosition = glm::vec2(transform.position.x, transform.position.y);
    glm::vec2 glmScale = glm::vec2(transform.scale.x, transform.scale.y);
    glm::vec2 glmAnchor = glm::vec2(transform.anchor.x, transform.anchor.y);
    glm::vec2 glmLayerSize = glm::vec2(layerSize.x, layerSize.y);

    // Calculate anchor offset in pixels
    glm::vec2 anchorOffset = glmAnchor * glmLayerSize;

    // Transform order: Anchor → Scale → Rotate → Translate

    // 1. Translate to anchor point (make anchor the origin)
    mat = glm::translate(mat, glm::vec3(-anchorOffset, 0.0f));

    // 2. Scale
    mat = glm::scale(mat, glm::vec3(glmScale, 1.0f));

    // 3. Rotate around anchor point
    mat = glm::rotate(mat, glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // 4. Translate to target position
    mat = glm::translate(mat, glm::vec3(glmPosition + anchorOffset, 0.0f));

    return mat;
}

} // namespace clipengine
