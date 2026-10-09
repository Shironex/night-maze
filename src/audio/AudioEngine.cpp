// AudioEngine: opens the sound card, plays short sounds that were loaded at start and
// keeps loops running.
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
// a click. Three are enough for the game: the longest sound that repeats is the bell of
// the gate (2.95 s, at least a second apart), and nothing plays one sound four times
// within its own length.
constexpr std::size_t VOICE_COUNT = 3;

// The format of an engine without a device (AudioOutput::None), where no device says
// what it prefers: the rate most sound cards of today run at, and stereo.
constexpr ma_uint32 SILENT_SAMPLE_RATE = 48000;
constexpr ma_uint32 SILENT_CHANNELS = 2;

// A stop time that never comes: the largest number miniaudio can count frames to.
constexpr ma_uint64 NEVER = ~static_cast<ma_uint64>(0);

// For ma_sound_set_fade_in_pcm_frames: "start the fade at the loudness the sound has
// at this moment".
constexpr float FROM_CURRENT_VOLUME = -1.0F;

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
    // True while the loop of this sound is switched on (AudioEngine::setLoop). A loop
    // runs on the first voice.
    bool loopOn = false;

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

// Reads one sound file and decodes all of it. group is the group its voices are mixed
// in. Returns a null pointer when that fails, with the reason in error.
std::unique_ptr<LoadedSound> loadSound(ma_engine& engine, ma_sound_group& group,
                                       const std::filesystem::path& file, std::string& error) {
    // The file is read by the reader of the project and miniaudio gets the bytes, like
    // the image loader does it for stb: one way of opening files for the whole program,
    // which also works for a path with letters outside of the code page of Windows.
    std::vector<unsigned char> bytes;
    if (!core::readBinaryFile(file, bytes)) {
        error = "the file cannot be opened";
        return nullptr;
    }

    // Decoded straight into the sample format and the sample rate the engine mixes in
    // (32 bit floats, the rate of the device), so the costly conversion is done once.
    // The number of channels is left as the file has it (the 0 below): a mono sound on
    // a device with eight loudspeakers would otherwise be held eight times. The engine
    // spreads the channels of a sound over the ones of the device while it mixes, which
    // is a copy per sample. miniaudio allocates the samples, tells how many frames they
    // are (one frame is one sample per channel) and writes the channels it found into
    // the configuration.
    //
    // What this means for a loop: a device mostly runs at another sample rate than the
    // files have, and the conversion starts from silence, so the first few converted
    // samples are pulled towards 0. In a sound that begins at 0 that changes nothing.
    // In a loop it sits right at the place where the file starts again, and a loop
    // that began at a loud sample would tick there on every round. So the file of
    // a loop has to begin where its wave crosses 0 (tools/make_sounds.py puts the seam
    // there, and tests/AudioEngineTests.cpp measures the result).
    ma_decoder_config decoderConfig =
        ma_decoder_config_init(ma_format_f32, 0, ma_engine_get_sample_rate(&engine));
    ma_uint64 frameCount = 0;
    void* frames = nullptr;
    ma_result result =
        ma_decode_memory(bytes.data(), bytes.size(), &decoderConfig, &frameCount, &frames);
    if (result != MA_SUCCESS) {
        error = std::string("it cannot be decoded (") + ma_result_description(result) + ")";
        return nullptr;
    }
    const ma_uint32 channels = decoderConfig.channels;

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

        // The part of the engine that plays what the reader gives, into the group of
        // the sound. The two flags switch off what a click does not need: a place in
        // a 3D world (every sound is as loud in both ears) and a changeable pitch.
        result = ma_sound_init_from_data_source(
            &engine, &voice.reader, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH,
            &group, &voice.sound);
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
    // The two groups, in the order of SoundGroup. A group is a node of the engine that
    // the voices of its sounds are mixed into, with a volume of its own, before the
    // sum goes on to the device.
    std::array<ma_sound_group, SOUND_GROUP_COUNT> groups{};
    // How many of the groups were initialized: the destructor releases those.
    std::size_t groupsReady = 0;
    // In the order of the files given to load. A null pointer stands for a file that
    // could not be loaded.
    std::vector<std::unique_ptr<LoadedSound>> sounds;
    // The first part of the status line: the device and its format.
    std::string device;
    // True once ma_engine_init has succeeded: the destructor then has to close it.
    bool engineReady = false;
    // True for an engine without a device: only render mixes.
    bool silent = false;

    Backend() = default;
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    // The order matters: the sounds are mixed into the groups and the groups are parts
    // of the engine, so the sounds go first and the engine last.
    ~Backend() {
        sounds.clear();
        for (std::size_t i = 0; i < groupsReady; ++i) {
            ma_sound_group_uninit(&groups.at(i));
        }
        if (engineReady) {
            ma_engine_uninit(&engine);
        }
    }

    // The loaded sound with this number, or a null pointer for a number that was not
    // loaded and for a file that failed.
    LoadedSound* sound(std::size_t index) const {
        return index < sounds.size() ? sounds[index].get() : nullptr;
    }

    // A length in seconds as a number of frames of the engine.
    ma_uint64 frames(float seconds) const {
        return static_cast<ma_uint64>(std::max(seconds, 0.0F) *
                                      static_cast<float>(ma_engine_get_sample_rate(&engine)));
    }
};

