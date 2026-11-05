// The clocks and counters of a played round that sit beside game::Round: the catch by the
// shade, the clocks of the repeating sounds, the bell of the gate.
#pragma once

#include "game/round/GateLamp.hpp"
#include "game/shade/Shade.hpp"
#include "game/sound/SoundCues.hpp"

#include <vector>

namespace game {

/// Simulation state of the application around the round: plain data, advanced by the fixed
/// steps of NightMazeAppRound.cpp and started anew by beginRound where it says so.
struct RoundClocks {
    /// Seconds since the shade reached the player, or -1 when no catch is going on. While
    /// it counts, the round stands still and the player cannot act: the picture fades to
    /// black, and at black the round starts again (carryPlayerBack). It counts in fixed
    /// steps of a played round, so it stands still in the pause.
    float catchSeconds = -1.0F;
    /// The clock of the low battery pulse. It is simulation state like the round:
    /// onUpdate advances it in fixed steps, and beginRound starts it anew.
    LowBatteryPulse lowBatteryPulse;
    /// The clock of the breathing of a winded player: the same kind of state.
    WindedBreath windedBreath;
    /// The clock of the bell of the open gate: the same kind of state.
    GateBell gateBell;
    /// Where the bell of the gatehouse is in its swing (game::advanceBellSwing): pushed by
    /// every toll, advanced in fixed steps wherever the picture moves, drawn by every pass.
    BellSwing bellSwing;
    /// For every cell of the maze, the passages from it to the exit cell
    /// (game::passageDistances, row after row): how loud a toll of the bell is played.
    /// Counted once per round and again when a lever opens a wall (measureExitDistances).
    std::vector<int> exitDistances;
    /// True when the player has pulled a lever since the last fixed step: the next step
    /// tells the shade, which may hear it (game::playerNoise). Keys are read once per
    /// frame and the shade moves in fixed steps, so the pull waits here in between.
    bool leverPulled = false;
    /// The clocks of the steps of the player and of the shade. They count metres and
    /// not seconds, and are the same kind of state too.
    StepClock footsteps;
    StepClock shadeSteps;
    /// The caught line that was shown last (game::caughtLine), so the next catch shows
    /// the next one. It lives as long as the program: a new round does not reset it, and
    /// it is not saved.
    int lastCaughtLine = NO_CAUGHT_LINE;
};

} // namespace game
