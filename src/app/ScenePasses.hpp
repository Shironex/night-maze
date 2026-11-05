// The passes of a frame that draw the world: the two shadow passes and the parts of the
// scene pass.
#pragma once

#include "app/AppShaders.hpp"
#include "app/FrameState.hpp"
#include "app/RenderSettings.hpp"
#include "app/RoundClocks.hpp"
#include "app/SceneRenderers.hpp"
#include "core/Time.hpp"
#include "game/lighting/Lighting.hpp"
#include "game/maze/MazeRenderer.hpp"
#include "game/maze/MazeWorld.hpp"
#include "game/menu/MenuCamera.hpp"
#include "game/player/Player.hpp"
#include "game/round/Round.hpp"
#include "game/screens/GameState.hpp"
#include "gfx/Shader.hpp"
#include "scene/Camera.hpp"
#include "scene/Collider.hpp"
#include "scene/Frustum.hpp"
#include "scene/LightSpace.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace game {

/// The passes that draw the world into the shadow maps and into the scene, in the order
/// onRender of NightMazeApp calls them. They read the state of the application (the
/// maze, the round, the player, the settings) and draw it with its shader programs and
/// its renderers. The class holds references to those members of the application, under
/// the same names, and is itself a member of it, declared after all of them, so they
/// live as long as it does. What a pass leaves for the rest of the frame (the spaces of
/// the shadow maps, the sway of the held lamp) goes into the FrameState of the frame.
class ScenePasses {
public:
    /// The members of the application the passes read and draw with.
    struct Sources {
        const RenderSettings& renderSettings;
        const AppShaders& shaders;
        SceneRenderers& renderers;
        const MazeSettings& mazeSettings;
        const MazeWorld& mazeWorld;
        const GameplaySettings& gameplay;
        const Round& round;
        const std::vector<scene::Aabb>& obstacles;
        FrameState& frameState;
        const Player& player;
        const glm::vec3& previousPlayerPosition;
        const scene::Camera& camera;
        const LightingSettings& lighting;
        const std::uint32_t& groundVersion;
        const MazeBounds& mazeBounds;
        const MenuCameraSettings& menuCamera;
        const GameMode& mode;
        const RoundClocks& roundClocks;
        const core::Time& time;
    };

    explicit ScenePasses(const Sources& sources);

    /// The shadow pass of the moon, the first pass of a frame. It fits the box of the
    /// moon to the land (m_frameState.moonLightSpace) and fills the shadow map in two steps. The
    /// casters that never move (drawFixedShadowCasters) are in the still layer of the
    /// map, which is drawn only when its key is not the one of this frame
    /// (game::StaticShadowKey). The layer is copied into the map, and the casters that
    /// can move (drawMovingShadowCasters) are drawn on top in every frame. Then it binds
    /// the map for the lit programs and, while the debug UI asks for it, draws its
    /// preview picture. With the shadows switched off it only computes the box. It
    /// leaves a framebuffer of the shadow map bound, so the scene framebuffer has to be
    /// bound after it.
    void drawMoonShadowMap();

    /// The shadow pass of the flashlight, right after the one of the moon. It builds
    /// the pyramid the flashlight looks along (m_frameState.flashlightLightSpace) from flashlight,
    /// the place and the direction of the light in this frame, and from the cone and
    /// the range of frameLighting, the lighting of this frame (game::lightingForFrame).
    /// Then it draws the shadow casters into the shadow map of the flashlight, binds
    /// the map for the lit programs and, while the debug UI asks for it, draws its
    /// preview picture. With the shadows of the flashlight switched off, or with the
    /// flashlight itself off in this frame, it only computes the pyramid. Like
    /// drawMoonShadowMap it leaves a framebuffer of its own bound.
    void drawFlashlightShadowMap(const LightingSettings& frameLighting,
                                 const FlashlightPose& flashlight);