AudioEngine::AudioEngine(AudioOutput output) {
    // The engine lives on the heap from the start: miniaudio keeps pointers into it.
    // With the configuration as it comes it opens the default playback device in the
    // format the device prefers and starts it. Without a device the format is named
    // here, and nothing is opened at all.
    auto backend = std::make_unique<Backend>();
    ma_engine_config config = ma_engine_config_init();
    if (output == AudioOutput::None) {
        config.noDevice = MA_TRUE;
        config.sampleRate = SILENT_SAMPLE_RATE;
        config.channels = SILENT_CHANNELS;
        backend->silent = true;
    }
    ma_result result = ma_engine_init(&config, &backend->engine);
    if (result != MA_SUCCESS) {
        // No sound card, no driver, the device is taken: the game runs silent.
        // m_backend stays empty, and every other function asks for that first.
        m_status = std::string("off: ") + ma_result_description(result);
        core::logWarn("Audio: the sound device cannot be opened (" +
                      std::string(ma_result_description(result)) + "): the game is silent");
        return;
    }
    backend->engineReady = true;

    // The two groups. Without them no sound could be loaded, so a failure here is
    // treated like a missing device.
    for (ma_sound_group& group : backend->groups) {
        result = ma_sound_group_init(&backend->engine, 0, nullptr, &group);
        if (result != MA_SUCCESS) {
            m_status = std::string("off: ") + ma_result_description(result);
            core::logWarn("Audio: a sound group cannot be made (" +
                          std::string(ma_result_description(result)) + "): the game is silent");
            return;
        }
        ++backend->groupsReady;
    }

    std::string name = "no device";
    if (!backend->silent) {
        std::array<char, MA_MAX_DEVICE_NAME_LENGTH + 1> deviceName{};
        ma_device_get_name(ma_engine_get_device(&backend->engine), ma_device_type_playback,
                           deviceName.data(), deviceName.size(), nullptr);
        name = deviceName.data();
    }
    backend->device = name + ", " + std::to_string(ma_engine_get_sample_rate(&backend->engine)) +
                      " Hz, " + std::to_string(ma_engine_get_channels(&backend->engine)) +
                      " channels";
    m_status = backend->device + ", no sounds loaded";
    core::logInfo("Audio: opened " + backend->device);
    m_backend = std::move(backend);
}

// Defined here, where Backend is a complete type: the unique_ptr needs its destructor.
AudioEngine::~AudioEngine() = default;

