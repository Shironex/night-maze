// "Environment" debug panel: how the crystals and the puddles show the sky.
// See docs/modules/renderer/env-mapping.md
#pragma once

#include <cstddef>

namespace game {
struct EnvironmentSettings;
} // namespace game

namespace debug {

/// Draws the "Environment" panel. Called by DebugUI::draw, inside the ImGui frame.
///
/// settings is editable: the switch of the whole effect, for the crystals the share of
/// the sky in their colour, the blend of reflection and refraction, the refraction
/// ratio and the share of their glow, and for the puddles the switch, the share of the
/// cells that get one (a change sets settings.replacePuddles, and the game places the
/// puddles again at the start of its next frame), the reflectivity and the switch of
/// the Fresnel effect. puddleCount is the number of puddles in the maze: the panel only
/// shows it.
void drawEnvironmentPanel(game::EnvironmentSettings& settings, std::size_t puddleCount);

} // namespace debug