    /// The parts of a frame. Each one selects its own shader program and sets its
    /// uniforms. drawMaze draws the terrain and the maze together with the crystals,
    /// the gate, the levers and the notes, and has two ways to do it: without lighting (the
    /// textured program, also used for the debug views of the normals and the texture coordinates)
    /// and with lighting (drawUnlitMaze and drawLitMaze). drawGrass draws the grass with its own
    /// program.
    ///
    /// drawMaze makes the frustum of the pass from the view and the projection it is
    /// given (scene::frustumFromMatrix) and hands it to the way it chose, so whatever camera the
    /// frame is drawn with (the player, the menu camera, the intro, the reveal of the
    /// village) is the one that is culled with. The terrain is one draw call and is never
    /// left out. drawGrass culls the chunks of the grass with the same frustum, when the
    /// frustum culling and the chunk culling of the grass are both switched on.
    void drawMaze(const glm::mat4& view, const glm::mat4& projection) const;
    void drawGrass(const glm::mat4& view, const glm::mat4& projection) const;
    void drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const;

    /// The debug view of the picking, drawn with the program of the collider lines: the
    /// pick box of every lever and note, the box the ray hit in a colour of its own, and
    /// the ray of m_frameState.shownPick as a line with a small sphere at its end.
    void drawPickLines(const glm::mat4& view, const glm::mat4& projection) const;

    /// The reflection pass, after the maze and the grass and before the sky: the
    /// crystals and the puddles, drawn with the reflect program, which shows the sky on
    /// them (environment mapping). It handles all four lighting modes itself, like
    /// drawGrass. In the two debug views it only draws the puddles, as data, with the
    /// textured program. With the environment mapping switched off it draws nothing.
    void drawReflections(const glm::mat4& view, const glm::mat4& projection) const;

    /// Draws the village on the ridge and its lights into the scene, centred on the eye
    /// like the sky and with real depth. view and projection are the ones of the frame,
    /// farPlane the far plane of its camera. nightsLit and newestStrength are what
    /// VillageRenderer::draw takes. Nothing is drawn in a maze without a gate, with the
    /// switch of the debug UI off, or without the textured program.
    void drawVillage(const glm::mat4& view, const glm::mat4& projection, float farPlane,
                     int nightsLit, float newestStrength) const;

    /// Draws the lamp in the hand of the player on top of the finished scene, while the
    /// picture is taken from the eyes of the player (game::showsHeldLamp) and the switch
    /// of the debug UI is on. view and projection are the ones of the frame, frameLighting
    /// the lighting the frame was drawn with, and lights the lights that were built from
    /// it. revealShown is NightMazeApp::villageRevealShown of this frame.
    ///
    /// The lamp lies on the axis of its beam (game::heldLampMatrix) and is drawn with the
    /// program of the lighting mode, like the maze, with a field of view of its own
    /// (game::heldLampProjection). It is in no shadow map. Its own beam does not light
    /// it: the function fills the buffer of the lights again without the flashlight, so
    /// it must come after everything else that is lit in the frame.
    ///
    /// It cannot sink into a wall the player stands against: its depths are squeezed
    /// into the nearest hundredth of the depth range, in front of everything the scene
    /// can hold. The depth buffer is not cleared, because the fog of the composite pass
    /// reads the depths of the scene.
    ///
    /// The lamp sways with the steps of the player and falls a little behind the turns
    /// of the camera (game::advanceHeldLampSway). The function advances that sway by the
    /// time of the frame while the round runs, which is why it is not const. Only the
    /// picture of the lamp moves: its light is where the frame put it.
    void drawHeldLamp(const glm::mat4& view, const glm::mat4& projection,
                      const LightingSettings& frameLighting, const scene::LightSet& lights,
                      bool revealShown);

private:
    /// Draws everything that casts a shadow with the depth program, as the light with
    /// the given view and projection sees it: the two halves below, one after the other.
    /// The grass casts no shadow. The target (a shadow map) must be bound already.
    ///
    /// Only the flashlight calls it, and it culls: a caster that lies outside the
    /// pyramid of the light leaves no texel in its map, so it is not drawn. The frustum
    /// is made here from lightSpace itself, the matrices the casters are drawn with.
    /// It is never the frustum of the camera: a wall behind the player is outside the
    /// picture and can still throw its shadow into it.
    void drawShadowCasters(const scene::LightSpace& lightSpace) const;

