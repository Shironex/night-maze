// StorybookRenderer: loads the layers of the storybook intro and draws its pictures.
#pragma once

#include "core/Window.hpp"
#include "game/Storybook.hpp"
#include "gfx/FrameTexture.hpp"
#include "gfx/Shader.hpp"
#include "gfx/VertexArray.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace game {

/// The OpenGL side of the storybook intro (game/Storybook.hpp): the layers of every
/// picture as textures, and the draw of one moment. What a moment shows is decided by
/// game::storybookFrame.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class StorybookRenderer {
public:
    StorybookRenderer();

    /// Loads every layer of book from folder. False when one of them cannot be loaded
    /// (the error is in the log): the classic intro is played then. Loading again
    /// replaces what was loaded.
    /// ponytail: all layers are loaded at once when the intro begins (about 8 MB of
    /// memory per 1920 x 1080 layer, a second of decoding behind the black). Load the
    /// next picture while the one before is shown if real art makes that too slow.
    bool load(const Storybook& book, const std::filesystem::path& folder);

    /// Frees the textures.
    void unload() { m_layers.clear(); }

    /// Draws the moment frame of book into the target that is bound, which is
    /// framebuffer pixels large: black, the layers of the picture back to front, and in
    /// a crossfade the layers of the next one over them. Leaves the depth test and
    /// the blending off.
    void draw(core::Size framebuffer, const Storybook& book, const StorybookFrame& frame) const;

private:
    void drawBeat(core::Size framebuffer, const Storybook& book, std::size_t beat, float progress,
                  float opacity) const;

    gfx::Shader m_shader;
    gfx::VertexArray m_vertexArray;
    // The textures, per picture and per layer, in the order of the book.
    std::vector<std::vector<std::unique_ptr<gfx::FrameTexture>>> m_layers;
};

} // namespace game
