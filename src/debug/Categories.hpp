// Categories of the debug window: their names, icons and tabs, as plain data.
// See docs/modules/debug-ui.md
#pragma once

#include "debug/Icons.hpp"

#include <array>
#include <cstddef>

namespace debug {

/// The seven pages of the debug window, in the order of the icon rail. The number of
/// an entry is its place in CATEGORIES.
enum class Category {
    Render = 0,
    Light,
    PostProcess,
    World,
    Player,
    Gameplay,
    Diagnostics,
};

inline constexpr int CATEGORY_COUNT = 7;

/// The largest number of tabs a category has.
inline constexpr int MAX_TAB_COUNT = 3;

/// What the window shows about a category without drawing it: the rail, the header and
/// the tabs are made from this.
struct CategoryInfo {
    /// The name in the header and in the tooltip of the rail.
    const char* name;
    /// One sentence for the tooltip of the rail.
    const char* description;
    Icon icon;
    /// How many controls the category holds: everything that edits a value or starts
    /// an action, a button counting as one. Shown in the header. The code that draws
    /// the category does not read it, so it has to be kept right by hand.
    int controlCount;
    /// How many tabs the category is split into: 0 for a category that is one page.
    int tabCount;
    /// The names of the tabs. The entries after tabCount are not used.
    std::array<const char*, MAX_TAB_COUNT> tabs;
};

inline constexpr std::array<CategoryInfo, CATEGORY_COUNT> CATEGORIES = {{
    {.name = "Render",
     .description = "Lighting mode, sky, clear colour and texture filtering.",
     .icon = Icon::Render,
     .controlCount = 8,
     .tabCount = 0,
     .tabs = {}},
    {.name = "Light",
     .description = "Moon, flashlight, crystal lights and the shadows of the moon and of "
                    "the flashlight.",
     .icon = Icon::Light,
     .controlCount = 34,
     .tabCount = 2,
     .tabs = {"Lights", "Shadows"}},
    {.name = "Post process",
     .description = "Exposure, tone mapping, bloom, fog, vignette and the framebuffer "
                    "previews.",
     .icon = Icon::PostProcess,
     .controlCount = 15,
     .tabCount = 0,
     .tabs = {}},
    {.name = "World",
     .description = "The maze, the terrain, the grass, and the sky on crystals and puddles.",
     .icon = Icon::World,
     .controlCount = 23,
     .tabCount = 3,
     .tabs = {"Maze", "Terrain and grass", "Reflections"}},
    {.name = "Player",
     .description = "Position, camera, speeds, stamina, noclip and the menu camera.",
     .icon = Icon::Player,
     .controlCount = 16,
     .tabCount = 0,
     .tabs = {}},
    {.name = "Gameplay",
     .description = "The state of the round, the battery, the rules of a round, the shade, "
                    "the minimap, the intro and the campaign.",
     .icon = Icon::Gameplay,
     .controlCount = 30,
     .tabCount = 0,
     .tabs = {}},
    {.name = "Diagnostics",
     .description = "Frame statistics, the sound device, shader programs, collision and "
                    "picking, loaded assets.",
     .icon = Icon::Diagnostics,
     .controlCount = 5,
     .tabCount = 3,
     .tabs = {"Frame and shaders", "Collision and picking", "Assets"}},
}};

/// The entry of a category in CATEGORIES.
constexpr const CategoryInfo& categoryInfo(Category category) {
    return CATEGORIES[static_cast<std::size_t>(category)];
}

/// How many controls the whole debug window holds: the sum over the categories.
constexpr int totalControlCount() {
    int total = 0;
    for (const CategoryInfo& info : CATEGORIES) {
        total += info.controlCount;
    }
    return total;
}

// The thirteen panels this window replaced held 114 controls, and every one of them
// moved into a category. The switch of the wall variants was added later, which makes
// 115, the six controls of the shade make 121, and the button that plays the intro again
// 122, the five sliders of the sway of the shade 127, the button that plays the wind of
// the maze 128, and the three controls of the campaign (the next night, clear, win the
// round) 131. A control that is added or removed changes this number and the count of its
// category above.
static_assert(totalControlCount() == 131);

} // namespace debug
