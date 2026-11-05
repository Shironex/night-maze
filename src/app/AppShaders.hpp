// The shader programs of the game, loaded once when the application starts.
#pragma once

#include "gfx/Shader.hpp"

namespace game {

/// Every shader program the frame is drawn with. The members are public: the passes use
/// them, the debug UI reloads them live. They are created from their files in the
/// constructor, in the order they are declared, so they need a live OpenGL context: the
/// application holds them as a member, created after the window and destroyed before it.
struct AppShaders {
    AppShaders();

    /// The scene without lighting and its debug views (textured models).
    gfx::Shader texturedShader;
    /// The lines of the collision boxes and spheres.
    gfx::Shader colorShader;
    /// The lit scene with lighting per fragment (Phong and Blinn-Phong).
    gfx::Shader litShader;
    /// The lit scene with lighting per vertex (Gouraud).
    gfx::Shader gouraudShader;
    /// The sky.
    gfx::Shader skyboxShader;
    /// The grass with a geometry stage, and the instanced grass.
    gfx::Shader grassShader;
    gfx::Shader grassInstancedShader;
    /// The composite pass (the HDR picture to the window), the attachment previews of the
    /// debug UI, and the bright pass and the blur passes of the bloom.
    gfx::Shader compositeShader;
    gfx::Shader previewShader;
    gfx::Shader brightPassShader;
    gfx::Shader blurShader;
    /// Draws depth only, from the view of a light: the program of the shadow pass.
    gfx::Shader shadowDepthShader;
    /// Draws the flat shapes of the minimap into its framebuffer, and puts the finished
    /// picture of the minimap into the window.
    gfx::Shader minimapShader;
    gfx::Shader minimapOverlayShader;
    /// Draws the crystals and the puddles with the sky on them: the reflection pass.
    gfx::Shader reflectShader;
};

} // namespace game
