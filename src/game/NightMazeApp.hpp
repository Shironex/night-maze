// The Night Maze application: game state and rendering of a frame.
// See docs/modules/core/README.md
#pragma once

#include "assets/AssetCache.hpp"
#include "core/Application.hpp"
#include "game/ColliderLines.hpp"
#include "game/GameplayRenderer.hpp"
#include "game/Grass.hpp"
#include "game/GrassRenderer.hpp"
#include "game/LightRig.hpp"
#include "game/Lighting.hpp"
#include "game/MazeRenderer.hpp"
#include "game/MazeWorld.hpp"
#include "game/Minimap.hpp"
#include "game/MinimapRenderer.hpp"
#include "game/Player.hpp"
#include "game/PostProcess.hpp"
#include "game/Round.hpp"
#include "game/ShadowMap.hpp"
#include "game/Shadows.hpp"
#include "game/Skybox.hpp"
#include "game/Terrain.hpp"
#include "game/TerrainRenderer.hpp"
#include "gfx/Shader.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"
#include "scene/LightSpace.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace game {

/// The game itself: a generated maze of textured walls and pillars at night, standing
/// on gently uneven ground that rises into hills around it (a heightmap terrain), and
/// a player who walks through it in first person without passing through the walls.
/// The mouse turns the camera, the keyboard moves the player. The maze is lit by the
/// moon, by the flashlight of the player and by the glowing crystals. Grass grows along
/// the walls, and above them is the night sky, a skybox.
///
/// The moon and the flashlight cast shadows. Before the scene, everything that casts
/// one is drawn twice into a depth texture (game::ShadowMap): from the direction of the
/// moon and from the hand that holds the flashlight. The lit programs look every
/// fragment up in both.
///
/// The scene is not drawn into the window directly. It is drawn into an HDR framebuffer
/// (game::PostProcess). Its bright parts are blurred into a glow (bloom), and a last
/// pass brings the picture to the window with fog near the ground, that glow, exposure,
/// tone mapping, a vignette and gamma correction.
///
/// On top of that finished picture comes the minimap (game::MinimapRenderer): a schematic
/// of the corridors the player has discovered, drawn into a framebuffer of its own and
/// shown in a corner of the window.
///
/// A round: the player collects crystals, each one charges the battery of the
/// flashlight, and when enough of them are collected the gate of the exit opens.
/// Walking through it wins the round. The rules are in game/Round.hpp, this class feeds
/// them the position of the player and draws their state.
///
/// It knows nothing about the debug UI: main.cpp derives from this class and draws the
/// debug panels and the HUD on top of the frame.
class NightMazeApp : public core::Application {
public:
    /// Creates the window (through core::Application), loads the shaders and the models,
    /// generates the first maze and starts the first round in it.
    NightMazeApp();

protected:
    void onUpdate(double fixedDt) override;
    void onRender(double alpha) override;

    /// Clear color (red, green, blue, as sRGB values), exposed so the debug UI can edit
    /// it live. It is the background only where the sky is not drawn: with the skybox
    /// switched off.
    std::array<float, 3>& clearColor() { return m_clearColor; }

    /// Shader program of the scene without lighting and of its debug views (textured
    /// models), exposed so the debug UI can reload it live.
    gfx::Shader& texturedShader() { return m_texturedShader; }

    /// Shader program of the lines of the collision boxes and spheres, exposed for the
    /// same reason.
    gfx::Shader& colorShader() { return m_colorShader; }

    /// Shader program of the lit scene with lighting per fragment (Phong and
    /// Blinn-Phong), exposed for the same reason.
    gfx::Shader& litShader() { return m_litShader; }

    /// Shader program of the lit scene with lighting per vertex (Gouraud), exposed for
    /// the same reason.
    gfx::Shader& gouraudShader() { return m_gouraudShader; }

    /// Shader program of the sky, exposed for the same reason.
    gfx::Shader& skyboxShader() { return m_skyboxShader; }

    /// Shader program of the grass (with a geometry stage), exposed for the same reason.
    gfx::Shader& grassShader() { return m_grassShader; }

