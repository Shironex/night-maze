# Bloom runs at half resolution on three targets with CPU weights and a hue keeping bright pass
Status: accepted (2026-10-05)
Code: src/game/Bloom.hpp, src/game/PostProcess.cpp, assets/shaders/post/bright.frag

Context: Bloom needs a threshold pass and a repeated blur, and pulsing crystals must glow without flicker.
Decision: All passes draw to RGBA16F targets of half the scene size (BLOOM_DOWNSCALE 2): one for the bright pass, two that swap for the blur. The bright pass multiplies the colour by the share of luminance above the threshold 0.8. Blur weights (radius 6, sigma 3) come from C++ as an array.
Why: I rejected a hard threshold because a pulsing crystal would switch its glow on and off, and per channel subtraction because it shifts the hue. Half size costs a quarter per pass.
Cost and revisit: Glow width follows the resolution. I reopen it if the glow looks thin at 1440p.
