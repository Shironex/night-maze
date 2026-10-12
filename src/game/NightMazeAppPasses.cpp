// The Night Maze application: the shadow passes and the parts of the scene pass.
#include "game/NightMazeApp.hpp"

#include "core/GlCheck.hpp"
#include "game/Crystals.hpp"
#include "game/Exit.hpp"
#include "game/Interactables.hpp"
#include "game/ModelDraw.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/ColorSpace.hpp"
#include "scene/Transform.hpp"

#include <GLFW/glfw3.h>

#include <cstddef>
#include <span>
#include <vector>

namespace game {

namespace {

// The names of the uniforms (MODEL_UNIFORM, VIEW_UNIFORM, PROJECTION_UNIFORM and the
// others) are in game/ShaderUniforms.hpp, shared with the classes that draw the maze.

// Colours of the collision lines (red, green, blue): the boxes of the maze in yellow,
// the box and the reach of the player in green, the box of the gate in orange, the
// pickup spheres of the crystals in cyan and the exit zone in magenta.
constexpr glm::vec3 MAZE_COLLIDER_COLOR{1.0F, 0.85F, 0.1F};
constexpr glm::vec3 PLAYER_COLLIDER_COLOR{0.2F, 1.0F, 0.4F};
constexpr glm::vec3 GATE_COLLIDER_COLOR{1.0F, 0.45F, 0.1F};
constexpr glm::vec3 PICKUP_COLLIDER_COLOR{0.2F, 0.9F, 1.0F};
constexpr glm::vec3 EXIT_ZONE_COLOR{1.0F, 0.3F, 0.9F};

// Colours of the debug view of the picking: the pick boxes of the levers in red and of
// the notes in white, the picking ray and the box it hit in bright green, and a ray that
// hit nothing in grey.
constexpr glm::vec3 LEVER_PICK_COLOR{1.0F, 0.35F, 0.25F};
constexpr glm::vec3 NOTE_PICK_COLOR{0.95F, 0.95F, 0.85F};
constexpr glm::vec3 PICK_HIT_COLOR{0.3F, 1.0F, 0.3F};
constexpr glm::vec3 PICK_MISS_COLOR{0.6F, 0.6F, 0.6F};

// Radius of the small sphere that marks the end of the drawn picking ray, in metres.
constexpr float PICK_MARKER_RADIUS = 0.04F;

// The highlight of a puddle. Water is smooth: its highlight is as bright as the light
// that makes it (strength 1) and small and sharp (a large exponent), unlike the weak,
// wide highlight of the rough stone (LightingSettings::specularStrength and shininess).
constexpr float PUDDLE_SPECULAR_STRENGTH = 1.0F;
constexpr float PUDDLE_SHININESS = 128.0F;

// The highlight of the cloak of the shade: dull cloth, far weaker than the stone.
constexpr float SHADE_SPECULAR_STRENGTH = 0.03F;

// A puddle only mirrors the sky: all of what it shows is the reflected picture, none
// the refracted one (the uniform uReflectShare of reflect.frag).
constexpr float MIRROR_ONLY = 1.0F;

// The face of the shade is the opposite: all of its colour is the sky
// (uEnvironmentStrength), all of that the picture seen through the surface
// (uReflectShare), and the ray into it is not bent (uRefractionRatio), so the sky is read
// along the look of the eye. That sky is turned by this angle around the vertical axis,
// in radians (uSkyTurn): 20 degrees, a little, but enough that no star in the hood sits
// where the sky behind the figure has it.
constexpr float SKY_ONLY = 1.0F;
constexpr float SEEN_THROUGH_ONLY = 0.0F;
constexpr float NO_REFRACTION = 1.0F;
constexpr float SHADE_HOOD_SKY_TURN = 0.35F;
// The face shows a wider piece of sky than it covers (uSkySpread, per metre from the
// middle of the face, which is this point of the model shade_hollow.obj): the face is
// 16 cm wide, and with a spread of 1 its rim reads the sky about 5 degrees further out
// than the look of the eye alone would.
constexpr float SHADE_HOOD_SKY_SPREAD = 1.0F;
constexpr glm::vec3 SHADE_HOOD_CENTRE{0.0F, 1.81F, 0.15F};

// The highlight of the tin of the held lamp: bare metal, brighter than the one of the
// rough stone.
constexpr float HELD_LAMP_SPECULAR_STRENGTH = 0.6F;

// The part of the depth range the held lamp is drawn into, from 0 (the near plane) up to
// this. With the planes of the camera (0.1 m and 100 m) the scene has a depth below it
// only within a millimetre behind the near plane, so the lamp is in front of every wall,
// and the fog of the composite pass reads its pixels as 0.1 m away: no fog.
constexpr double HELD_LAMP_DEPTH_RANGE = 0.01;

} // namespace

void NightMazeApp::drawMoonShadowMap() {
    // Where the moon looks: an orthographic box around the whole land, seen from the
    // direction of its light. It is computed again in every frame, from the terrain
    // and the two angles of the moon alone: a few dozen multiplications, and nothing
    // that could be forgotten when a new maze is built, the height scale changes or
    // the Lights panel moves the moon. The camera is not part of it, so the map covers
    // the same ground in every frame and the shadows stand still when the player
    // moves.
    m_moonLightSpace = scene::directionalLightSpace(shadowCasterBounds(m_mazeWorld.terrain),
                                                    moonDirection(m_lighting));

    // Until the pass below has run, this frame has no shadows.
    m_moonShadowDrawn = false;
    if (!m_moonShadow.enabled || !m_shadowDepthShader.isValid()) {
        return;
    }

    // Everything the casters that never move are drawn from. Because the camera is not
    // part of the box of the moon, this key stays the same from frame to frame until
    // one of its fields changes: the Lights panel moves the moon, another resolution is
    // chosen, a maze or a terrain is built, the wall variants are switched, or the
    // depth program is reloaded. Nothing has to tell this pass about any of them.
    const StaticShadowKey key{.lightView = m_moonLightSpace.view,
                              .lightProjection = m_moonLightSpace.projection,
                              .mapSize = shadowMapSize(m_moonShadow.resolution),
                              .groundVersion = m_groundVersion,
                              .wallVariants = m_mazeSettings.wallVariants,
                              .depthProgramLoads = m_shadowDepthShader.loadCount()};

    // What the graphics card does from here to end() is the measured pass: the copy
    // into the map and the draw calls into it, without the preview below. In a frame
    // that draws the still layer again, that is in it too.
    m_gpuPasses.begin(GpuPass::MoonShadow);
    if (!m_moonShadowMap.staticLayerHolds(key)) {
        if (!m_moonShadowMap.beginStaticPass(key)) {
            m_gpuPasses.end();
            return;
        }
        drawFixedShadowCasters(m_moonLightSpace);
    }
    // The map of this frame: the still layer, and on top of it what can move.
    if (!m_moonShadowMap.beginDepthPassOverStatic()) {
        m_gpuPasses.end();
        return;
    }
    drawMovingShadowCasters(m_moonLightSpace);
    m_gpuPasses.end();
    m_moonShadowDrawn = true;

    // The map goes to its texture unit once, and stays there while the scene is drawn.
    m_moonShadowMap.bindForSampling(MOON_SHADOW_TEXTURE_UNIT, m_moonShadow.hardwareFilter);

    // The picture of the map, only while the debug UI shows it.
    if (m_moonShadow.preview) {
        m_moonShadowMap.drawPreview(m_previewShader, m_moonLightSpace);
    }
}

void NightMazeApp::drawFlashlightShadowMap(const LightingSettings& frameLighting,
                                           const FlashlightPose& flashlight) {
    // Where the flashlight looks: a pyramid with its tip in the hand, along the beam,
    // a little wider than the cone of light and as deep as the light reaches. It is
    // computed again in every frame, also with the shadows switched off: the debug UI
    // shows its size. The position and the direction are the ones the spot light of
    // this frame is built with (onRender), the cone and the range come from the same
    // settings of the frame.
    m_flashlightLightSpace =
        scene::spotLightSpace(flashlight.position, flashlight.direction,
                              frameLighting.flashlightOuterDegrees, frameLighting.flashlightRange);

    // Until the pass below has run, this frame has no flashlight shadows.
    m_flashlightShadowDrawn = false;
    // A flashlight that is off gives no light, so there is nothing its shadows could
    // take away: the pass is skipped. frameLighting is asked and not m_lighting,
    // because an empty battery switches the light off for the frame.
    if (!m_flashlightShadow.enabled || !frameLighting.flashlightOn ||
        !m_shadowDepthShader.isValid()) {
        return;
    }
    // Measured like the pass of the moon.
    m_gpuPasses.begin(GpuPass::FlashlightShadow);
    if (!m_flashlightShadowMap.beginDepthPass(shadowMapSize(m_flashlightShadow.resolution))) {
        m_gpuPasses.end();
        return;
    }
    drawShadowCasters(m_flashlightLightSpace);
    m_gpuPasses.end();
    m_flashlightShadowDrawn = true;

    // The map goes to its own texture unit, next to the one of the moon.
    m_flashlightShadowMap.bindForSampling(FLASHLIGHT_SHADOW_TEXTURE_UNIT,
                                          m_flashlightShadow.hardwareFilter);

    // The picture of the map, only while the debug UI shows it.
    if (m_flashlightShadow.preview) {
        m_flashlightShadowMap.drawPreview(m_previewShader, m_flashlightLightSpace);
    }
}

void NightMazeApp::setShadowUniformsOf(const gfx::Shader& shader) const {
    // The shadow map of the moon: where it is bound, the matrix it was drawn with and
    // the numbers of the comparison.
    setShadowUniforms(shader, MOON_SHADOW_UNIFORMS, MOON_SHADOW_TEXTURE_UNIT, m_moonShadowDrawn,
                      m_moonShadow, m_moonLightSpace);
    // The same for the shadow map of the flashlight.
    setShadowUniforms(shader, FLASHLIGHT_SHADOW_UNIFORMS, FLASHLIGHT_SHADOW_TEXTURE_UNIT,
                      m_flashlightShadowDrawn, m_flashlightShadow, m_flashlightLightSpace);
}

void NightMazeApp::drawShadowCasters(const scene::LightSpace& lightSpace) const {
    // The flashlight draws both halves in every frame: its pyramid moves with the hand,
    // so nothing stands still in its map. The moon calls the halves apart
    // (drawMoonShadowMap).
    drawFixedShadowCasters(lightSpace);
    drawMovingShadowCasters(lightSpace);
}

void NightMazeApp::drawFixedShadowCasters(const scene::LightSpace& lightSpace) const {
    // The classes that draw the scene are used as they are, with another program and
    // the matrices of the light in place of the ones of the camera. So everything
    // stands in the shadow map exactly where it stands in the picture. The depth program
    // has no samplers, no tint and no glow: those uniforms are set all the same and
    // ignored, as every uniform a program does not have.
    m_shadowDepthShader.use();
    m_shadowDepthShader.setMat4(VIEW_UNIFORM, lightSpace.view);
    m_shadowDepthShader.setMat4(PROJECTION_UNIFORM, lightSpace.projection);

    // The terrain is always drawn filled here: the wireframe switch is a way to look
    // at the ground, and a ground of lines would cast a shadow of lines.
    constexpr bool NO_WIREFRAME = false;
    m_terrainRenderer.draw(m_shadowDepthShader, NO_WIREFRAME);
    // The walls no lever lowers, from the matrices of the world and not from the ones
    // of the round: the two lists are the same for these walls (game::leverWallFlags),
    // and read from the world this function depends on no round.
    m_mazeRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_mazeWorld.wallMatrices,
                        m_mazeSettings.wallVariants, MazeParts::Fixed);
}

