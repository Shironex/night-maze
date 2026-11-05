// The shader programs of the game, loaded once when the application starts.
#include "app/AppShaders.hpp"

#include "core/Paths.hpp"

namespace game {

namespace {

// Shader files, relative to the assets directory. The scene without lighting is drawn
// with the first pair, the lines of the collision boxes and spheres with the second, the
// scene with lighting per fragment with the third, with lighting per vertex with the
// fourth and the sky with the fifth. The grass has two programs that draw the same
// blades (game::GrassPath): the first has three files, because between its vertex and its
// fragment shader runs a geometry shader, and the second, the instanced one, has
// a vertex shader of its own and shares the fragment shader. The programs after them do not draw
// the scene: they draw one triangle over the whole target and share its vertex shader. The
// composite program brings the HDR picture of the scene to the window, the preview
// program makes the pictures of the framebuffer attachments for the debug UI, and the
// bright pass and blur programs are the two steps of the bloom. The shadow depth
// program draws the scene from a light into a shadow map: positions only, no colours.
constexpr const char* TEXTURED_VERTEX_SHADER_FILE = "shaders/textured.vert";
constexpr const char* TEXTURED_FRAGMENT_SHADER_FILE = "shaders/textured.frag";
constexpr const char* COLOR_VERTEX_SHADER_FILE = "shaders/color.vert";
constexpr const char* COLOR_FRAGMENT_SHADER_FILE = "shaders/color.frag";
constexpr const char* LIT_VERTEX_SHADER_FILE = "shaders/lit.vert";
constexpr const char* LIT_FRAGMENT_SHADER_FILE = "shaders/lit.frag";
constexpr const char* GOURAUD_VERTEX_SHADER_FILE = "shaders/gouraud.vert";
constexpr const char* GOURAUD_FRAGMENT_SHADER_FILE = "shaders/gouraud.frag";
constexpr const char* SKYBOX_VERTEX_SHADER_FILE = "shaders/skybox.vert";
constexpr const char* SKYBOX_FRAGMENT_SHADER_FILE = "shaders/skybox.frag";
constexpr const char* GRASS_VERTEX_SHADER_FILE = "shaders/grass.vert";
constexpr const char* GRASS_GEOMETRY_SHADER_FILE = "shaders/grass.geom";
constexpr const char* GRASS_FRAGMENT_SHADER_FILE = "shaders/grass.frag";
constexpr const char* GRASS_INSTANCED_VERTEX_SHADER_FILE = "shaders/grass_instanced.vert";
constexpr const char* FULLSCREEN_VERTEX_SHADER_FILE = "shaders/post/composite.vert";
constexpr const char* COMPOSITE_FRAGMENT_SHADER_FILE = "shaders/post/composite.frag";
constexpr const char* PREVIEW_FRAGMENT_SHADER_FILE = "shaders/post/preview.frag";
constexpr const char* BRIGHT_PASS_FRAGMENT_SHADER_FILE = "shaders/post/bright.frag";
constexpr const char* BLUR_FRAGMENT_SHADER_FILE = "shaders/post/blur.frag";
constexpr const char* SHADOW_DEPTH_VERTEX_SHADER_FILE = "shaders/shadow_depth.vert";
constexpr const char* SHADOW_DEPTH_FRAGMENT_SHADER_FILE = "shaders/shadow_depth.frag";
// The minimap has two programs. The first draws its flat shapes into the framebuffer of
// the minimap. The second puts that picture into the window: one more program that
// draws the triangle of FULLSCREEN_VERTEX_SHADER_FILE.
constexpr const char* MINIMAP_VERTEX_SHADER_FILE = "shaders/post/minimap.vert";
constexpr const char* MINIMAP_FRAGMENT_SHADER_FILE = "shaders/post/minimap.frag";
constexpr const char* MINIMAP_OVERLAY_FRAGMENT_SHADER_FILE = "shaders/post/minimap_overlay.frag";
// The reflect program draws the crystals and the puddles with the sky on them.
constexpr const char* REFLECT_VERTEX_SHADER_FILE = "shaders/reflect.vert";
constexpr const char* REFLECT_FRAGMENT_SHADER_FILE = "shaders/reflect.frag";

} // namespace

AppShaders::AppShaders()
    : texturedShader(core::assetPath(TEXTURED_VERTEX_SHADER_FILE),
                     core::assetPath(TEXTURED_FRAGMENT_SHADER_FILE)),
      colorShader(core::assetPath(COLOR_VERTEX_SHADER_FILE),
                  core::assetPath(COLOR_FRAGMENT_SHADER_FILE)),
      litShader(core::assetPath(LIT_VERTEX_SHADER_FILE), core::assetPath(LIT_FRAGMENT_SHADER_FILE)),
      gouraudShader(core::assetPath(GOURAUD_VERTEX_SHADER_FILE),
                    core::assetPath(GOURAUD_FRAGMENT_SHADER_FILE)),
      skyboxShader(core::assetPath(SKYBOX_VERTEX_SHADER_FILE),
                   core::assetPath(SKYBOX_FRAGMENT_SHADER_FILE)),
      // The geometry shader is the third argument, although it runs second: it is the
      // optional one.
      grassShader(core::assetPath(GRASS_VERTEX_SHADER_FILE),
                  core::assetPath(GRASS_FRAGMENT_SHADER_FILE),
                  core::assetPath(GRASS_GEOMETRY_SHADER_FILE)),
      grassInstancedShader(core::assetPath(GRASS_INSTANCED_VERTEX_SHADER_FILE),
                           core::assetPath(GRASS_FRAGMENT_SHADER_FILE)),
      compositeShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                      core::assetPath(COMPOSITE_FRAGMENT_SHADER_FILE)),
      previewShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                    core::assetPath(PREVIEW_FRAGMENT_SHADER_FILE)),
      brightPassShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                       core::assetPath(BRIGHT_PASS_FRAGMENT_SHADER_FILE)),
      blurShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                 core::assetPath(BLUR_FRAGMENT_SHADER_FILE)),
      shadowDepthShader(core::assetPath(SHADOW_DEPTH_VERTEX_SHADER_FILE),
                        core::assetPath(SHADOW_DEPTH_FRAGMENT_SHADER_FILE)),
      minimapShader(core::assetPath(MINIMAP_VERTEX_SHADER_FILE),
                    core::assetPath(MINIMAP_FRAGMENT_SHADER_FILE)),
      minimapOverlayShader(core::assetPath(FULLSCREEN_VERTEX_SHADER_FILE),
                           core::assetPath(MINIMAP_OVERLAY_FRAGMENT_SHADER_FILE)),
      reflectShader(core::assetPath(REFLECT_VERTEX_SHADER_FILE),
                    core::assetPath(REFLECT_FRAGMENT_SHADER_FILE)) {}

} // namespace game