    /// The casters that stand where the world has put them until the world is built
    /// again: the terrain, the walls no lever lowers, the pillars, the post of the stile
    /// and the stone sheep (MazeParts::Fixed). Nothing here reads the round or a clock,
    /// which is what lets the moon keep them in the still layer of its map. What they
    /// do depend on is in game::StaticShadowKey.
    ///
    /// frustum, here and below, is the frustum of lightSpace (view frustum culling), or
    /// nullptr to draw every caster. The moon passes nullptr: its box is fitted around
    /// the whole land, so no caster lies outside of it and a test could reject nothing.
    void drawFixedShadowCasters(const scene::LightSpace& lightSpace,
                                const scene::Frustum* frustum) const;

    /// Every other caster, as it is in this frame: the walls of the levers (as far as
    /// they have sunk), the gate (as far as it has sunk) with its gatehouse and the
    /// swinging bell, the crystals and the flasks where they float, the shade, the
    /// levers and the notes. A caster belongs here as soon as anything of a round can
    /// move it or take it away, also when it stands still most of the time.
    void drawMovingShadowCasters(const scene::LightSpace& lightSpace,
                                 const scene::Frustum* frustum) const;

    /// Sets the uniforms of both shadow maps (the moon and the flashlight) in the
    /// program shader, which must be in use and must include common/shadows.glsl:
    /// game::setShadowUniforms once for each map. Called in every frame for the lit
    /// program in use and for the grass programs, also with the shadows switched off.
    void setShadowUniformsOf(const gfx::Shader& shader) const;

    /// The two ways of drawMaze, without and with lighting. frustum is the one of drawMaze.
    void drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection,
                       const scene::Frustum* frustum) const;
    void drawLitMaze(const glm::mat4& view, const glm::mat4& projection,
                     const scene::Frustum* frustum) const;

    /// Draws the levers and the notes with the given program of the maze, the picked
    /// one highlighted (game::highlightGlow). frustum is the one of drawMaze.
    void drawInteractables(const gfx::Shader& shader, const scene::Frustum* frustum) const;

    /// True when the crystals of this frame are drawn by drawReflections. False when
    /// drawMaze draws them with the program of the walls: with the environment mapping
    /// switched off, in the two debug views and when the reflect program failed to load.
    bool crystalsReflect() const;

    /// Draws the gate with the given program of the maze, and the crystals too unless
    /// drawReflections draws them in this frame (crystalsReflect). frustum is the one
    /// of drawMaze.
    void drawGateAndCrystals(const gfx::Shader& shader, const scene::Frustum* frustum) const;

    /// The light the crystals give off by themselves at this moment, as a linear colour
    /// for the uniform uEmissive: game::crystalGlow of the colour of the crystal lights.
    glm::vec3 crystalEmissive() const;

    /// The clock of the application, for the time of the frame.
    const core::Time& time() const { return m_time; }

    // The members of the application, under the names they have there.
    const RenderSettings& m_renderSettings;
    const AppShaders& m_shaders;
    SceneRenderers& m_renderers;
    const MazeSettings& m_mazeSettings;
    const MazeWorld& m_mazeWorld;
    const GameplaySettings& m_gameplay;
    const Round& m_round;
    const std::vector<scene::Aabb>& m_obstacles;
    FrameState& m_frameState;
    const Player& m_player;
    const glm::vec3& m_previousPlayerPosition;
    const scene::Camera& m_camera;
    const LightingSettings& m_lighting;
    const std::uint32_t& m_groundVersion;
    const MazeBounds& m_mazeBounds;
    const MenuCameraSettings& m_menuCamera;
    const GameMode& m_mode;
    const RoundClocks& m_roundClocks;
    const core::Time& m_time;
};

} // namespace game
