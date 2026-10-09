// "World" category of the debug window: the maze and its plan, the terrain, the grass,
// and how the crystals and the puddles show the sky.
#include "debug/categories/WorldCategory.hpp"

#include "debug/DebugContext.hpp"
#include "debug/MazePlan.hpp"
#include "debug/Widgets.hpp"
#include "game/EnvironmentMapping.hpp"
#include "game/Grass.hpp"
#include "game/Interactables.hpp"
#include "game/MazeWorld.hpp"
#include "game/Terrain.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>

namespace debug {

namespace {

// ---- Maze ------------------------------------------------------------------------------

// Limits of the size sliders, in cells. The game draws every wall and pillar with its
// own draw call, about two per cell, and the terrain has 32 triangles per cell, so a much
// larger maze would make the frame slow. game::Maze itself accepts up to Maze::MAX_SIZE.
constexpr int MIN_MAZE_SIZE = 2;
constexpr int MAX_MAZE_SIZE = 40;

// The seed field changes by this much for one click on its + or - button.
constexpr std::uint32_t SEED_STEP = 1;

// Width of the seed field with its two buttons, in pixels at 100 % display scaling.
constexpr float SEED_FIELD_WIDTH = 142.0F;

// The plan is at most this wide, in pixels at 100 % display scaling.
constexpr float MAX_PLAN_WIDTH = 280.0F;

// ---- Terrain and grass -----------------------------------------------------------------

// The smallest height scale of the slider: a flat world. The largest one is
// game::MAX_HEIGHT_SCALE.
constexpr float MIN_HEIGHT_SCALE = 0.0F;

// The density, in tufts per metre of wall and side. 0 plants nothing. The largest one is
// game::MAX_GRASS_DENSITY.
constexpr float MIN_DENSITY = 0.0F;

// The height of the tallest blades, in metres: from stubble to knee high.
constexpr float MIN_BLADE_HEIGHT = 0.05F;
constexpr float MAX_BLADE_HEIGHT = 0.8F;

// The strength of the wind: 0 is still air, 1 the default breeze.
constexpr float MIN_WIND_STRENGTH = 0.0F;
constexpr float MAX_WIND_STRENGTH = 3.0F;

// The blades one tuft is made of: BLADE_COUNT in assets/shaders/grass.geom. It is only
// used to show a number, so a wrong value here changes nothing that is drawn.
constexpr int BLADES_PER_TUFT = 3;

// ---- Reflections -----------------------------------------------------------------------

// A share goes from nothing to everything: the sky in the colour of a surface, the
// mirrored picture in what a crystal shows, the glow a crystal keeps.
constexpr float MIN_SHARE = 0.0F;
constexpr float MAX_SHARE = 1.0F;

// The part of the free cells that gets a puddle. 0 places none. The largest one is
// game::MAX_PUDDLE_SHARE.
constexpr float MIN_PUDDLE_SHARE = 0.0F;

// The request for the next maze. The rows edit the request, not the maze: nothing
// happens until one of the buttons sets settings.regenerate.
void drawMazeRequest(Page& page, game::MazeSettings& settings) {
    page.beginCard("Next maze");

    page.sliderInt("Width", &settings.width, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                   "The width of the next maze, in cells. It counts when Regenerate is "
                   "clicked.");
    page.sliderInt("Height", &settings.height, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                   "The height of the next maze, in cells. It counts when Regenerate is "
                   "clicked.");

    // InputScalar edits a number of any type through a pointer: the type is named by
    // the second argument and must match the variable, here a 32 bit unsigned. The
    // fourth argument is the step of its + and - buttons.
    const float seedWidth = SEED_FIELD_WIDTH * displayScale();
    if (page.beginRow("Seed",
                      "The maze is built from this number: the same seed gives the same "
                      "maze, with the same crystals, levers, notes and puddles.",
                      seedWidth)) {
        ImGui::SetNextItemWidth(seedWidth);
        ImGui::InputScalar("##seed", ImGuiDataType_U32, &settings.seed, &SEED_STEP);
        page.endRow();
    }

    // How many levers and notes the next maze should get. A maze can end up with
    // fewer: it only gets a lever for a wall that is worth opening.
    page.sliderInt("Levers", &settings.interactables.leverCount, 0, game::MAX_LEVER_COUNT, "%d",
                   "How many levers the next maze should get. A maze can end up with "
                   "fewer: it only gets a lever for a wall that is worth opening.");
    page.sliderInt("Notes", &settings.interactables.noteCount, 0, game::MAX_NOTE_COUNT, "%d",
                   "How many notes the next maze should get. A maze can end up with fewer.");

    const int clicked = page.buttons("Generate", "Regenerate", "Random seed",
                                     "Regenerate builds a new maze from the numbers above "
                                     "and starts a new round. Random seed picks a new seed "
                                     "first.");
    if (clicked == 1) {
        settings.regenerate = true;
    }
    if (clicked == 2) {
        // std::random_device asks the operating system for a number that cannot be
        // predicted. It only picks the seed: the maze itself is still built by the
        // seeded generator, so writing the seed down brings the same maze back.
        std::random_device device;
        settings.seed = static_cast<std::uint32_t>(device());
        settings.regenerate = true;
    }

    page.endCard();
}

// The maze in play, which may differ from the request until a button is clicked.
void drawMazeInPlay(Page& page, const game::MazeWorld& world, game::MazeSettings& settings) {
    page.beginCard("In play");
    page.stat("Maze", "%d x %d cells, seed %u", world.maze.width(), world.maze.height(),
              static_cast<unsigned int>(world.seed));
    page.stat("Walls, pillars", "%d walls, %d pillars", static_cast<int>(world.walls.size()),
              static_cast<int>(world.pillars.size()));
    // The crystals are chosen from the seed together with the maze. How many of them
    // open the gate is a rule of the round (the Gameplay category).
    page.stat("Crystals, exit", "%d crystals, exit in cell (%d, %d)",
              static_cast<int>(world.crystals.size()), world.exitCell.x, world.exitCell.z);
    // The levers and the notes the maze really got, which can be fewer than asked.
    page.stat("Levers, notes", "%d levers, %d notes",
              static_cast<int>(world.interactables.levers.size()),
              static_cast<int>(world.interactables.notes.size()));
    // The worn walls. The switch only changes what is drawn: the looks stay chosen, so
    // the numbers below it do not change with it.
    page.toggle("Wall variants", &settings.wallVariants,
                "Some walls are drawn cracked, mossy or damaged (other textures), crowned "
                "with stone twigs or with a broken coping (other models), chosen from the "
                "seed. Off: every wall is plain.");
    const std::array<int, game::WALL_VARIANT_COUNT> looks =
        game::countWallVariants(world.wallVariants);
    page.stat("Wall looks", "%d plain, %d cracked, %d mossy, %d damaged, %d crowned, %d broken",
              looks[0], looks[1], looks[2], looks[3], looks[4], looks[5]);
    page.endCard();
}

// The plan of the maze in play, seen from above.
void drawPlan(Page& page, const DebugContext& context) {
    page.beginCard("Plan");
    if (page.beginBlock("plan map top down walls crystals levers notes gate exit player")) {
        // As wide as the card, up to a limit: a plan as wide as the search results
        // would be taller than the window.
        const float width =
            std::min(ImGui::GetContentRegionAvail().x, MAX_PLAN_WIDTH * displayScale());
        drawMazePlan(context.mazeWorld, context.round, context.player, context.camera, width);
    }
    page.note("Amber: the player and where the camera looks. Teal: crystals. Red: levers. "
              "Pale yellow: notes. Brown: the gate. Green: the exit. What is collected, "
              "pulled or opened is drawn dim.");
    page.endCard();
}

void drawMazeTab(Page& page, const DebugContext& context) {
    page.setPlace("World / Maze");
    page.beginColumns();
    drawMazeRequest(page, context.mazeSettings);
    drawMazeInPlay(page, context.mazeWorld, context.mazeSettings);
    page.nextColumn();
    drawPlan(page, context);
    page.endColumns();
}

// The height scale and the wireframe switch of the terrain, and the size of its grid.
void drawTerrain(Page& page, const DebugContext& context) {
    game::TerrainSettings& settings = context.terrain;
    const game::Terrain& terrain = context.mazeWorld.terrain;
    page.beginCard("Terrain");

    // slider returns true in every frame in which the value changed, so the terrain
    // follows the slider while it is dragged. The row only asks: the game builds the
    // terrain at the start of its next frame.
    if (page.slider("Height scale", &settings.heightScale, MIN_HEIGHT_SCALE, game::MAX_HEIGHT_SCALE,
                    "%.2f",
                    "Every height of the terrain is multiplied by this number. 0 is a flat "
                    "world. The walls, the gate, the crystals and the player are put on "
                    "the new ground at once.")) {
        settings.rebuild = true;
    }
    page.toggle("Wireframe", &settings.wireframe,
                "Draws the edges of the triangles of the terrain instead of their faces "
                "(glPolygonMode). Everything else stays filled.");
    page.stat("Grid", "%d x %d points, %.2f m apart", terrain.columns(), terrain.rows(),
              terrain.spacing());
    page.stat("Triangles", "%d", static_cast<int>(terrain.triangleCount()));
    page.stat("Height", "%.2f m to %.2f m", terrain.minHeight(), terrain.maxHeight());

    page.endCard();
}

// The switch, the density, the blade height and the wind of the grass.
void drawGrass(Page& page, const DebugContext& context) {
    game::GrassSettings& settings = context.grass;
    page.beginCard("Grass");

    page.toggle("Grass", &settings.enabled,
                "Blades of grass at the foot of the walls and on the hills. The geometry "
                "shader makes the blades of every tuft.");
    // The row only asks: the game places the tufts at the start of its next frame.
    if (page.slider("Density", &settings.density, MIN_DENSITY, game::MAX_GRASS_DENSITY,
                    "%.1f per m",
                    "Tufts per metre of wall, on each side of the wall. The scatter on the "
                    "hills follows in proportion.")) {
        settings.replant = true;
    }
    // These two are uniforms of the grass program: they change the blades the geometry
    // shader builds, and no tuft has to be placed again.
    page.slider("Blade height", &settings.bladeHeight, MIN_BLADE_HEIGHT, MAX_BLADE_HEIGHT, "%.2f m",
                "The height of the tallest blades: from stubble to knee high.");
    page.slider("Wind strength", &settings.windStrength, MIN_WIND_STRENGTH, MAX_WIND_STRENGTH,
                "%.2f", "How far the blades sway: 0 is still air, 1 the default breeze.");
    // One tuft is one point in the vertex buffer. The blades are made of it by the
    // geometry shader, so their number is not stored anywhere.
    const int tufts = static_cast<int>(context.grassTuftCount);
    page.stat("Tufts", "%d (%d blades)", tufts, tufts * BLADES_PER_TUFT);

    page.endCard();
}

void drawTerrainAndGrassTab(Page& page, const DebugContext& context) {
    page.setPlace("World / Terrain and grass");
    page.beginColumns();
    drawTerrain(page, context);
    page.nextColumn();
    drawGrass(page, context);
    page.endColumns();
}

// The switch of the environment mapping and how the crystals show the sky. These are
// uniforms of the reflect program: a change shows in the next frame.
void drawCrystals(Page& page, game::EnvironmentSettings& settings) {
    page.beginCard("Crystals");
    page.toggle("Environment mapping", &settings.enabled,
                "The crystals and the puddles show the sky: the cube map of the skybox, "
                "read in the direction of the mirrored or of the refracted ray. Only the "
                "sky: walls are never mirrored. Off: the crystals are drawn like the "
                "walls, no puddles.");
    page.slider("Sky share", &settings.crystalStrength, MIN_SHARE, MAX_SHARE, "%.2f",
                "How much of the lit colour of a crystal is replaced by the sky it shows. "
                "Its glow is added on top.");
    page.slider("Refract / reflect", &settings.crystalReflectShare, MIN_SHARE, MAX_SHARE, "%.2f",
                "0: the sky seen through the crystal (refraction). 1: the sky mirrored on "
                "it (reflection).");
    page.slider("Refraction ratio", &settings.crystalRefractionRatio, game::MIN_REFRACTION_RATIO,
                game::MAX_REFRACTION_RATIO, "%.2f",
                "n1 / n2 of Snell's law (eta of GLSL refract). Air into glass: 1 / 1.5 = "
                "0.67. Water: 0.75. Diamond: 0.41. 1 does not bend the ray. Above 1 the "
                "ray would leave a denser material: flat rays are then mirrored instead "
                "(total internal reflection).");
    page.slider("Glow", &settings.crystalGlowShare, MIN_SHARE, MAX_SHARE, "%.2f",
                "How much of its own glow a crystal keeps. The glow is far brighter than "
                "the night sky: turn it down to see the sky on a crystal plainly. The "
                "halo of the bloom fades with it.");
    page.endCard();
}

// The puddles: where they lie and how they mirror the sky.
void drawPuddles(Page& page, const DebugContext& context) {
    game::EnvironmentSettings& settings = context.environment;
    page.beginCard("Puddles");
    page.toggle("Puddles", &settings.puddles,
                "A puddle is a thin film of water that follows the ground, with a level "
                "mirror on it and a rim that fades out. Puddles need the environment "
                "mapping switched on.");
    // The row only asks: the game places the puddles at the start of its next frame.
    if (page.slider("Share of cells", &settings.puddleShare, MIN_PUDDLE_SHARE,
                    game::MAX_PUDDLE_SHARE, "%.2f",
                    "The part of the free cells (not the start, not the exit, no crystal) "
                    "that gets a puddle. The cells come from the seed of the maze. A larger "
                    "share keeps the puddles and adds more.")) {
        settings.replacePuddles = true;
    }
    page.slider("Reflectivity", &settings.puddleReflectivity, MIN_SHARE, MAX_SHARE, "%.2f",
                "How much of the sky a puddle shows when looked at straight from above. "
                "Real water: 0.02. The rest is the water and the ground that shows "
                "through it.");
    page.toggle("Fresnel", &settings.puddleFresnel,
                "On: a puddle mirrors more the flatter it is looked at (Schlick's "
                "formula), up to a full mirror far ahead, and hides more of the ground. "
                "Off: the reflectivity above at every angle.");
    page.stat("Puddles", "%d", static_cast<int>(context.puddleCount));
    page.endCard();
}

void drawReflectionsTab(Page& page, const DebugContext& context) {
    page.setPlace("World / Reflections");
    page.beginColumns();
    drawCrystals(page, context.environment);
    page.nextColumn();
    drawPuddles(page, context);
    page.endColumns();
}

} // namespace

void drawWorldCategory(Page& page, const DebugContext& context, WorldTab tab) {
    // While the user searches, the rows of every tab can be found.
    const bool all = page.searching();
    if (all || tab == WorldTab::Maze) {
        drawMazeTab(page, context);
    }
    if (all || tab == WorldTab::TerrainAndGrass) {
        drawTerrainAndGrassTab(page, context);
    }
    if (all || tab == WorldTab::Reflections) {
        drawReflectionsTab(page, context);
    }
}

} // namespace debug
