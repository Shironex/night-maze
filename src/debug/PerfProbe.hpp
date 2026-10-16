// PerfProbe: a developer aid that measures the frame in a few fixed settings, one after
// another, and writes the averages to the log.
#pragma once

#include "gfx/RenderStats.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace core {
struct CpuTimes;
class Time;
} // namespace core

namespace game {
enum class GameMode;
class GpuPassTimers;
struct GrassSettings;
struct LightingSettings;
struct Player;
enum class ViewMode;
} // namespace game

namespace scene {
struct Camera;
} // namespace scene

namespace debug {

// A tool for measuring, not a part of the game. It is switched on by the command line
// switch --perf-probe <spec>, which main.cpp takes away before the switches of the game
// are read. Without the switch nothing of it runs.
//
// The numbers of the Diagnostics card jump from one half second to the next, and the
// card of an Apple GPU shows the scene and the grass as one time. A comparison of two
// settings therefore needs many seconds of each, measured from the same place, and the
// settings taken in turn, so that a card that gets warmer and slower (a laptop without
// a fan) slows all of them alike. The probe does exactly that and prints one line per
// measured window:
//
//     PERF config=<name> round=<n> frames=<n> frame_ms_mean=... frame_ms_median=...
//          gpu_<pass>=... cpu_render=... cpu_swap=... draws=... triangles=...
//          grass_draws=... grass_chunks=<drawn>/<tested> playing=<1 or 0>
//
// The spec is a list of name=value parts separated by ';':
//
//     configs=off,on        the settings to measure, in this order, in every round
//     seconds=6             how long each one is measured
//     warmup=2              how long each one runs before it is measured
//     rounds=3              how often the whole list is measured, every second time
//                           backwards
//     pose=x,y,z,yaw,pitch  where the player stands (feet, in metres) and where the eye
//                           looks (degrees). The player flies there (noclip), so the
//                           place can be in the air. Without it the player stays where
//                           the round started (--start-cell, --start-yaw).
//
// A config is one or more words joined by '+': "off" (no grass), "on", "geom" (the
// geometry shader), "inst" (instanced), "cull" and "nocull" (the chunk culling of the
// grass), "lod" and "nolod" (the distance LOD of the grass), "density=<n>" (tufts per
// metre of wall, which plants the grass again), "allcull" and "noallcull"
// (the frustum culling of the whole scene). For pictures only: "unlit" (the lighting mode
// Unlit, where the grass is shown at full brightness), "nowind" (still blades, so two
// pictures can be compared pixel by pixel) and "uvview" (the debug view of the
// texture coordinates, where every blade is a gradient from black to green).

/// Where the player stands and looks during a measurement.
struct PerfProbePose {
    glm::vec3 feet{0.0F};
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
};

/// What the switch --perf-probe asked for.
struct PerfProbeSpec {
    std::vector<std::string> configs;
    double seconds = 6.0;
    double warmupSeconds = 2.0;
    int rounds = 3;
    std::optional<PerfProbePose> pose;
};

/// Reads the text of the switch. Empty, with a message in error, when a part is not
/// understood.
std::optional<PerfProbeSpec> parsePerfProbeSpec(std::string_view text, std::string& error);

/// What the probe reads and changes in one frame. The references live as long as the
/// frame.
struct PerfProbeFrame {
    const core::Time& time;
    const core::CpuTimes& cpuTimes;
    const gfx::RenderCounters& counters;
    const game::GpuPassTimers& gpuPasses;
    game::GameMode mode;
    bool& vsync;
    bool& frustumCulling;
    game::GrassSettings& grass;
    game::LightingSettings& lighting;
    game::ViewMode& viewMode;
    game::Player& player;
    scene::Camera& camera;
};

/// Measures the configs of a spec one after another, see the comment at the top.
class PerfProbe {
public:
    explicit PerfProbe(PerfProbeSpec spec);

    /// Called once per frame, after the frame was drawn. Returns true when every window
    /// has been measured: the program should then close.
    bool afterFrame(const PerfProbeFrame& frame);

private:
    // The sums of one measured window.
    struct Window {
        std::vector<double> frameMs;
        std::vector<double> gpuSum;
        std::vector<int> gpuSamples;
        double cpuRender = 0.0;
        double cpuSwap = 0.0;
        int cpuSamples = 0;
        double drawCalls = 0.0;
        double triangles = 0.0;
        double grassDraws = 0.0;
        double grassTested = 0.0;
        double grassDrawn = 0.0;
        bool alwaysPlaying = true;
    };

    // The config of window number index.
    const std::string& configOf(int index) const;
    void applyConfig(const std::string& config, const PerfProbeFrame& frame) const;
    void printWindow(const std::string& config, int round) const;

    PerfProbeSpec m_spec;
    // Which window runs: round * configs + config. -1 before the round has started.
    int m_index = -1;
    double m_secondsInWindow = 0.0;
    int m_framesInRound = 0;
    Window m_window;
};

} // namespace debug