void NightMazeApp::drawMovingShadowCasters(const scene::LightSpace& lightSpace) const {
    // The program and the matrices again: the moon draws this half without the other
    // one in most frames. Here the casters stand as the round has them at this moment:
    // the gate and the walls of pulled levers as far as they have sunk, every crystal
    // where it floats.
    m_shadowDepthShader.use();
    m_shadowDepthShader.setMat4(VIEW_UNIFORM, lightSpace.view);
    m_shadowDepthShader.setMat4(PROJECTION_UNIFORM, lightSpace.projection);

    // The walls of the levers, in every frame of the round and not only while one
    // sinks: there are at most MAX_LEVER_COUNT of them, and a wall that is always drawn
    // needs no moment at which the still layer has to learn that it started to move or
    // came to rest.
    m_mazeRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_wallMatrices,
                        m_mazeSettings.wallVariants, MazeParts::LeverWalls);
    // The gatehouse, the milestone and the lanterns never move, but they are drawn by
    // the same call as the door and the bell, which do: they stay on this side.
    m_gameplayRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, crystalEmissive(),
                            m_gateLampGlow, m_bellSwing.degrees);
    m_gameplayRenderer.drawFlasks(m_shadowDepthShader, m_mazeWorld, m_round);
    // The shade casts a shadow like everything that stands in the maze.
    if (m_shadeDrawn) {
        m_gameplayRenderer.drawShade(m_shadowDepthShader, m_shadeMatrix);
        m_gameplayRenderer.drawShadeHollow(m_shadowDepthShader, m_shadeMatrix);
    }
    // The levers and the notes cast shadows too. An empty PickState: nothing is
    // highlighted, the depth program has no colours.
    m_interactableRenderer.draw(m_shadowDepthShader, m_mazeWorld, m_round, PickState{},
                                glm::vec3{0.0F});
    // The grass is left out. A blade is 4 cm wide at its root and thinner above, and
    // a texel of the map of the moon is about 3 cm, so its shadow would be a flicker of
    // single texels that moves with the wind, on ground the tuft itself hides. The map
    // of the flashlight has finer texels, but the grass is left out of it too: one rule
    // for both lights, and no shadows that sway on every wall the beam passes. The
    // grass still RECEIVES shadows.
}