void AudioEngine::load(std::span<const SoundFile> files) {
    if (m_backend == nullptr) {
        return;
    }
    m_backend->sounds.clear();
    std::size_t loadedCount = 0;
    for (const SoundFile& file : files) {
        std::string error;
        std::unique_ptr<LoadedSound> loaded =
            loadSound(m_backend->engine, m_backend->groups.at(static_cast<std::size_t>(file.group)),
                      file.path, error);
        if (loaded == nullptr) {
            core::logWarn("Audio: " + core::pathText(file.path) + ": " + error);
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

void AudioEngine::play(std::size_t index, float volume) {
    LoadedSound* found = m_backend == nullptr ? nullptr : m_backend->sound(index);
    if (found == nullptr) {
        return;
    }
    // The next voice in turn: the one that has been ringing the longest, and nearly
    // always one that has ended. The voice before it is left alone and rings out.
    LoadedSound& loaded = *found;
    ma_sound& sound = loaded.voices.at(loaded.nextVoice).sound;
    loaded.nextVoice = (loaded.nextVoice + 1) % VOICE_COUNT;
    // The volume of this play, written every time: the voice still holds the number
    // of the play before, which may have been a quieter one.
    ma_sound_set_volume(&sound, std::clamp(volume, 0.0F, 1.0F));
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
    // Every voice of every sound, except a voice that runs a loop or has run one
    // (setLoop marks it for good, and miniaudio remembers the mark): loops are ended
    // by setLoop alone. Stopping a voice that is not playing does nothing. Like play,
    // the call only leaves a note for the audio thread. The place a voice has reached
    // does not matter: play rewinds a voice before it starts it.
    for (const std::unique_ptr<LoadedSound>& loaded : m_backend->sounds) {
        if (loaded == nullptr) {
            continue;
        }
        for (Voice& voice : loaded->voices) {
            if (ma_sound_is_looping(&voice.sound) == MA_FALSE) {
                ma_sound_stop(&voice.sound);
            }
        }
    }
}

void AudioEngine::setLoop(std::size_t index, bool on, float fadeSeconds) {
    LoadedSound* loaded = m_backend == nullptr ? nullptr : m_backend->sound(index);
    // The state it has already: nothing to do. This is what lets the owner ask in every
    // frame without starting the fade again every time.
    if (loaded == nullptr || loaded->loopOn == on) {
        return;
    }
    loaded->loopOn = on;
    ma_sound& sound = loaded->voices.front().sound;
    const ma_uint64 fadeFrames = m_backend->frames(fadeSeconds);

    if (!on) {
        // The loudness goes down to 0 over the fade, and at its end the voice stops.
        // The place it has reached stays, so the loop goes on from there next time.
        ma_sound_stop_with_fade_in_pcm_frames(&sound, fadeFrames);
        return;
    }
    // Where the fade in begins. A voice that is still playing is in the middle of
    // a fade out: the fade turns round at the loudness it has reached, without a jump.
    // A voice that has stopped begins at silence. Asked first, because the stop time
    // that is taken away below is what tells the two apart.
    const float from = ma_sound_is_playing(&sound) == MA_TRUE ? FROM_CURRENT_VOLUME : 0.0F;
    // From its end the file goes on at its beginning. The mark stays for good.
    ma_sound_set_looping(&sound, MA_TRUE);
    // The fade out before has left a moment at which the voice stops: it is taken away,
    // or the loop would start and stop again at once.
    ma_sound_set_stop_time_in_pcm_frames(&sound, NEVER);
    ma_sound_set_fade_in_pcm_frames(&sound, from, 1.0F, fadeFrames);
    ma_sound_start(&sound);
}

bool AudioEngine::isLooping(std::size_t index) const {
    const LoadedSound* loaded = m_backend == nullptr ? nullptr : m_backend->sound(index);
    return loaded != nullptr && loaded->loopOn;
}

std::size_t AudioEngine::loopCount() const {
    if (m_backend == nullptr) {
        return 0;
    }
    return static_cast<std::size_t>(
        std::ranges::count_if(m_backend->sounds, [](const std::unique_ptr<LoadedSound>& loaded) {
            return loaded != nullptr && loaded->loopOn;
        }));
}

void AudioEngine::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0F, 1.0F);
    if (m_backend != nullptr) {
        ma_engine_set_volume(&m_backend->engine, m_masterVolume);
    }
}

void AudioEngine::setGroupVolume(SoundGroup group, float volume) {
    const auto index = static_cast<std::size_t>(group);
    m_groupVolumes.at(index) = std::clamp(volume, 0.0F, 1.0F);
    if (m_backend != nullptr) {
        ma_sound_group_set_volume(&m_backend->groups.at(index), m_groupVolumes.at(index));
    }
}

unsigned AudioEngine::sampleRate() const {
    return m_backend == nullptr ? 0 : ma_engine_get_sample_rate(&m_backend->engine);
}

unsigned AudioEngine::channels() const {
    return m_backend == nullptr ? 0 : ma_engine_get_channels(&m_backend->engine);
}

std::vector<float> AudioEngine::render(std::size_t frameCount) {
    // With a device the audio thread is the one that asks the engine for sound: a second
    // reader here would take frames away from it.
    if (m_backend == nullptr || !m_backend->silent) {
        return {};
    }
    // The engine mixes 32 bit floats: one per channel and frame.
    std::vector<float> samples(frameCount * channels());
    ma_engine_read_pcm_frames(&m_backend->engine, samples.data(), frameCount, nullptr);
    return samples;
}

} // namespace audio
