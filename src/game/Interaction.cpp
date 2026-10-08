// Interaction: what the picking ray of a frame points at in a round, what the player can
// do with it, and where the models of the levers and the notes hang.
#include "game/Interaction.hpp"

#include "scene/Transform.hpp"

#include <cmath>

namespace game {

namespace {

// The turn around the vertical axis that makes the +Z of a model point away from the
// wall on a side of a cell, in degrees.
//
// The model is built for the north wall: its +Z points south, into the cell, and the
// turn is 0. yawTowards counts the sides clockwise seen from above (North 0, East 90,
// South 180, West 270). A positive rotation around +Y turns COUNTER clockwise seen from
// above, hence the minus sign. For the east wall that gives -90 degrees, which takes
// +Z to -X: west, away from the east wall.
float mountYawDegrees(Direction side) {
    return -yawTowards(side);
}

} // namespace

scene::Ray rayFromEye(const scene::Ray& screenRay, const glm::vec3& eye) {
    return {.origin = eye, .direction = screenRay.direction};
}

Interaction interactionFor(const Round& round, const PickedInteractable& picked) {
    if (round.state != RoundState::Playing) {
        return Interaction::None;
    }
    // An open card comes first: the key closes it before it does anything else.
    if (round.noteOpen) {
        return Interaction::CloseNote;
    }
    if (picked.kind == InteractableKind::Lever) {
        // A lever can be pulled once. The index is checked before it is used: the
        // PickedInteractable may come from another maze.
        const bool known = picked.index < round.interactables.leverPulled.size();
        if (known && !round.interactables.leverPulled[picked.index]) {
            return Interaction::PullLever;
        }
        return Interaction::None;
    }
    if (picked.kind == InteractableKind::Note) {
        return Interaction::ReadNote;
    }
    return Interaction::None;
}

PickState pickInRound(const scene::Ray& ray, bool centered, const MazeWorld& world,
                      const Round& round, std::span<const scene::Aabb> obstacles) {
    PickState pick;
    pick.hasRay = true;
    pick.centered = centered;
    pick.ray = ray;
    pick.picked = pickInteractable(ray, world.interactables, obstacles, INTERACTION_REACH);
    pick.action = interactionFor(round, pick.picked);
    return pick;
}

PickState pickNothing(const Round& round) {
    PickState pick;
    pick.action = interactionFor(round, pick.picked);
    return pick;
}

bool interact(Round& round, const MazeWorld& world, const PickState& pick) {
    // Asked again here and not taken from pick.action: the round may have changed since
    // the PickState was made.
    const Interaction action = interactionFor(round, pick.picked);
    if (action == Interaction::CloseNote) {
        closeNote(round);
        return false;
    }
    if (action == Interaction::ReadNote) {
        readNote(round, world, pick.picked.index);
        return false;
    }
    if (action == Interaction::PullLever) {
        return pullRoundLever(round, world, pick.picked.index);
    }
    return false;
}

std::string_view interactionWords(Interaction action) {
    switch (action) {
    case Interaction::PullLever:
        return "pull lever";
    case Interaction::ReadNote:
        return "read note";
    case Interaction::CloseNote:
        return "close";
    case Interaction::None:
        break;
    }
    return {};
}

std::string interactionPrompt(Interaction action, std::string_view key) {
    const std::string_view words = interactionWords(action);
    if (words.empty()) {
        return {};
    }
    return std::string(key) + ": " + std::string(words);
}

glm::vec3 highlightGlow(float seconds) {
    // The sine swings between -1 and 1. Half of it plus one half swings between 0 and
    // 1: the place of this moment between the weakest and the strongest glow.
    const float swing = 0.5F + 0.5F * std::sin(HIGHLIGHT_PULSE_SPEED * seconds);
    const float glow = HIGHLIGHT_MIN_GLOW + (HIGHLIGHT_MAX_GLOW - HIGHLIGHT_MIN_GLOW) * swing;
    return HIGHLIGHT_COLOR * glow;
}

glm::mat4 mountModelMatrix(const glm::vec3& position, Direction side) {
    scene::Transform transform;
    transform.position = position;
    transform.rotationDegrees = {0.0F, mountYawDegrees(side), 0.0F};
    return transform.matrix();
}

glm::mat4 leverHandleMatrix(const Lever& lever, float handleProgress) {
    // The pivot: from the point on the wall face straight into the cell. The step
    // towards the wall is (columnStep, 0, rowStep), so into the cell is its opposite.
    const Direction side = lever.mount.side;
    const glm::vec3 towardsWall{static_cast<float>(columnStep(side)), 0.0F,
                                static_cast<float>(rowStep(side))};

    scene::Transform transform;
    transform.position = lever.position - towardsWall * LEVER_PIVOT_DEPTH;
    // The tilt of the handle: a straight blend between the two angles. Transform turns
    // around X before it turns around Y, so the handle is tilted in the space of the
    // model first and turned with the wall afterwards.
    const float tilt = glm::mix(LEVER_HANDLE_UP_DEGREES, LEVER_HANDLE_DOWN_DEGREES, handleProgress);
    transform.rotationDegrees = {tilt, mountYawDegrees(side), 0.0F};
    return transform.matrix();
}

} // namespace game
