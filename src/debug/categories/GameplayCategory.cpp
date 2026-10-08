// "Gameplay" category of the debug window: the state of the round, the battery, the
// numbers of the rules, the shade and the minimap.
// See docs/modules/game/gameplay.md
#include "debug/categories/GameplayCategory.hpp"

#include "debug/DebugContext.hpp"
#include "debug/Pictures.hpp"
#include "debug/Widgets.hpp"
#include "game/Minimap.hpp"
#include "game/MinimapRenderer.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "gfx/Framebuffer.hpp"

#include <imgui.h>

#include <algorithm>

namespace debug {

namespace {

// The charge of the battery: from empty to full.
constexpr float MIN_BATTERY = 0.0F;
constexpr float MAX_BATTERY = 1.0F;

// The part of the crystals that opens the gate. Not down to 0: at least one crystal is
// always needed (game::requiredCrystalCount), so smaller values would change nothing.
constexpr float MIN_REQUIRED_FRACTION = 0.05F;
constexpr float MAX_REQUIRED_FRACTION = 1.0F;

// How long a full battery lasts, in seconds: from a few seconds, to see it run out
// while watching, to ten minutes.
constexpr float MIN_BATTERY_LIFETIME = 5.0F;
constexpr float MAX_BATTERY_LIFETIME = 600.0F;

// How much of a full battery one crystal gives back.
constexpr float MIN_BATTERY_PER_CRYSTAL = 0.0F;
constexpr float MAX_BATTERY_PER_CRYSTAL = 1.0F;

// The charge below which the flashlight flickers. 0 switches the flicker off.
constexpr float MIN_LOW_BATTERY_THRESHOLD = 0.0F;
constexpr float MAX_LOW_BATTERY_THRESHOLD = 0.5F;

// The radius of the pickup sphere of a crystal, in metres. At 2 m a crystal is collected
// from the neighbouring cell, through the wall.
constexpr float MIN_PICKUP_RADIUS = 0.1F;
constexpr float MAX_PICKUP_RADIUS = 2.0F;

constexpr float PERCENT = 100.0F;

// The speed of the shade in metres per second: from standing still to faster than the
// player sprints (5.5).
constexpr float MIN_SHADE_SPEED = 0.0F;
constexpr float MAX_SHADE_SPEED = 8.0F;

// How long the shade stands still after a round starts, in seconds.
constexpr float MIN_SHADE_GRACE = 0.0F;
constexpr float MAX_SHADE_GRACE = 30.0F;

// How near the shade has to come to catch the player, in metres. At 2 m it catches from
// the middle of the next cell.
constexpr float MIN_SHADE_CATCH_DISTANCE = 0.3F;
constexpr float MAX_SHADE_CATCH_DISTANCE = 2.0F;

// The picture of the minimap in its card is at most this wide, in pixels at 100 %
// display scaling.
constexpr float MAX_MINIMAP_PICTURE_WIDTH = 200.0F;

// What the round is doing, in words.
const char* stateName(game::RoundState state) {
    return state == game::RoundState::Won ? "won" : "playing";
}

// The round as it is, and the two buttons that act on it.
void drawRound(Page& page, const DebugContext& context) {
    const game::Round& round = context.round;
    page.beginCard("Round");

    page.stat("Round", "%s, %.1f s", stateName(round.state), round.elapsedSeconds);
    page.stat("Crystals", "%d collected, %d needed, %d in the maze", round.collectedCount,
              round.requiredCount, static_cast<int>(round.crystals.size()));
    if (!round.gateOpen) {
        page.stat("Gate", "closed");
    } else if (round.gateProgress < 1.0F) {
        page.stat("Gate", "opening, %.0f%%", round.gateProgress * PERCENT);
    } else {
        page.stat("Gate", "open");
    }
    // The levers of the maze and how many of them are pulled. Each pulled lever is one
    // wall that is open. The state has one entry per lever.
    page.stat("Levers", "%d pulled of %d", game::pulledLeverCount(round),
              static_cast<int>(round.interactables.leverPulled.size()));
    page.stat("Note card", "%s", round.noteOpen ? "open" : "closed");
    page.stat("Flasks", "%d picked up of %d", round.flasksCollected,
              static_cast<int>(round.flasks.size()));

    // The buttons only ask: the game starts the round, or pulls the levers, at the
    // start of its next frame. Pulling every lever is a switch for testing: it opens
    // every shortcut without walking to the levers. A restart closes the walls again.
    game::GameplaySettings& settings = context.gameplay;
    const int clicked = page.buttons("Round actions", "Restart round (key R)", "Pull all levers",
                                     "Restart starts the round again in the same maze. Pull "
                                     "all levers opens every wall a lever controls, for "
                                     "testing.");
    if (clicked == 1) {
        settings.restart = true;
    }
    if (clicked == 2) {
        settings.pullAllLevers = true;
    }

    page.endCard();
}

// The battery: its charge can be dragged, to look at a low and at an empty battery.
void drawBattery(Page& page, const DebugContext& context) {
    page.beginCard("Battery");
    page.slider("Battery", &context.round.battery, MIN_BATTERY, MAX_BATTERY, "%.2f",
                "The charge of the battery of the flashlight: 1 is full, 0 is empty. "
                "Editable here, to look at a low and at an empty battery.");
    page.toggle("Battery drains", &context.gameplay.batteryDrains,
                "Off: the charge stays where the slider put it and the flashlight never "
                "runs down.");
    page.endCard();
}

// The numbers of the rules (game::GameplaySettings).
void drawRules(Page& page, const DebugContext& context) {
    game::GameplaySettings& settings = context.gameplay;
    page.beginCard("Rules");
    page.slider("Crystals needed", &settings.requiredFraction, MIN_REQUIRED_FRACTION,
                MAX_REQUIRED_FRACTION, "%.2f",
                "The part of all the crystals of the maze that opens the gate. At least "
                "one crystal is always needed.");
    page.slider("Battery lifetime", &settings.batteryLifetimeSeconds, MIN_BATTERY_LIFETIME,
                MAX_BATTERY_LIFETIME, "%.0f s",
                "How long a full battery lasts, in seconds. The bar is coarse for this "
                "range: Ctrl and click on it to type a number.");
    page.slider("Recharge", &settings.batteryPerCrystal, MIN_BATTERY_PER_CRYSTAL,
                MAX_BATTERY_PER_CRYSTAL, "%.2f",
                "What one collected crystal gives back, as a part of a full battery.");
    page.slider("Flicker below", &settings.lowBatteryThreshold, MIN_LOW_BATTERY_THRESHOLD,
                MAX_LOW_BATTERY_THRESHOLD, "%.2f",
                "The charge below which the flashlight flickers and the battery bar of "
                "the HUD turns red. 0 switches the flicker off.");
    page.slider("Pickup radius", &settings.pickupRadius, MIN_PICKUP_RADIUS, MAX_PICKUP_RADIUS,
                "%.2f m",
                "How near the player has to be to collect a crystal. At 2 m a crystal is "
                "collected from the neighbouring cell, through the wall.");
    page.sliderInt("Flasks", &settings.flaskCount, 0, game::MAX_FLASK_COUNT, "%d",
                   "How many flasks of tea lie in the maze: in dead ends without a crystal "
                   "first, and in other free cells when none is left. "
                   "A new number shows when the round starts again (key R). A new game sets "
                   "the number of its difficulty level.");
    page.endCard();
}

// The shade: where it is, whether the flashlight is on it, and its numbers
// (game::ShadeSettings).
void drawShade(Page& page, const DebugContext& context) {
    const game::Shade& shade = context.round.shade;
    game::ShadeSettings& settings = context.gameplay.shade;
    page.beginCard("Shade");

    if (!shade.present) {
        page.stat("Shade", "none in this round");
    } else if (shade.graceLeft > 0.0F) {
        page.stat("Shade", "waiting, %.1f s of grace left", shade.graceLeft);
    } else {
        page.stat("Shade", "%s", shade.lit ? "lit: standing still" : "unlit: walking");
    }
    if (shade.present) {
        // Straight through the walls, and along the passages it has to walk.
        page.stat("Distance", "%.1f m", game::shadeDistance(shade, context.player.position));
        if (shade.wayMetres < game::SHADE_NO_WAY) {
            page.stat("Way to the player", "%.1f m", shade.wayMetres);
        } else {
            page.stat("Way to the player", "none");
        }
    }

    page.toggle("Shade in the round", &settings.enabled,
                "Off: the shade is gone at once, like on a calm night. On: it comes with the "
                "next round (key R). A new game sets the switch from the menu: off for "
                "a calm night.");
    page.slider("Speed", &settings.speed, MIN_SHADE_SPEED, MAX_SHADE_SPEED, "%.1f m/s",
                "How fast the shade walks while it is not lit. The player walks 3.0 and "
                "sprints 5.5 m/s.");
    page.slider("Grace time", &settings.graceSeconds, MIN_SHADE_GRACE, MAX_SHADE_GRACE, "%.0f s",
                "How long the shade stands still after a round starts or starts again. "
                "A new number counts from the next round.");
    page.slider("Catch distance", &settings.catchDistance, MIN_SHADE_CATCH_DISTANCE,
                MAX_SHADE_CATCH_DISTANCE, "%.2f m",
                "The shade has caught the player when it is this near, measured on the "
                "ground from middle to middle.");
    page.toggle("Show shade on the map", &settings.showOnMap,
                "Debug switch: mark the shade on the map (key M). The map of the game "
                "never shows it.");

    page.endCard();
}

// The minimap: its settings (game::MinimapSettings), the facts of its framebuffer and
// the picture of that framebuffer.
void drawMinimap(Page& page, const DebugContext& context) {
    game::MinimapSettings& minimap = context.minimapSettings;
    const gfx::Framebuffer& target = context.minimap.target();
    page.beginCard("Minimap");

    page.toggle("Pin map open", &minimap.pinned,
                "Debug switch: keep the map on the screen without holding M, for "
                "screenshots and tuning. It counts like the held key: the player stands "
                "still and does not look around until it is cleared.");
    page.toggle("Reveal all", &minimap.revealAll,
                "Debug switch: show the whole maze. Without it the map shows only the "
                "cells the player has seen along the corridors.");
    page.slider("Size", &minimap.size, game::MIN_MINIMAP_SIZE, game::MAX_MINIMAP_SIZE, "%.2f",
                "The side of the map as a part of the height of the window. Its "
                "framebuffer gets exactly that many pixels.");
    page.slider("Opacity", &minimap.opacity, game::MIN_MINIMAP_OPACITY, game::MAX_MINIMAP_OPACITY,
                "%.2f", "How much the map hides of the scene behind it. 1 hides it completely.");

    // The framebuffer is filled only in the frames that show the map, so the picture
    // below is the map as it was shown the last time.
    if (target.isValid()) {
        page.stat("Framebuffer", "%d x %d px, %s", target.width(), target.height(),
                  gfx::colorFormatName(target.colorFormat()));
    } else {
        page.stat("Framebuffer", "not drawn yet (hold M in a round)");
    }

    if (page.beginBlock("minimap picture framebuffer")) {
        const float width =
            std::min(ImGui::GetContentRegionAvail().x, MAX_MINIMAP_PICTURE_WIDTH * displayScale());
        drawFramebufferPicture("Minimap framebuffer",
                               "The framebuffer of the minimap: sRGB colours, shown as stored. "
                               "It is the map as it was shown the last time.",
                               target, width, target.isValid());
    }

    page.endCard();
}

} // namespace

void drawGameplayCategory(Page& page, const DebugContext& context) {
    page.setPlace("Gameplay");
    page.beginColumns();
    drawRound(page, context);
    drawBattery(page, context);
    drawRules(page, context);
    page.nextColumn();
    drawShade(page, context);
    drawMinimap(page, context);
    page.endColumns();
}

} // namespace debug