    /// Shader program of the composite pass (the HDR picture to the window), exposed
    /// for the same reason.
    gfx::Shader& compositeShader() { return m_compositeShader; }

    /// Shader program of the attachment previews of the debug UI, exposed for the same
    /// reason.
    gfx::Shader& previewShader() { return m_previewShader; }

    /// Shader program of the bright pass of the bloom, exposed for the same reason.
    gfx::Shader& brightPassShader() { return m_brightPassShader; }

    /// Shader program of the blur passes of the bloom, exposed for the same reason.
    gfx::Shader& blurShader() { return m_blurShader; }

    /// Shader program of the depth pass of the shadow maps, exposed for the same reason.
    gfx::Shader& shadowDepthShader() { return m_shadowDepthShader; }

    /// Shader program that draws the minimap into its framebuffer, exposed for the same
    /// reason.
    gfx::Shader& minimapShader() { return m_minimapShader; }

    /// Shader program that puts the picture of the minimap into the window, exposed for
    /// the same reason.
    gfx::Shader& minimapOverlayShader() { return m_minimapOverlayShader; }

    /// The settings of the minimap (switch, reveal all, size, margin, corner, opacity),
    /// exposed so the debug UI can edit them live.
    MinimapSettings& minimapSettings() { return m_minimapSettings; }

    /// The minimap, read only: the debug UI shows the size, the format and the picture
    /// of its framebuffer.
    const MinimapRenderer& minimapRenderer() const { return m_minimapRenderer; }

    /// The settings of the shadows of the moon (switch, resolution, bias, PCF,
    /// strength), exposed so the debug UI can edit them live.
    ShadowSettings& moonShadowSettings() { return m_moonShadow; }

    /// The shadow map of the moon, read only: the debug UI shows its size, its format
    /// and its preview picture.
    const ShadowMap& moonShadowMap() const { return m_moonShadowMap; }

    /// The view and the projection the shadow map of the moon was drawn with in the
    /// last frame, read only: the debug UI shows how much ground the map covers.
    const scene::LightSpace& moonLightSpace() const { return m_moonLightSpace; }

    /// The settings of the shadows of the flashlight, exposed like the ones of the
    /// moon.
    ShadowSettings& flashlightShadowSettings() { return m_flashlightShadow; }

    /// The shadow map of the flashlight, read only, like the one of the moon.
    const ShadowMap& flashlightShadowMap() const { return m_flashlightShadowMap; }

    /// The view and the projection the shadow map of the flashlight was drawn with in
    /// the last frame, read only: the debug UI shows how much the map covers.
    const scene::LightSpace& flashlightLightSpace() const { return m_flashlightLightSpace; }

    /// Whether the shadow map of the flashlight was drawn in the last frame. It is not
    /// with its shadows switched off, and not while the flashlight itself is off (key
    /// F or an empty battery). The debug UI then shows no picture of the map.
    bool flashlightShadowDrawn() const { return m_flashlightShadowDrawn; }

    /// The exposure, the tone mapping and the preview switch of the composite pass and
    /// the settings of the bloom, the fog and the vignette, exposed so the debug UI can
    /// edit them live.
    PostProcessSettings& postProcessSettings() { return m_postProcessSettings; }

    /// The framebuffers of the frame, read only: the debug UI shows their sizes, their
    /// formats and pictures of their attachments.
    const PostProcess& postProcess() const { return m_postProcess; }

    /// The height scale and the wireframe switch of the terrain, exposed so the debug UI
    /// can edit them live.
    TerrainSettings& terrainSettings() { return m_terrainSettings; }

    /// The switch, the density, the blade height and the wind of the grass, exposed so
    /// the debug UI can edit them live.
    GrassSettings& grassSettings() { return m_grassSettings; }

    /// How many tufts of grass are on the graphics card, for the debug UI.
    std::size_t grassTuftCount() const { return m_grassRenderer.tuftCount(); }

    /// The switch and the brightness of the sky, exposed so the debug UI can edit them
    /// live.
    SkyboxSettings& skyboxSettings() { return m_skyboxSettings; }

