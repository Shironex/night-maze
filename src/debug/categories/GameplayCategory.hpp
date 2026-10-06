// "Gameplay" category of the debug window: the state of the round, the battery, the
// numbers of the rules and the minimap.
// See docs/modules/game/gameplay.md
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// Draws the cards of the "Gameplay" category onto the page. Called by the debug
/// window, inside the ImGui frame.
///
/// It edits, through the context: the charge of the battery (the one thing of the round
/// that can be changed), the numbers of the rules, the requests for a restart and for
/// pulling every lever, and the settings of the minimap. The state of the round and
/// the picture of the minimap are only shown.
void drawGameplayCategory(Page& page, const DebugContext& context);

} // namespace debug