glm::vec3 NightMazeApp::crystalEmissive() const {
    // The colour of the crystal lights is an sRGB value, like every colour of the
    // lighting settings. It is converted here the way buildLightSet converts it for
    // the lights, so the mesh glows in the colour of the light around it.
    return crystalGlow(gfx::srgbToLinear(m_lighting.pointColor), m_round.animationSeconds);
}

void NightMazeApp::drawMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // The two debug views (normals and texture coordinates as colours) only exist in the
    // textured program, and they show data, not light. So they are drawn without
    // lighting whatever the lighting mode is. The view of the normals still follows the
    // lighting in one thing: it shows the normals the chosen mode shades with.
    if (m_lighting.mode == LightingMode::Unlit || m_viewMode != ViewMode::Textured) {
        drawUnlitMaze(view, projection);
    } else {
        drawLitMaze(view, projection);
    }
}

void NightMazeApp::drawUnlitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Without a shader program there is nothing to draw with. The load error was logged
    // once, when the shader was created, and the rest of the frame is still drawn.
    if (!m_texturedShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_texturedShader.use();
    m_texturedShader.setMat4(VIEW_UNIFORM, view);
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    // The enum values are the numbers textured.frag compares uViewMode with.
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    // Only the view of the normals reads it: that view shows the normals the lighting
    // would use, so with normal mapping the ones from the normal maps.
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);

    // The ground first, then what stands on it. The order does not change the picture
    // (the depth test sorts it out), it only follows the way the scene is built.
    m_terrainRenderer.draw(m_texturedShader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(m_texturedShader, m_mazeWorld, m_wallMatrices, m_mazeSettings.wallVariants);
    // The crystals and the gate, with the same program: they show up in the debug
    // views like the walls do. (In the picture without lighting the crystals may be
    // left to the reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(m_texturedShader);
    // The levers and the notes. The highlight of the picked one shows in the picture
    // without lighting. The two debug views show data and ignore it.
    drawInteractables(m_texturedShader);
}