    /// The settings of the lighting (mode, moon, flashlight, point lights, highlight,
    /// normal mapping), exposed so the debug UI can edit them live.
    LightingSettings& lighting() { return m_lighting; }

    /// The camera, exposed so the debug UI can show and edit its angles and projection
    /// live. Its position follows the eyes of the player, see player().
    scene::Camera& camera() { return m_camera; }

    /// Mouse look sensitivity in degrees per screen coordinate unit of mouse movement,
    /// exposed so the debug UI can edit it live.
    float& mouseSensitivity() { return m_mouseSensitivity; }

    /// The player, exposed so the debug UI can show and edit its position, its speeds
    /// and the noclip mode live.
    Player& player() { return m_player; }

    /// The request for the next maze, exposed so the debug UI can ask for a new one.
    MazeSettings& mazeSettings() { return m_mazeSettings; }

    /// The maze in play, read only: the debug UI draws its plan and counts its boxes.
    const MazeWorld& mazeWorld() const { return m_mazeWorld; }

    /// The numbers of the rules of a round and the request for a restart, exposed so
    /// the debug UI can edit them live.
    GameplaySettings& gameplaySettings() { return m_gameplay; }

    /// The round in play, exposed so the HUD can show it and the debug UI can set the
    /// charge of the battery.
    Round& round() { return m_round; }

    /// The loaded models and textures, exposed so the debug UI can list them and change
    /// the texture filtering live.
    assets::AssetCache& assets() { return m_assets; }

    /// What the textured shader shows, exposed so the debug UI can switch it live.
    ViewMode& viewMode() { return m_viewMode; }

    /// Whether the collision boxes and spheres are drawn as lines, exposed so the debug
    /// UI can switch it live.
    bool& drawColliders() { return m_drawColliders; }

private:
    // Camera turn for one screen coordinate unit of mouse movement, in degrees. The mouse
    // is measured in the units of the window size, not in framebuffer pixels, so the same
    // hand movement turns the camera equally on a Retina display.
    static constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1F;

    /// Builds the maze described by m_mazeSettings on its terrain and starts a round in
    /// it (beginRound).
    void regenerateMaze();

    /// Builds the terrain of the maze in play again with the height scale of
    /// m_terrainSettings and puts everything back on it: the walls, the gate, the
    /// crystals of the round, the obstacle list and the walking player. The round goes
    /// on: nothing is collected or reset.
    void rebuildTerrain();

    /// Copies what stands on the ground of m_mazeWorld to the graphics card: the mesh of
    /// the terrain and the points of the grass (plantGrass).
    void uploadGround();

    /// Chooses the places of the grass tufts again (game::placeGrass) and copies them to
    /// the graphics card.
    void plantGrass();

    /// Starts a round on the maze m_mazeWorld holds: every crystal back in its place,
    /// a full battery with the flashlight on, the gate closed and the player at the
    /// start, looking down an open passage. It runs for the first maze, after every
    /// regeneration and when the round is restarted (key R or the debug UI).
    void beginRound();

    /// The shadow pass of the moon, the first pass of a frame. It fits the box of the
    /// moon to the land (m_moonLightSpace), draws the shadow casters into the shadow
    /// map (drawShadowCasters), binds the map for the lit programs and, while the
    /// debug UI asks for it, draws its preview picture. With the shadows switched off
    /// it only computes the box. It leaves a framebuffer of the shadow map bound, so
    /// the scene framebuffer has to be bound after it.
    void drawMoonShadowMap();

    /// The shadow pass of the flashlight, right after the one of the moon. It builds
    /// the pyramid the flashlight looks along (m_flashlightLightSpace) from flashlight,
    /// the place and the direction of the light in this frame, and from the cone and
    /// the range of frameLighting, the lighting of this frame (game::lightingForFrame).
    /// Then it draws the shadow casters into the shadow map of the flashlight, binds
    /// the map for the lit programs and, while the debug UI asks for it, draws its
    /// preview picture. With the shadows of the flashlight switched off, or with the
    /// flashlight itself off in this frame, it only computes the pyramid. Like
    /// drawMoonShadowMap it leaves a framebuffer of its own bound.
    void drawFlashlightShadowMap(const LightingSettings& frameLighting,
                                 const FlashlightPose& flashlight);

