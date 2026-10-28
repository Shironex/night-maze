// PerfProbe: a developer aid that measures the frame in a few fixed settings, one after
// another, and writes the averages to the log.
#include "debug/PerfProbe.hpp"

#include "core/Application.hpp"
#include "core/Time.hpp"
#include "game/GameState.hpp"
#include "game/GpuPasses.hpp"
#include "game/Grass.hpp"
#include "game/Lighting.hpp"
#include "game/MazeRenderer.hpp"
#include "game/Minimap.hpp"
#include "game/Player.hpp"
#include "game/Village.hpp"
#include "scene/Camera.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>

namespace debug {

namespace {

// The passes in the order of game::GpuPass, with the short names of the log line.
constexpr std::array<const char*, game::GPU_PASS_COUNT> PASS_NAMES = {
    "moon", "flashlight", "scene", "grass", "reflections", "bloom", "composite", "minimap"};

// The round needs a few frames before its picture is settled: the first frames load
// and compile on the card.
constexpr int FRAMES_BEFORE_FIRST_WINDOW = 30;

constexpr double MILLISECONDS_PER_SECOND = 1000.0;

// The parts of text between the separators, without empty ones.
std::vector<std::string> split(std::string_view text, char separator) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = std::min(text.find(separator, start), text.size());
        if (end > start) {
            parts.emplace_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    return parts;
}

bool parseNumber(std::string_view text, double& value) {
    // std::from_chars for double is missing in some standard libraries of macOS, so
    // the text goes through std::stod, which needs a string.
    try {
        std::size_t used = 0;
        value = std::stod(std::string(text), &used);
        return used == text.size();
    } catch (...) {
        return false;
    }
}

double median(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }
    std::ranges::sort(values);
    const std::size_t middle = values.size() / 2;
    return values.size() % 2 == 1 ? values[middle] : (values[middle - 1] + values[middle]) / 2.0;
}

} // namespace

std::optional<PerfProbeSpec> parsePerfProbeSpec(std::string_view text, std::string& error) {
    PerfProbeSpec spec;
    for (const std::string& part : split(text, ';')) {
        const std::size_t equals = part.find('=');
        if (equals == std::string::npos) {
            error = "--perf-probe: '" + part + "' has no '='";
            return std::nullopt;
        }
        const std::string name = part.substr(0, equals);
        const std::string value = part.substr(equals + 1);
        double number = 0.0;
        if (name == "configs") {
            spec.configs = split(value, ',');
        } else if (name == "seconds" && parseNumber(value, number) && number > 0.0) {
            spec.seconds = number;
        } else if (name == "warmup" && parseNumber(value, number) && number >= 0.0) {
            spec.warmupSeconds = number;
        } else if (name == "rounds" && parseNumber(value, number) && number >= 1.0) {
            spec.rounds = static_cast<int>(number);
        } else if (name == "shot" && !value.empty()) {
            spec.shotPath = value;
        } else if (name == "pose") {
            const std::vector<std::string> numbers = split(value, ',');
            std::array<double, 5> pose{};
            bool ok = numbers.size() == pose.size();
            for (std::size_t i = 0; ok && i < pose.size(); ++i) {
                ok = parseNumber(numbers[i], pose[i]);
            }
            if (!ok) {
                error = "--perf-probe: pose needs five numbers: x,y,z,yaw,pitch";
                return std::nullopt;
            }
            spec.pose =
                PerfProbePose{.feet = {static_cast<float>(pose[0]), static_cast<float>(pose[1]),
                                       static_cast<float>(pose[2])},
                              .yawDegrees = static_cast<float>(pose[3]),
                              .pitchDegrees = static_cast<float>(pose[4])};
        } else {
            error = "--perf-probe: '" + part + "' is not understood";
            return std::nullopt;
        }
    }
    if (spec.configs.empty()) {
        spec.configs = {"on"};
    }
    return spec;
}

PerfProbe::PerfProbe(PerfProbeSpec spec) : m_spec(std::move(spec)) {
    m_window.gpuSum.assign(game::GPU_PASS_COUNT, 0.0);
    m_window.gpuSamples.assign(game::GPU_PASS_COUNT, 0);
}