void NightMazeApp::drawLitMaze(const glm::mat4& view, const glm::mat4& projection) const {
    // Gouraud has a program of its own (the light is computed in its vertex shader).
    // Phong and Blinn-Phong share the other one and differ in one uniform.
    const gfx::Shader& shader =
        m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;
    if (!shader.isValid()) {
        return;
    }

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, projection);
    // The material of the stone, which the ground shares. The lights themselves are not
    // set here: they are in the uniform buffer that onRender filled before this call.
    // The enum values are the numbers common/lighting.glsl compares uSpecularModel with.
    shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    shader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    // Normal mapping, the switch of the lit program (1 on, 0 off). The Gouraud program
    // has no such uniform, and usesNormalMap is false for it anyway.
    shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    // The two shadow maps (the moon and the flashlight). Set in every frame, also with
    // the shadows off.
    setShadowUniformsOf(shader);

    // The ground first, then what stands on it, as in drawUnlitMaze.
    m_terrainRenderer.draw(shader, m_terrainSettings.wireframe);
    m_mazeRenderer.draw(shader, m_mazeWorld, m_wallMatrices, m_mazeSettings.wallVariants);
    // The crystals and the gate, with the same program and so the same lighting mode.
    // The crystals glow in the colour of their lights. (The crystals may be left to the
    // reflection pass, see drawGateAndCrystals.)
    drawGateAndCrystals(shader);
    // The levers and the notes, lit like the walls they hang on.
    drawInteractables(shader);
}

void NightMazeApp::drawInteractables(const gfx::Shader& shader) const {
    // The highlight pulses on the animation clock of the round, which never stops.
    m_interactableRenderer.draw(shader, m_mazeWorld, m_round, m_pick,
                                highlightGlow(m_round.animationSeconds));
}

bool NightMazeApp::crystalsReflect() const {
    return m_environment.enabled && m_viewMode == ViewMode::Textured && m_reflectShader.isValid();
}

void NightMazeApp::drawGateAndCrystals(const gfx::Shader& shader) const {
    m_gameplayRenderer.drawGate(shader, m_mazeWorld, m_round, m_gateLampGlow, m_bellSwing.degrees);
    // The flasks, always with the program of the walls: they are brass, not glass, and
    // show no sky.
    m_gameplayRenderer.drawFlasks(shader, m_mazeWorld, m_round);
    // The shade, with the program of the walls too: it is lit like them, so the beam of
    // the flashlight shows it and the dark hides it. Its cloth has almost no highlight:
    // with the highlight of the stone the flashlight, which shines from where the player
    // looks, would paint a bright board on the dark figure. The strength of the stone is
    // put back for what is drawn next. (The textured program has no such uniform.)
    if (m_shadeDrawn) {
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, SHADE_SPECULAR_STRENGTH);
        m_gameplayRenderer.drawShade(shader, m_shadeMatrix);
        // The night in its hood and its cuffs are drawn exactly once per frame, like the
        // crystals: here as dark cloth, or later by drawReflections as the night sky.
        if (!crystalsReflect()) {
            m_gameplayRenderer.drawShadeHollow(shader, m_shadeMatrix);
        }
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    }
    // Every crystal is drawn exactly once per frame: here, with the program of the
    // walls, or later by drawReflections with the reflect program.
    if (!crystalsReflect()) {
        m_gameplayRenderer.drawCrystals(shader, m_mazeWorld, m_round, crystalEmissive());
    }
}

