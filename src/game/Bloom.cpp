// Bloom: the settings of the glow around bright things, the size of its render targets
// and the weights of its blur.
#include "game/Bloom.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game {

namespace {

// The smallest size of a render target in one direction.
constexpr int MIN_TARGET_EXTENT = 1;

} // namespace

int bloomTargetExtent(int sceneExtent) {
    // Integer division drops the rest: 1281 / 2 is 640. For a scene of one pixel it
    // gives 0, which std::max lifts back to 1.
    return std::max(MIN_TARGET_EXTENT, sceneExtent / BLOOM_DOWNSCALE);
}

std::array<float, BLOOM_BLUR_WEIGHT_COUNT> bloomBlurWeights() {
    std::array<float, BLOOM_BLUR_WEIGHT_COUNT> weights{};

    // The height of the bell at every distance, and the sum over the whole kernel. The
    // centre is read once, every other distance twice (left and right, or up and down).
    float sum = 0.0F;
    for (std::size_t distance = 0; distance < weights.size(); ++distance) {
        const auto d = static_cast<float>(distance);
        weights[distance] = std::exp(-(d * d) / (2.0F * BLOOM_BLUR_SIGMA * BLOOM_BLUR_SIGMA));
        sum += distance == 0 ? weights[distance] : 2.0F * weights[distance];
    }

    // Divided by the sum, the kernel adds up to 1.
    for (float& weight : weights) {
        weight /= sum;
    }
    return weights;
}

} // namespace game