const std::string& PerfProbe::configOf(int index) const {
    // Every second round takes the configs backwards (A B C, C B A, ...): a card that
    // gets slower while it warms up then slows the first and the last config of the list
    // alike, and not always the ones at the end.
    const int count = static_cast<int>(m_spec.configs.size());
    const int round = index / count;
    const int place = index % count;
    return m_spec.configs[static_cast<std::size_t>(round % 2 == 0 ? place : count - 1 - place)];
}

void PerfProbe::applyConfig(const std::string& config, const PerfProbeFrame& frame) const {
    // A line a script can wait for, to take a picture of this config.
    std::fprintf(stdout, "PERF begin config=%s\n", config.c_str());
    std::fflush(stdout);
    for (const std::string& word : split(config, '+')) {
        if (word == "off") {
            frame.grass.enabled = false;
        } else if (word == "on") {
            frame.grass.enabled = true;
        } else if (word == "geom") {
            frame.grass.enabled = true;
            frame.grass.path = game::GrassPath::GeometryShader;
        } else if (word == "inst") {
            frame.grass.enabled = true;
            frame.grass.path = game::GrassPath::Instanced;
        } else if (word == "cull") {
            frame.grass.chunkCulling = true;
        } else if (word == "nocull") {
            frame.grass.chunkCulling = false;
        } else if (word == "lod") {
            frame.grass.distanceLod = true;
        } else if (word == "nolod") {
            frame.grass.distanceLod = false;
        } else if (word.starts_with("density=")) {
            double density = 0.0;
            if (parseNumber(word.substr(std::string("density=").size()), density)) {
                frame.grass.density = static_cast<float>(density);
                frame.grass.replant = true;
            }
        } else if (word == "allcull") {
            frame.frustumCulling = true;
        } else if (word == "noallcull") {
            frame.frustumCulling = false;
        } else if (word == "nowind") {
            frame.grass.windStrength = 0.0F;
        } else if (word == "unlit") {
            frame.lighting.mode = game::LightingMode::Unlit;
        } else if (word == "uvview") {
            frame.viewMode = game::ViewMode::Uvs;
        } else if (word == "land") {
            frame.village.landEnabled = true;
        } else if (word == "noland") {
            frame.village.landEnabled = false;
        } else if (word == "map") {
            frame.minimap.pinned = true;
        } else if (word.starts_with("fov=")) {
            double fov = 0.0;
            if (parseNumber(word.substr(std::string("fov=").size()), fov) && fov > 0.0) {
                frame.camera.fovDegrees = static_cast<float>(fov);
            }
        } else {
            std::fprintf(stdout, "PERF unknown word '%s' in config '%s'\n", word.c_str(),
                         config.c_str());
        }
    }
}

void PerfProbe::printWindow(const std::string& config, int round) const {
    const Window& window = m_window;
    double sum = 0.0;
    for (const double value : window.frameMs) {
        sum += value;
    }
    const auto frames = static_cast<double>(std::max<std::size_t>(window.frameMs.size(), 1));
    std::string line = "PERF config=" + config + " round=" + std::to_string(round) +
                       " frames=" + std::to_string(window.frameMs.size());
    std::array<char, 64> buffer{};
    const auto add = [&line, &buffer](const char* name, double value) {
        std::snprintf(buffer.data(), buffer.size(), " %s=%.3f", name, value);
        line += buffer.data();
    };
    add("frame_ms_mean", sum / frames);
    add("frame_ms_median", median(window.frameMs));
    for (std::size_t pass = 0; pass < game::GPU_PASS_COUNT; ++pass) {
        const std::string name = std::string("gpu_") + PASS_NAMES[pass];
        const int samples = window.gpuSamples[pass];
        add(name.c_str(), samples > 0 ? window.gpuSum[pass] / samples : -1.0);
    }
    const double cpuSamples = std::max(window.cpuSamples, 1);
    add("cpu_render", window.cpuRender / cpuSamples);
    add("cpu_swap", window.cpuSwap / cpuSamples);
    add("draws", window.drawCalls / frames);
    add("triangles", window.triangles / frames);
    add("grass_draws", window.grassDraws / frames);
    std::snprintf(buffer.data(), buffer.size(), " grass_chunks=%.1f/%.1f",
                  window.grassDrawn / frames, window.grassTested / frames);
    line += buffer.data();
    line += window.alwaysPlaying ? " playing=1" : " playing=0";
    std::fprintf(stdout, "%s\n", line.c_str());
    std::fflush(stdout);
}

