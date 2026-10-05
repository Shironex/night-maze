// Tests of the parts of gfx::Framebuffer that need no OpenGL context: the names of the
// formats and the text for a framebuffer status.
// See docs/modules/gfx/framebuffers.md
#include "gfx/Framebuffer.hpp"

#include <doctest/doctest.h>

#include <array>
#include <string>

// The class itself (creating, binding, resizing) needs a window and is checked by
// running the game. These functions only map constants to text.

TEST_CASE("the formats of a framebuffer have their OpenGL names") {
    CHECK(std::string(gfx::colorFormatName(gfx::ColorFormat::Rgba16F)) == "GL_RGBA16F");
    CHECK(std::string(gfx::colorFormatName(gfx::ColorFormat::Rgba8)) == "GL_RGBA8");
    CHECK(std::string(gfx::colorFormatName(gfx::ColorFormat::None)) == "none");
    CHECK(std::string(gfx::depthFormatName(gfx::DepthFormat::Depth24)) == "GL_DEPTH_COMPONENT24");
    CHECK(std::string(gfx::depthFormatName(gfx::DepthFormat::None)) == "none");
}

TEST_CASE("a framebuffer spec starts empty") {
    // An empty spec is what the constructor refuses: no size and nothing attached.
    const gfx::FramebufferSpec spec;
    CHECK(spec.width == 0);
    CHECK(spec.height == 0);
    CHECK(spec.color == gfx::ColorFormat::None);
    CHECK(spec.depth == gfx::DepthFormat::None);
}

TEST_CASE("every framebuffer status has a text of its own") {
    CHECK(std::string(gfx::framebufferStatusText(GL_FRAMEBUFFER_COMPLETE)) == "complete");

    // The ways a framebuffer can be incomplete in OpenGL 4.1.
    const std::array<GLenum, 8> failures = {
        GL_FRAMEBUFFER_UNDEFINED,
        GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,
        GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,
        GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,
        GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,
        GL_FRAMEBUFFER_UNSUPPORTED,
        GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
        GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,
    };
    const std::string unknown = gfx::framebufferStatusText(0);
    CHECK(unknown == "unknown status");

    for (std::size_t i = 0; i < failures.size(); ++i) {
        const std::string text = gfx::framebufferStatusText(failures[i]);
        CHECK_FALSE(text.empty());
        CHECK(text != "complete");
        CHECK(text != unknown);
        // No two statuses share a text: the log must tell them apart.
        for (std::size_t j = i + 1; j < failures.size(); ++j) {
            CHECK(text != std::string(gfx::framebufferStatusText(failures[j])));
        }
    }
}
