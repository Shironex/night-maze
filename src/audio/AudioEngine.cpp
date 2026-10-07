// AudioEngine: opens the sound card and plays short sounds that were loaded at start.
#include "audio/AudioEngine.hpp"

#include "core/Files.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <vector>

namespace audio {

namespace {

// One loaded sound: the decoded samples (the buffer) and the voice that plays them (the
// sound). miniaudio keeps pointers to both while the engine runs, so an object of this
// struct must never move in memory: each one lives behind a std::unique_ptr.
struct LoadedSound {
    ma_audio_buffer buffer{};
    ma_sound sound{};
    // Which of the two were initialized, so the destructor releases exactly those.
    bool bufferReady = false;
    bool soundReady = false;

    LoadedSound() = default;
    LoadedSound(const LoadedSound&) = delete;
    LoadedSound& operator=(const LoadedSound&) = delete;

    // The voice first, because it reads from the buffer.
    ~LoadedSound() {
        if (soundReady) {
            ma_sound_uninit(&sound);
        }
        if (bufferReady) {
            ma_audio_buffer_uninit(&buffer);
        }
    }
};

// Reads one sound file and decodes all of it. Returns a null pointer when that fails,
// with the reason in error.
std::unique_ptr<LoadedSound> loadSound(ma_engine& engine, const std::filesystem::path& file,
                                       std::string& error) {
    // The file is read by the reader of the project and miniaudio gets the bytes, like
    // the image loader does it for stb: one way of opening files for the whole program,
    // which also works for a path with letters outside of the code page of Windows.
    std::vector<unsigned char> bytes;
    if (!core::readBinaryFile(file, bytes)) {
        error = "the file cannot be opened";
        return nullptr;
    }

    // Decoded straight into the format the engine mixes in (32 bit floats, its number
    // of channels and its sample rate), so nothing has to be converted while a sound
    // plays. miniaudio allocates the samples and tells how many frames they are (one
    // frame is one sample per channel).
    const ma_uint32 channels = ma_engine_get_channels(&engine);
    ma_decoder_config decoderConfig =
        ma_decoder_config_init(ma_format_f32, channels, ma_engine_get_sample_rate(&engine));
    ma_uint64 frameCount = 0;
    void* frames = nullptr;
    ma_result result =
        ma_decode_memory(bytes.data(), bytes.size(), &decoderConfig, &frameCount, &frames);
    if (result != MA_SUCCESS) {
        error = std::string("it cannot be decoded (") + ma_result_description(result) + ")";
        return nullptr;
    }

    auto loaded = std::make_unique<LoadedSound>();
    // init_copy: the buffer gets a copy of the samples that it owns and releases
    // itself, so the block from the decoder can be given back right here.
    ma_audio_buffer_config bufferConfig =
        ma_audio_buffer_config_init(ma_format_f32, channels, frameCount, frames, nullptr);
    bufferConfig.sampleRate = ma_engine_get_sample_rate(&engine);
    result = ma_audio_buffer_init_copy(&bufferConfig, &loaded->buffer);
    ma_free(frames, nullptr);
    if (result != MA_SUCCESS) {
        error = std::string("no memory for its samples (") + ma_result_description(result) + ")";
        return nullptr;
    }
    loaded->bufferReady = true;

    // The voice. The two flags switch off what a click does not need: a place in a 3D
    // world (every sound is as loud in both ears) and a changeable pitch.
    result = ma_sound_init_from_data_source(
        &engine, &loaded->buffer, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, nullptr,
        &loaded->sound);
    if (result != MA_SUCCESS) {
        error = std::string("the engine does not take it (") + ma_result_description(result) + ")";
        return nullptr;
    }
    loaded->soundReady = true;
    return loaded;
}

} // namespace

struct AudioEngine::Backend {
    ma_engine engine{};
    // In the order of the files given to load. A null pointer stands for a file that
    // could not be loaded.
    std::vector<std::unique_ptr<LoadedSound>> sounds;
    // The first part of the status line: the device and its format.
    std::string device;
    // True once ma_engine_init has succeeded: the destructor then has to close it.
    bool engineReady = false;

    Backend() = default;
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    // The order matters: the sounds are parts of the engine, so they go first.
    ~Backend() {
        sounds.clear();
        if (engineReady) {
            ma_engine_uninit(&engine);
        }
    }
};

AudioEngine::AudioEngine() {
    // The engine lives on the heap from the start: miniaudio keeps pointers into it.
    // Without a configuration it opens the default playback device in the format the
    // device prefers and starts it.
    auto backend = std::make_unique<Backend>();
    const ma_result result = ma_engine_init(nullptr, &backend->engine);
    if (result != MA_SUCCESS) {
        // No sound card, no driver, the device is taken: the game runs silent.
        // m_backend stays empty, and every other function asks for that first.
        m_status = std::string("off: ") + ma_result_description(result);
        core::logWarn("Audio: the sound device cannot be opened (" +
                      std::string(ma_result_description(result)) + "): the game is silent");
        return;
    }
    backend->engineReady = true;

    std::array<char, MA_MAX_DEVICE_NAME_LENGTH + 1> name{};
    ma_device_get_name(ma_engine_get_device(&backend->engine), ma_device_type_playback, name.data(),
                       name.size(), nullptr);
    backend->device = std::string(name.data()) + ", " +
                      std::to_string(ma_engine_get_sample_rate(&backend->engine)) + " Hz, " +
                      std::to_string(ma_engine_get_channels(&backend->engine)) + " channels";
    m_status = backend->device + ", no sounds loaded";
    core::logInfo("Audio: opened " + backend->device);
    m_backend = std::move(backend);
}

// Defined here, where Backend is a complete type: the unique_ptr needs its destructor.
AudioEngine::~AudioEngine() = default;

void AudioEngine::load(std::span<const std::filesystem::path> files) {
    if (m_backend == nullptr) {
        return;
    }
    m_backend->sounds.clear();
    std::size_t loadedCount = 0;
    for (const std::filesystem::path& file : files) {
        std::string error;
        std::unique_ptr<LoadedSound> loaded = loadSound(m_backend->engine, file, error);
        if (loaded == nullptr) {
            core::logWarn("Audio: " + core::pathText(file) + ": " + error);
        } else {
            ++loadedCount;
        }
        // Also the null pointer goes into the list: the numbers of the sounds are the
        // numbers of the files.
        m_backend->sounds.push_back(std::move(loaded));
    }
    const std::string counts =
        std::to_string(loadedCount) + " of " + std::to_string(files.size()) + " sounds loaded";
    m_status = m_backend->device + ", " + counts;
    core::logInfo("Audio: " + counts);
}

void AudioEngine::play(std::size_t index) {
    if (m_backend == nullptr || index >= m_backend->sounds.size() ||
        m_backend->sounds[index] == nullptr) {
        return;
    }
    ma_sound& sound = m_backend->sounds[index]->sound;
    // Back to the first frame, then start. For a sound that is playing the start does
    // nothing and the seek makes it begin again. For one that has ended the seek
    // rewinds it and the start plays it. Both calls only leave a note for the audio
    // thread, which acts on it when it mixes its next piece.
    // A known limit: one voice per sound. Cutting a sound that is still ringing can be heard
    // as a small pop (two crystals within half a second). When that matters, give the
    // long sounds two or three voices and take them in turn.
    ma_sound_seek_to_pcm_frame(&sound, 0);
    ma_sound_start(&sound);
}

void AudioEngine::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0F, 1.0F);
    if (m_backend != nullptr) {
        ma_engine_set_volume(&m_backend->engine, m_masterVolume);
    }
}

} // namespace audio