bool PerfProbe::afterFrame(const PerfProbeFrame& frame) {
    // Measuring needs frames as fast as the card can draw them.
    frame.vsync = false;

    const bool playing = frame.mode == game::GameMode::Playing;
    if (m_spec.pose && playing) {
        // The place is set again in every frame: nothing moves the player away, but
        // a step that starts in the air would otherwise fall.
        frame.player.noclip = true;
        frame.player.position = m_spec.pose->feet;
        frame.camera.yawDegrees = m_spec.pose->yawDegrees;
        frame.camera.pitchDegrees = m_spec.pose->pitchDegrees;
    }

    const int configCount = static_cast<int>(m_spec.configs.size());
    const int windowCount = configCount * m_spec.rounds;
    if (m_index < 0) {
        if (!playing || ++m_framesInRound < FRAMES_BEFORE_FIRST_WINDOW) {
            return false;
        }
        m_index = 0;
        m_secondsInWindow = 0.0;
        applyConfig(m_spec.configs[0], frame);
        return false;
    }

    // This frame belongs to the window m_index: it was drawn with its config.
    const double seconds = frame.time.deltaSeconds();
    m_secondsInWindow += seconds;
    // The picture of the config, once, when its warmup ends in the first round.
    if (!m_spec.shotPath.empty() && m_index < configCount &&
        m_secondsInWindow - seconds <= m_spec.warmupSeconds &&
        m_secondsInWindow > m_spec.warmupSeconds) {
        m_shotDue = configOf(m_index);
    }
    if (m_secondsInWindow > m_spec.warmupSeconds) {
        Window& window = m_window;
        window.alwaysPlaying = window.alwaysPlaying && playing;
        window.frameMs.push_back(seconds * MILLISECONDS_PER_SECOND);
        window.drawCalls += frame.counters.drawCalls;
        window.triangles += frame.counters.triangles;
        const game::GpuPassReading& grass = frame.gpuPasses.reading(game::GpuPass::Grass);
        window.grassDraws += grass.drawCalls;
        window.grassTested += grass.cullTested;
        window.grassDrawn += grass.cullDrawn;
        // The averages of the card and of the CPU are new every half second.
        if (frame.time.fpsRefreshed()) {
            for (std::size_t pass = 0; pass < game::GPU_PASS_COUNT; ++pass) {
                const game::GpuPassReading& reading =
                    frame.gpuPasses.reading(static_cast<game::GpuPass>(pass));
                if (reading.ran && reading.milliseconds.hasValue()) {
                    window.gpuSum[pass] += reading.milliseconds.value();
                    ++window.gpuSamples[pass];
                }
            }
            window.cpuRender += frame.cpuTimes.render.value();
            window.cpuSwap += frame.cpuTimes.swap.value();
            ++window.cpuSamples;
        }
    }
    if (m_secondsInWindow < m_spec.warmupSeconds + m_spec.seconds) {
        return false;
    }

    printWindow(configOf(m_index), m_index / configCount + 1);
    m_window = Window{};
    m_window.gpuSum.assign(game::GPU_PASS_COUNT, 0.0);
    m_window.gpuSamples.assign(game::GPU_PASS_COUNT, 0);
    m_secondsInWindow = 0.0;
    ++m_index;
    if (m_index >= windowCount) {
        std::fprintf(stdout, "PERF done\n");
        std::fflush(stdout);
        return true;
    }
    applyConfig(configOf(m_index), frame);
    return false;
}

void PerfProbe::writeShotIfDue(int width, int height) {
    if (m_shotDue.empty() || width <= 0 || height <= 0) {
        return;
    }
    // The back buffer of the window, as the next swap will show it: rows bottom first,
    // three bytes a pixel, packed.
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height) * 3U);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    const std::string path = m_spec.shotPath + "-" + m_shotDue + ".ppm";
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << width << ' ' << height << "\n255\n";
    const std::size_t row = static_cast<std::size_t>(width) * 3U;
    for (int y = height - 1; y >= 0; --y) {
        file.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * row),
                   static_cast<std::streamsize>(row));
    }
    std::fprintf(stdout, "PERF shot %s\n", path.c_str());
    std::fflush(stdout);
    m_shotDue.clear();
}

} // namespace debug