void NightMazeApp::drawReflections(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_environment.enabled) {
        return;
    }
    const bool puddlesDrawn = m_environment.puddles && m_puddleRenderer.puddleCount() > 0;

    // The two debug views show data, not light or sky. The crystals were drawn by
    // drawUnlitMaze with the textured program. The puddles follow here with the same
    // program, whose uniforms of this frame (the matrices and the view mode) that
    // function has already set: a uniform keeps its value while other programs draw.
    if (m_viewMode != ViewMode::Textured) {
        if (puddlesDrawn && m_texturedShader.isValid()) {
            m_texturedShader.use();
            m_puddleRenderer.draw(m_texturedShader);
        }
        return;
    }

    if (!m_reflectShader.isValid()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    m_reflectShader.use();
    m_reflectShader.setMat4(VIEW_UNIFORM, view);
    m_reflectShader.setMat4(PROJECTION_UNIFORM, projection);

    // The lighting, as drawLitMaze sets it for the lit program. The reflect program
    // computes the light per fragment in every lit mode, also in the mode Gouraud (the
    // sky has to be looked up per fragment anyway), and shows the surface at full
    // brightness in the mode Unlit, like the grass program.
    const bool lit = m_lighting.mode != LightingMode::Unlit;
    m_reflectShader.setInt(REFLECT_LIT_UNIFORM, lit ? 1 : 0);
    m_reflectShader.setInt(SPECULAR_MODEL_UNIFORM,
                           static_cast<int>(specularModelOf(m_lighting.mode)));
    m_reflectShader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    m_reflectShader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
    m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    setShadowUniformsOf(m_reflectShader);

    // The sky the surfaces show: the cube map of the skybox on a texture unit of its
    // own, as bright as the skybox draws it. With the skybox switched off (or without
    // its pictures) the background of the frame is the clear colour, so that colour is
    // what a mirror shows. The sampler gets its unit in every case: left at 0 it would
    // share a unit with the colour texture, which OpenGL does not allow.
    const bool skyVisible = m_skyboxSettings.enabled && m_skybox.isValid();
    m_reflectShader.setInt(ENVIRONMENT_MAP_UNIFORM, static_cast<int>(ENVIRONMENT_TEXTURE_UNIT));
    m_reflectShader.setInt(ENVIRONMENT_SKY_VISIBLE_UNIFORM, skyVisible ? 1 : 0);
    m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM, m_skyboxSettings.brightness);
    m_reflectShader.setVec3(
        ENVIRONMENT_BACKGROUND_UNIFORM,
        gfx::srgbToLinear(glm::vec3{m_clearColor[0], m_clearColor[1], m_clearColor[2]}));
    if (m_skybox.isValid()) {
        m_skybox.cubemap().bind(ENVIRONMENT_TEXTURE_UNIT);
        // Blend the texels of two faces at the border between them, see Skybox::draw. It
        // is switched on here too: this pass reads the cube map before the sky is drawn.
        GL_CHECK(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
    }

    // The crystals: a blend of the mirrored and of the refracted sky, the same share at
    // every angle. Their glow can be turned down for this pass, to see the sky on them.
    m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, m_environment.crystalStrength);
    m_reflectShader.setInt(ENVIRONMENT_FRESNEL_ENABLED_UNIFORM, 0);
    m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, m_environment.crystalReflectShare);
    m_reflectShader.setFloat(ENVIRONMENT_REFRACTION_RATIO_UNIFORM,
                             m_environment.crystalRefractionRatio);
    m_gameplayRenderer.drawCrystals(m_reflectShader, m_mazeWorld, m_round,
                                    crystalEmissive() * m_environment.crystalGlowShare);

    // The face in the hood of the shade and its cuffs: a piece of the night. All of
    // their colour is the sky (a strength of 1, so the beam of the flashlight does not
    // light them), read where the eye looks and not where a mirror would send the look:
    // the refracted picture alone, with a ratio that bends nothing. The sky is turned
    // by SHADE_HOOD_SKY_TURN, so these are not the stars behind the figure, and is as
    // bright as the burn clock says, its stars lifted most. The faces are flat: no normal
    // map. The turn, the star boost and the brightness are put back for the puddles. The strength,
    // the shares and the normal map they set themselves.
    if (m_shadeDrawn) {
        m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, 0);
        m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, SKY_ONLY);
        m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, SEEN_THROUGH_ONLY);
        m_reflectShader.setFloat(ENVIRONMENT_REFRACTION_RATIO_UNIFORM, NO_REFRACTION);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_TURN_UNIFORM, SHADE_HOOD_SKY_TURN);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM,
                                 m_skyboxSettings.brightness * m_shadeHoodBrightness);
        m_reflectShader.setFloat(ENVIRONMENT_STAR_BOOST_UNIFORM, m_shadeHoodStarBoost);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_SPREAD_UNIFORM, SHADE_HOOD_SKY_SPREAD);
        m_reflectShader.setVec3(ENVIRONMENT_SKY_SPREAD_CENTRE_UNIFORM,
                                glm::vec3{m_shadeMatrix * glm::vec4{SHADE_HOOD_CENTRE, 1.0F}});
        m_gameplayRenderer.drawShadeHollow(m_reflectShader, m_shadeMatrix);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_TURN_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_STAR_BOOST_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_SPREAD_UNIFORM, 0.0F);
        m_reflectShader.setFloat(ENVIRONMENT_SKY_BRIGHTNESS_UNIFORM, m_skyboxSettings.brightness);
    }

    // The puddles: level mirrors. Their normal points straight up (no normal map),
    // their highlight is the one of water, and they show the mirrored sky only, more of
    // it the flatter they are looked at. The refraction ratio stays as the crystals
    // left it: with a reflect share of 1 the refracted picture is not shown.
    if (puddlesDrawn) {
        m_reflectShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, 0);
        m_reflectShader.setFloat(SPECULAR_STRENGTH_UNIFORM, PUDDLE_SPECULAR_STRENGTH);
        m_reflectShader.setFloat(SHININESS_UNIFORM, PUDDLE_SHININESS);
        m_reflectShader.setFloat(ENVIRONMENT_STRENGTH_UNIFORM, m_environment.puddleReflectivity);
        m_reflectShader.setInt(ENVIRONMENT_FRESNEL_ENABLED_UNIFORM,
                               m_environment.puddleFresnel ? 1 : 0);
        m_reflectShader.setFloat(ENVIRONMENT_REFLECT_SHARE_UNIFORM, MIRROR_ONLY);
        m_puddleRenderer.draw(m_reflectShader);
    }

    // Binding the cube map made its texture unit the active one. The draws above put
    // unit 0 back, but with no crystal left and no puddle nothing was drawn, so it is
    // put back here: the rest of the frame expects unit 0 to be the active one.
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
}

