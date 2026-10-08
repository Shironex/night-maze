# Colours are decoded on read, lit in linear HDR and encoded once in the composite shader
Status: accepted (2026-10-05)
Code: src/gfx/ColorSpace.cpp, assets/shaders/common/color.glsl, assets/shaders/post/composite.frag

Context: Until M7 the game lit sRGB numbers and wrote them to the window. An HDR buffer and a tone curve need linear values.
Decision: Colour textures are GL_SRGB8, shaders work in a linear RGBA16F buffer, and the last line of the composite shader encodes with the exact sRGB formula. The colour space is a required argument when a texture loads.
Why: I rejected GL_FRAMEBUFFER_SRGB because ImGui writes sRGB numbers to the same window and would be encoded twice, and the result depends on the window the system gives. One line in the shader is visible.
Cost and revisit: The constants exist in C++ and GLSL with no check that they match. I reopen it if the Mac shows a different brightness.
