#include "Layer.h"
#include "../effects/Effect.h"
#include <iostream>

namespace clipengine {

wgpu::TextureView Layer::applyEffects(wgpu::TextureView input, float time) {
    // If no effects or all effects disabled, return input unchanged
    if (effects_.empty()) {
        return input;
    }

    // TODO: Implement effect chain application
    // For now, just return input unchanged (effects are not yet implemented)

    wgpu::TextureView current = input;

    for (const auto& effect : effects_) {
        if (effect && effect->isEnabled()) {
            // TODO: Create intermediate render target
            // TODO: Apply effect from current to render target
            // TODO: current = render target
        }
    }

    return current;
}

} // namespace clipengine