    /// Draws everything that casts a shadow with the depth program, as the light with
    /// the given view and projection sees it: the terrain, the walls and the pillars,
    /// the gate (as far as it has sunk) and the crystals. The grass casts no shadow.
    /// The target (a shadow map) must be bound already.
    void drawShadowCasters(const scene::LightSpace& lightSpace) const;

    /// Sets the uniforms of both shadow maps (the moon and the flashlight) in the
    /// program shader, which must be in use and must include common/shadows.glsl:
    /// game::setShadowUniforms once for each map. Called in every frame for the lit
    /// program in use and for the grass program, also with the shadows switched off.
    void setShadowUniformsOf(const gfx::Shader& shader) const;

    /// The parts of a frame. Each one selects its own shader program and sets its
    /// uniforms. drawMaze draws the terrain and the maze together with the crystals and
    /// the gate, and has two ways to do it: without lighting (the textured program, also
    /// used for the debug views of the normals and the texture coordinates) and with
    /// lighting. drawGrass draws the grass with its own program.
    void drawMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawGrass(const glm::mat4& view, const glm::mat4& projection) const;
    void drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const;

    /// The light the crystals give off by themselves at this moment, as a linear colour
    /// for the uniform uEmissive: game::crystalGlow of the colour of the crystal lights.
    glm::vec3 crystalEmissive() const;

    /// The minimap, the last thing of a frame the game draws: after the composite pass,
    /// so the fog, the bloom and the tone mapping do not touch it. It builds the shapes
    /// of the map (game::buildMinimapVertices), draws them into the framebuffer of the
    /// minimap and puts that picture into its corner of the window. framebuffer is the
    /// framebuffer size of the window and feet the place the frame is drawn from (the
    /// feet of the player, blended between two fixed steps). With the minimap switched
    /// off it does nothing. It leaves the window bound, with the viewport over all of it.
    void drawMinimap(core::Size framebuffer, const glm::vec3& feet);

    // The colour every frame starts with: a very dark blue, darker than the ambient
    // light on the stone. The sky is drawn over it wherever no wall is, so it shows
    // only when the skybox is switched off or its pictures could not be loaded. It is
    // close to the colour of the sky straight above, so switching the skybox off does
    // not change the mood of the scene. An sRGB value, as the colour picker of the
    // debug UI shows it: onRender converts it to linear for the HDR buffer.
    std::array<float, 3> m_clearColor{0.022F, 0.033F, 0.088F};

    // OpenGL objects. They are members of a class derived from core::Application, so they
    // are created after the window and its OpenGL context, and destroyed before them.
    //
    // Order: members are constructed top to bottom. The renderers of the maze, the
    // round and the terrain ask m_assets for their models and textures in their
    // constructors, so they come after it.
    gfx::Shader m_texturedShader;
    gfx::Shader m_colorShader;
    gfx::Shader m_litShader;
    gfx::Shader m_gouraudShader;
    gfx::Shader m_skyboxShader;
    gfx::Shader m_grassShader;
    gfx::Shader m_compositeShader;
    gfx::Shader m_previewShader;
    gfx::Shader m_brightPassShader;
    gfx::Shader m_blurShader;
    // Draws depth only, from the view of a light: the program of the shadow pass.
    gfx::Shader m_shadowDepthShader;
    // Draws the flat shapes of the minimap into its framebuffer, and puts the finished
    // picture of the minimap into the window.
    gfx::Shader m_minimapShader;
    gfx::Shader m_minimapOverlayShader;
    assets::AssetCache m_assets;
    MazeRenderer m_mazeRenderer;
    GameplayRenderer m_gameplayRenderer;
    TerrainRenderer m_terrainRenderer;
    GrassRenderer m_grassRenderer;
    ColliderLines m_colliderLines;
    LightRig m_lightRig;
    Skybox m_skybox;
    // The HDR framebuffer of the scene and the passes after the scene.
    PostProcess m_postProcess;
    // The depth texture the scene is drawn into from the direction of the moon.
    ShadowMap m_moonShadowMap;
    // The depth texture the scene is drawn into from the flashlight.
    ShadowMap m_flashlightShadowMap;
    // The framebuffer the minimap is drawn into and the buffer of its triangles.
    MinimapRenderer m_minimapRenderer;