void NightMazeApp::drawVillage(const glm::mat4& view, const glm::mat4& projection, float farPlane,
                               int nightsLit, float newestStrength) const {
    if (!m_villageSettings.enabled || !m_mazeWorld.hasGate || !m_texturedShader.isValid()) {
        return;
    }
    // The state this draw relies on, said here and not taken from the pass before it: the
    // puddles are blended and write no depth, and the village must write its depth, or
    // the sky, drawn next at the largest depth, would paint over it.
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glDisable(GL_BLEND));

    m_texturedShader.use();
    // The view matrix without its translation, as in skybox.vert: the village is centred
    // on the eye and never comes closer. It keeps its real depth, unlike the sky.
    m_texturedShader.setMat4(VIEW_UNIFORM, glm::mat4{glm::mat3{view}});
    m_texturedShader.setMat4(PROJECTION_UNIFORM, projection);
    m_texturedShader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    m_texturedShader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    setModelSamplers(m_texturedShader);
    m_villageRenderer.draw(
        m_texturedShader,
        villageMatrix(villageSide(m_mazeWorld), farPlane, m_villageSettings.elevationDegrees),
        nightsLit, newestStrength);
}

void NightMazeApp::drawHeldLamp(const glm::mat4& view, const glm::mat4& projection,
                                const LightingSettings& frameLighting,
                                const scene::LightSet& lights) {
    if (!m_lighting.heldLampShown ||
        !showsHeldLamp(m_mode, m_menuCamera.enabled, villageRevealShown())) {
        return;
    }
    // The program of the lighting mode, chosen as drawMaze chooses it.
    const bool unlit = m_lighting.mode == LightingMode::Unlit || m_viewMode != ViewMode::Textured;
    const gfx::Shader& lit =
        m_lighting.mode == LightingMode::Gouraud ? m_gouraudShader : m_litShader;
    const gfx::Shader& shader = unlit ? m_texturedShader : lit;
    if (!shader.isValid()) {
        return;
    }

    // The camera of the frame, read back from its view matrix: the eye is the last
    // column of the inverse, and the rows of the rotation are the right vector, the up
    // vector and the direction against the look.
    const glm::vec3 eye{glm::inverse(view)[3]};
    const glm::vec3 right{view[0][0], view[1][0], view[2][0]};
    const glm::vec3 forward{-view[0][2], -view[1][2], -view[2][2]};

    // The lights of the frame are sent again for the lamp: without the flashlight, in
    // whose beam it lies, and with the light that spills back onto its head
    // (game::heldLampLights). Nothing that is lit is drawn after the lamp, and the next
    // frame fills the buffer anew.
    //
    // The lamp sways with the steps and lags behind the turns, by centimetres and in the
    // picture only. The sway goes on with the time of the frame while the round runs:
    // under the pause menu, on the result screen and during a catch it is left as it
    // is, so the lamp stands still. The speed is the one the feet had in the last fixed
    // step, without the height, and a flying player has no steps.
    if (m_mode == GameMode::Playing && m_catchSeconds < 0.0F) {
        const glm::vec3 walked = m_player.position - m_previousPlayerPosition;
        const float metres = m_player.noclip ? 0.0F : glm::length(glm::vec2{walked.x, walked.z});
        advanceHeldLampSway(m_heldLampSway,
                            {.seconds = static_cast<float>(time().deltaSeconds()),
                             .speed = metres / static_cast<float>(core::Time::FIXED_DT),
                             .walkSpeed = m_player.walkSpeed,
                             .sprintSpeed = m_player.sprintSpeed,
                             .yawDegrees = m_camera.yawDegrees,
                             .pitchDegrees = m_camera.pitchDegrees});
    }
    const glm::mat4 lampMatrix = heldLampMatrix(frameLighting, eye, forward, right) *
                                 heldLampSwayMatrix(heldLampSwayPose(m_heldLampSway));
    m_lightRig.upload(heldLampLights(lights, frameLighting, lampMatrix), eye);

    // The state this draw relies on, said here: the puddles are blended and write no
    // depth.
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glDisable(GL_BLEND));
    GL_CHECK(glDepthRange(0.0, HELD_LAMP_DEPTH_RANGE));

    shader.use();
    shader.setMat4(VIEW_UNIFORM, view);
    shader.setMat4(PROJECTION_UNIFORM, heldLampProjection(projection));
    shader.setInt(NORMAL_MAP_ENABLED_UNIFORM, usesNormalMap(m_lighting) ? 1 : 0);
    if (unlit) {
        shader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(m_viewMode));
    } else {
        // The material and the shadow maps, as drawLitMaze sets them. The moon shadow
        // falls on the lamp like on everything else.
        shader.setInt(SPECULAR_MODEL_UNIFORM, static_cast<int>(specularModelOf(m_lighting.mode)));
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, HELD_LAMP_SPECULAR_STRENGTH);
        shader.setFloat(SHININESS_UNIFORM, m_lighting.shininess);
        setShadowUniformsOf(shader);
    }

    // The window shows the charge of the battery and flickers with the beam.
    const float flicker = flashlightFlicker(m_round.battery, m_round.animationSeconds, m_gameplay);
    m_gameplayRenderer.drawHeldLamp(shader, lampMatrix,
                                    heldLampGlow(frameLighting, m_round.battery, flicker));

    // Put back for whatever draws next: the whole depth range, and the highlight of the
    // stone.
    GL_CHECK(glDepthRange(0.0, 1.0));
    if (!unlit) {
        shader.setFloat(SPECULAR_STRENGTH_UNIFORM, m_lighting.specularStrength);
    }
}

