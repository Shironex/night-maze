// GpuPasses: how long the graphics card works on every pass of a frame.
#pragma once

#include "core/PeriodicAverage.hpp"
#include "gfx/RenderStats.hpp"
#include "gfx/TimerQuery.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace game {

/// The parts of a frame that are measured, in the order they are drawn. The number of
/// an entry is its place in the list of GpuPassTimers.
enum class GpuPass {
    /// The shadow map of the moon.
    MoonShadow = 0,
    /// The shadow map of the flashlight.
    FlashlightShadow,
    /// The start of the scene pass: the clear, the terrain, the walls and everything
    /// that stands in the maze.
    Scene,
    /// The grass.
    Grass,
    /// The rest of the scene pass: the crystals and the puddles that show the sky, the
    /// debug lines, the village, the sky and the lamp in the hand.
    Reflections,
    /// The bright pass and the blur passes of the bloom.
    Bloom,
    /// The pass that brings the picture to the window.
    Composite,
    /// The map: its own framebuffer, and its picture into the window.
    Minimap,
};

inline constexpr std::size_t GPU_PASS_COUNT = 8;

/// What is known about one pass after a frame.
struct GpuPassReading {
    /// True when the pass made at least one draw call in the last frame. False for
    /// a pass that was not reached (the map is not held) or that had nothing to draw
    /// (the flashlight is off, the grass or the bloom is switched off).
    bool ran = false;
    /// How many draw calls it made in the last frame.
    std::uint32_t drawCalls = 0;
    /// How many objects it tested against its frustum in the last frame, and how many
    /// of them it drew (gfx::RenderCounters::cullTested and cullDrawn). Both are 0 for
    /// a pass that does not cull and while the culling is switched off.
    std::uint32_t cullTested = 0;
    std::uint32_t cullDrawn = 0;
    /// The time the card worked on it, in milliseconds, averaged over the interval of
    /// the FPS display. Without a value in the first moments of a pass, and on a card
    /// that never answers.
    core::PeriodicAverage milliseconds;
};

/// Measures the passes of a frame with one gfx::TimerQuery each.
///
/// The frame calls beginFrame() once, and then begin(pass) in front of every pass and
/// end() after the last one of a group. OpenGL 4.1 runs one timer query at a time, so
/// the passes are ranges that follow each other: begin() ends the pass that was running.
/// That is why the grass is a pass of its own BETWEEN two parts of the scene and not
/// a part inside it.
///
/// A time is the answer of the card to the query of an earlier frame, read when it has
/// arrived (gfx::TimerQuery::takeResult), so measuring never makes the program wait.
/// The draw calls of a pass and the objects it tested against its frustum are counted
/// with gfx::renderCounters: the difference between its start and its end.
///
/// How finely the card can tell two ranges apart is up to the card. An Apple GPU does
/// the work of a framebuffer in one go when the framebuffer is left, and its driver
/// gives every range the time of the whole piece it has commands in: measured on an
/// Apple M3 (2026-10-12), Scene and Grass report the same number, the time of both
/// together, while the passes with a framebuffer of their own (the two shadow maps,
/// the bloom, the composite pass) are told apart.
///
/// It owns OpenGL objects, so it needs a current context for its whole life.
class GpuPassTimers {
public:
    /// Starts a frame: reads the answers that have arrived, and forgets which passes
    /// ran. publish is true in the frame in which the averages are to be refreshed
    /// (core::Time::fpsRefreshed). A pass that was left running is ended first.
    void beginFrame(bool publish);

    /// Ends the pass that is running, if there is one, and starts this one.
    void begin(GpuPass pass);

    /// Ends the pass that is running. Does nothing when none is.
    void end();

    /// What is known about a pass: of the frame that was just drawn.
    const GpuPassReading& reading(GpuPass pass) const;

private:
    struct Slot {
        gfx::TimerQuery query;
        GpuPassReading reading;
        // Whether the answer the query is waiting for belongs to a pass that drew
        // something. The time of an empty pass is thrown away: it would pull the
        // average towards 0 in the moment the pass is switched on.
        bool answerCounts = false;
    };

    std::array<Slot, GPU_PASS_COUNT> m_slots;
    // The pass between begin() and end(), and the counters at its begin().
    std::optional<GpuPass> m_running;
    gfx::RenderCounters m_countersAtBegin;
};

} // namespace game