    // The request for the next maze (edited by the debug UI).
    MazeSettings m_mazeSettings;

    // The heightmap of the terrain, read from its picture once at start-up, and the
    // settings of the terrain (edited by the debug UI). Both are declared before
    // m_mazeWorld, because the first maze is built from them in the initializer list.
    Heightmap m_heightmap;
    TerrainSettings m_terrainSettings;

    // The maze in play, standing on its terrain.
    MazeWorld m_mazeWorld;

    // The settings of the grass (edited by the debug UI). The tufts themselves are not
    // kept: they are placed, copied to the graphics card and forgotten.
    GrassSettings m_grassSettings;

    // The numbers of the rules (edited by the debug UI) and the round in play. The round
    // is simulation state: onUpdate advances it in fixed steps. It starts empty and is
    // filled by beginRound in the constructor.
    GameplaySettings m_gameplay;
    Round m_round;

    // What the player cannot walk through in this round: the boxes of the maze, plus the
    // box of the gate while it is closed (game::roundObstacles). A copy that is rebuilt
    // only when it changes: at the start of a round, when the gate opens and when the
    // terrain is built again with another height scale (the boxes move up or down).
    std::vector<scene::Aabb> m_obstacles;

    // The player is simulation state: onUpdate moves it in fixed steps.
    Player m_player;
    // Position of the player before the last fixed step. onRender draws from a point
    // between this one and m_player.position. It starts equal to the position of the
    // player, so the frames before the first step are drawn from where the player stands.
    // Declared after m_player, because members are initialized top to bottom.
    glm::vec3 m_previousPlayerPosition = m_player.position;
    // Whether the player was in noclip mode in the last fixed step. The step in which
    // a flying player starts to walk drops the feet to the ground, and that drop is the
    // one change of height that must not be blended (see onUpdate).
    bool m_playerWasFlying = false;

    // Where the scene is seen from (the view and projection matrices). The angles are
    // turned by the mouse. The position is not controlled directly: after every fixed
    // step it is set to the eyes of the player.
    scene::Camera m_camera;

    // The lighting: how the scene is shaded and the settings of every light. The lights
    // of a frame are built from it in onRender.
    LightingSettings m_lighting;

    // The shadows of the moon: their settings (edited by the debug UI), the view and
    // the projection of the moon in this frame, and whether the shadow pass has filled
    // the map in this frame. The last two are set by drawMoonShadowMap in every frame.
    ShadowSettings m_moonShadow;
    scene::LightSpace m_moonLightSpace;
    bool m_moonShadowDrawn = false;

    // The same three for the shadows of the flashlight, set by drawFlashlightShadowMap
    // in every frame. The settings start with a smaller map and a bias of their own
    // (game::flashlightShadowDefaults).
    ShadowSettings m_flashlightShadow = flashlightShadowDefaults();
    scene::LightSpace m_flashlightLightSpace;
    bool m_flashlightShadowDrawn = false;

    // What the textured shader shows: the picture, or one of the two debug views. A debug
    // view replaces the lighting: it is drawn with the textured program in every
    // lighting mode.
    ViewMode m_viewMode = ViewMode::Textured;
    // Whether the collision boxes and spheres are drawn as lines on top of the scene.
    bool m_drawColliders = false;

    // Whether the sky is drawn and how bright it is.
    SkyboxSettings m_skyboxSettings;

    // The exposure and the tone mapping of the composite pass and the settings of the
    // bloom, the fog and the vignette.
    PostProcessSettings m_postProcessSettings;

    // How the camera is turned. It belongs to the controls, not to the camera.
    float m_mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;

    // Whether the minimap is drawn, whether it shows the whole maze, and its size, its
    // corner and its opacity.
    MinimapSettings m_minimapSettings;
};

} // namespace game