void NightMazeApp::drawGrass(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_grassSettings.enabled) {
        return;
    }

    // The grass has one program for every lighting mode. It is lit per fragment in the
    // three lit modes (with Gouraud too: a tuft has no vertices in any buffer that
    // light could be computed at), and shown at full brightness in the mode Unlit.
    const bool lit = m_lighting.mode != LightingMode::Unlit;
    // The clock of the wind: the seconds since GLFW was started. It only has to keep
    // growing, so it does not stop when the round is won and does not jump when one is
    // restarted.
    const auto windSeconds = static_cast<float>(glfwGetTime());

    // The grass lies in the shadows of the moon and of the flashlight like the ground,
    // so its program gets the uniforms of the two shadow maps too. A uniform is written
    // into the program in use, hence use() here: GrassRenderer::draw calls it again,
    // which changes nothing.
    if (m_grassShader.isValid()) {
        m_grassShader.use();
        setShadowUniformsOf(m_grassShader);
    }
    m_grassRenderer.draw(m_grassShader, view, projection, m_grassSettings, windSeconds, lit,
                         m_viewMode);
}

void NightMazeApp::drawColliderLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The depth test stays on: a line behind a wall is hidden by it, which shows where
    // each box really is. The box of the player is drawn at the simulation position (the
    // last fixed step), the camera at a blend of two steps, so while moving the box runs
    // ahead of the camera by a fraction of one step.
    //
    // The boxes of the maze come from the obstacle list of the round, so a wall that
    // a lever has opened is not drawn. While the gate blocks, its box is the last one of
    // that list (game::roundObstacles): it is left out here and drawn in its own colour
    // below.
    const std::size_t gateBoxes = gateBlocks(m_mazeWorld, m_round) ? 1 : 0;
    const std::span<const scene::Aabb> mazeBoxes =
        std::span<const scene::Aabb>(m_obstacles).first(m_obstacles.size() - gateBoxes);
    m_colliderLines.draw(m_colorShader, mazeBoxes, MAZE_COLLIDER_COLOR);
    // draw takes a list of boxes. A span made of a pointer and a count of 1 is a list
    // with this one box in it.
    const scene::Aabb playerBox = m_player.box();
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&playerBox, 1),
                         PLAYER_COLLIDER_COLOR);

    // The gate, while it is an obstacle, and the zone behind it that wins the round.
    if (gateBlocks(m_mazeWorld, m_round)) {
        m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.gateBox, 1),
                             GATE_COLLIDER_COLOR);
    }
    m_colliderLines.draw(m_colorShader, std::span<const scene::Aabb>(&m_mazeWorld.exitZone, 1),
                         EXIT_ZONE_COLOR);

    // The spheres of the pickup test: the reach of the player and, around every crystal
    // that is still there, the sphere the reach has to overlap. They stay on the
    // resting place of the crystal while the crystal itself bobs.
    const scene::Sphere reach = playerReach(m_player.position);
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&reach, 1),
                                PLAYER_COLLIDER_COLOR);
    std::vector<scene::Sphere> pickupSpheres;
    for (const RoundCrystal& crystal : m_round.crystals) {
        if (!crystal.collected) {
            pickupSpheres.push_back(
                {.center = crystalCenter(crystal.restPosition), .radius = m_gameplay.pickupRadius});
        }
    }
    m_colliderLines.drawSpheres(m_colorShader, pickupSpheres, PICKUP_COLLIDER_COLOR);
}

