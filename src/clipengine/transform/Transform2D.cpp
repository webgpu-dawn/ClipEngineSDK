#include "Transform2D.h"

namespace clipengine {

glm::mat4 Transform2D::toMatrix(const glm::vec2& layerSize) const {
    glm::mat4 mat = glm::mat4(1.0f);

    // Calculate anchor offset in pixels
    glm::vec2 anchorOffset = anchor * layerSize;

    // Transform order: Anchor → Scale → Rotate → Translate

    // 1. Translate to anchor point (make anchor the origin)
    mat = glm::translate(mat, glm::vec3(-anchorOffset, 0.0f));

    // 2. Scale
    mat = glm::scale(mat, glm::vec3(scale, 1.0f));

    // 3. Rotate around anchor point
    mat = glm::rotate(mat, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));

    // 4. Translate to target position
    mat = glm::translate(mat, glm::vec3(position + anchorOffset, 0.0f));

    return mat;
}

} // namespace clipengine
