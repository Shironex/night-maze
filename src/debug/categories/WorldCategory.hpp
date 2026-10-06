// "World" category of the debug window: the maze and its plan, the terrain, the grass,
// and how the crystals and the puddles show the sky.
// See docs/modules/game/maze-generator.md
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// The tabs of the "World" category, in the order of CategoryInfo::tabs.
enum class WorldTab {
    Maze = 0,
    TerrainAndGrass,
    Reflections,
};

/// Draws the cards of one tab of the "World" category onto the page, or of all three
/// while the page is searching. Called by the debug window, inside the ImGui frame.
///
/// It edits, through the context: the request for the next maze (size, seed, numbers of
/// levers and notes, the "regenerate" flag), the height scale and the wireframe switch
/// of the terrain, the settings of the grass and the settings of the environment
/// mapping. The maze in play, its plan and the counts are only shown.
void drawWorldCategory(Page& page, const DebugContext& context, WorldTab tab);

} // namespace debug