void NightMazeApp::drawPickLines(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_colorShader.isValid()) {
        return;
    }

    m_colorShader.use();
    m_colorShader.setMat4(VIEW_UNIFORM, view);
    m_colorShader.setMat4(PROJECTION_UNIFORM, projection);

    // The pick boxes: what the ray has to hit. They are larger than the models on
    // purpose, and they reach out of the collision box of the wall they hang on.
    const Interactables& interactables = m_mazeWorld.interactables;
    std::vector<scene::Aabb> leverBoxes;
    leverBoxes.reserve(interactables.levers.size());
    for (const Lever& lever : interactables.levers) {
        leverBoxes.push_back(lever.box);
    }
    std::vector<scene::Aabb> noteBoxes;
    noteBoxes.reserve(interactables.notes.size());
    for (const Note& note : interactables.notes) {
        noteBoxes.push_back(note.box);
    }

    // The ray that is shown: the one of this frame, or the frozen one. The box it hit
    // is drawn FIRST, in the colour of a hit. The same box follows below in the colour
    // of its kind, exactly on top of these lines: the depth test keeps a fragment only
    // when it is nearer than what is there, so the lines drawn first stay. The index is
    // checked: the frozen ray may be older than the maze.
    const PickState& shown = m_shownPick;
    const PickedInteractable& picked = shown.picked;
    const bool hitLever =
        shown.hasRay && picked.kind == InteractableKind::Lever && picked.index < leverBoxes.size();
    const bool hitNote =
        shown.hasRay && picked.kind == InteractableKind::Note && picked.index < noteBoxes.size();
    if (hitLever) {
        m_colliderLines.draw(m_colorShader,
                             std::span<const scene::Aabb>(&leverBoxes[picked.index], 1),
                             PICK_HIT_COLOR);
    }
    if (hitNote) {
        m_colliderLines.draw(m_colorShader,
                             std::span<const scene::Aabb>(&noteBoxes[picked.index], 1),
                             PICK_HIT_COLOR);
    }
    m_colliderLines.draw(m_colorShader, leverBoxes, LEVER_PICK_COLOR);
    m_colliderLines.draw(m_colorShader, noteBoxes, NOTE_PICK_COLOR);
    if (!shown.hasRay) {
        return;
    }

    // The ray itself: from the eye to the point where it enters the box it hit, or as
    // far as the player can reach when it hit nothing. A small sphere marks its end.
    // Seen from the eye it started in, the line is a single point behind the crosshair:
    // freeze it and step aside to see it.
    const bool hit = hitLever || hitNote;
    const float length = hit ? picked.distance : INTERACTION_REACH;
    const glm::vec3 end = shown.ray.origin + shown.ray.direction * length;
    const glm::vec3 color = hit ? PICK_HIT_COLOR : PICK_MISS_COLOR;
    m_colliderLines.drawLine(m_colorShader, shown.ray.origin, end, color);
    const scene::Sphere marker{.center = end, .radius = PICK_MARKER_RADIUS};
    m_colliderLines.drawSpheres(m_colorShader, std::span<const scene::Sphere>(&marker, 1), color);
}

} // namespace game
