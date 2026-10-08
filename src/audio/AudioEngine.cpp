// AudioEngine: opens the sound card and plays short sounds that were loaded at start.
// See docs/modules/audio/README.md
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

// How many copies of one sound can ring at the same moment. A sound that is played
// again while it still rings is not cut off: the next voice starts and the one before
// rings out. Cutting a ringing sound makes the loudspeaker jump, which is heard as
// a click. Three are enough for the game: its longest sounds last under two seconds,
// and nothing plays the same one four times within that.
constexpr std::size_t VOICE_COUNT = 3;

// One voice: something that can play a sound once at a time. The reader knows where in
// the samples this voice is, the sound is the part of the engine that mixes what the
// reader gives it.
struct Voice {
    ma_audio_buffer_ref reader{};
    ma_sound sound{};
    // True once both were initialized: the destructor of LoadedSound releases them.
    bool ready = false;
};

// One loaded sound: its decoded samples, once, and the voices that play them. Every
// voice has a reader of its own, because the place a sound has reached is kept in the
// reader (the field cursor of ma_audio_buffer_ref): two voices on one reader would
// share one place and could not play apart from each other. A reader does not copy the
// samples, it only points at them.
//
// miniaudio keeps pointers to the readers and the sounds while the engine runs, so an
// object of this struct must never move in memory: each one lives behind
// a std::unique_ptr.
struct LoadedSound {
    std::vector<float> samples;
    std::array<Voice, VOICE_COUNT> voices;
    // The voice the next play takes: they are used in turn, so it is always the one
    // that was started the longest time ago.
    std::size_t nextVoice = 0;

    LoadedSound() = default;
    LoadedSound(const LoadedSound&) = delete;
    LoadedSound& operator=(const LoadedSound&) = delete;

    // The sound of a voice before its reader, because the sound reads from it. The
    // samples go last, by themselves (members are destroyed after this body has run).
    ~LoadedSound() {
        for (Voice& voice : voices) {
            if (voice.ready) {
                ma_sound_uninit(&voice.sound);
                ma_audio_buffer_ref_uninit(&voice.reader);
            }
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
    // The samples are copied into a vector of ours, which releases them by itself, and
    // the block from the decoder is given back right here. One frame holds one float
    // per channel.
    const auto* decoded = static_cast<const float*>(frames);
    loaded->samples.assign(decoded, decoded + frameCount * channels);
    ma_free(frames, nullptr);

    for (Voice& voice : loaded->voices) {
        // The reader of this voice: a place in the samples, starting at their beginning.
        result = ma_audio_buffer_ref_init(ma_format_f32, channels, loaded->samples.data(),
                                          frameCount, &voice.reader);
        if (result != MA_SUCCESS) {
            error = std::string("no reader for it (") + ma_result_description(result) + ")";
            return nullptr;
        }
        // This version of miniaudio leaves the sample rate of a reader at 0 (its source
        // says so), and the engine asks the reader for it: it is written here by hand.
        voice.reader.sampleRate = ma_engine_get_sample_rate(&engine);

        // The part of the engine that plays what the reader gives. The two flags switch
        // off what a click does not need: a place in a 3D world (every sound is as loud
        // in both ears) and a changeable pitch.
        result = ma_sound_init_from_data_source(
            &engine, &voice.reader, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH,
            nullptr, &voice.sound);
        if (result != MA_SUCCESS) {
            ma_audio_buffer_ref_uninit(&voice.reader);
            error =
                std::string("the engine does not take it (") + ma_result_description(result) + ")";
            return nullptr;
        }
        voice.ready = true;
    }
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
    // The next voice in turn: the one that has been ringing the longest, and nearly
    // always one that has ended. The voice before it is left alone and rings out.
    LoadedSound& loaded = *m_backend->sounds[index];
    ma_sound& sound = loaded.voices.at(loaded.nextVoice).sound;
    loaded.nextVoice = (loaded.nextVoice + 1) % VOICE_COUNT;
    // Back to the first frame, then start. For a voice that has ended the seek rewinds
    // it and the start plays it. For one that is still playing the start does nothing
    // and the seek makes it begin again. Both calls only leave a note for the audio
    // thread, which acts on it when it mixes its next piece.
    // A known limit: that second case cuts a ringing voice and can be heard as a small
    // click. It takes VOICE_COUNT + 1 plays of one sound within the length of that
    // sound, which the game does not do. If it ever does, raise VOICE_COUNT.
    ma_sound_seek_to_pcm_frame(&sound, 0);
    ma_sound_start(&sound);
}

void AudioEngine::stopAll() {
    if (m_backend == nullptr) {
        return;
    }
    // Every voice of every sound. Stopping a voice that is not playing does nothing.
    // Like play, the call only leaves a note for the audio thread. The place a voice
    // has reached does not matter: play rewinds a voice before it starts it.
    for (const std::unique_ptr<LoadedSound>& loaded : m_backend->sounds) {
        if (loaded == nullptr) {
            continue;
        }
        for (Voice& voice : loaded->voices) {
            ma_sound_stop(&voice.sound);
        }
    }
}

void AudioEngine::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0F, 1.0F);
    if (m_backend != nullptr) {
        ma_engine_set_volume(&m_backend->engine, m_masterVolume);
    }
}

} // namespace audio
