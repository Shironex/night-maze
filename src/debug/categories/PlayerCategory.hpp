// "Player" category of the debug window: position and noclip, the angles and the
// projection of the camera, the speeds and the menu camera.
// See docs/modules/scene/camera-controls.md
#pragma once

namespace debug {

struct DebugContext;
class Page;

/// Draws the cards of the "Player" category onto the page. Called by the debug window,
/// inside the ImGui frame.
///
/// It edits, through the context: the position, the noclip mode and the three speeds of
/// the player, the angles and the projection of the camera, the mouse sensitivity and
/// the settings of the menu camera. The position of the eye and the length of one loop
/// of the menu camera are only shown.
void drawPlayerCategory(Page& page, const DebugContext& context);

} // namespace debug
