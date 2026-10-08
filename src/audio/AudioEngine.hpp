// AudioEngine: opens the sound card, plays short sounds that were loaded at start and
// keeps loops running.
#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace audio {

/// The two groups a sound can belong to. Each has a volume of its own under the master
/// volume, so the player can turn the wind down without losing the sounds that tell
/// something, or the other way round.
enum class SoundGroup {
    Effects = 0, ///< everything that happens: clicks, pickups, steps, warnings
    Ambient,     ///< the air of a place: wind
};

/// How many groups there are: the number of entries of SoundGroup.
constexpr std::size_t SOUND_GROUP_COUNT = 2;

/// One sound to load: its file and the group it is mixed in.
struct SoundFile {
    std::filesystem::path path;
    SoundGroup group = SoundGroup::Effects;
};

/// Where the mixed sound goes.
enum class AudioOutput {
    /// To the default sound device of the computer: the game.
    Device,
    /// Nowhere by itself: no device is opened, and the engine mixes only when render is
    /// called. For the tests, which run on machines without a sound card and must not
    /// be heard on the others.
    None,
};

/// Plays sounds through the default sound device of the computer: short ones from
/// start to end (play), and loops that are switched on and off with a fade (setLoop).
///
/// It knows nothing about the game. Whoever owns it hands it a list of sound files once
/// (load) and from then on names a sound by its place in that list. What a sound means,
/// and when it is played, is decided elsewhere (game/SoundCues.hpp).
///
/// The work is done by the miniaudio library. This header does not include it: all of
/// its objects are in the struct Backend, which only AudioEngine.cpp knows. The code
/// that uses this class needs neither the header of miniaudio nor its include path.
///
/// How loud a sound is heard is the product of four numbers, each from 0 to 1: the
/// samples of its file, the volume of the play (play), the volume of its group
/// (setGroupVolume) and the master volume (setMasterVolume). miniaudio multiplies them
/// while it mixes: the groups are two nodes every sound runs through on its way to the
/// device. This class does not mix anything itself.
///
/// Sound is something the game can do without. When the computer has no sound device,
/// or the device cannot be opened, the constructor writes one line into the log and
/// every later call does nothing. The same goes for a single file that is missing or
/// cannot be decoded: one line, and playing that sound does nothing. No function of
/// this class throws for such a reason.
///
/// Threads: miniaudio starts a thread of its own when the device is opened (on macOS
/// the system calls into miniaudio on a thread of the system). A few hundred times per
/// second that thread asks the engine for the next few milliseconds of sound, and the
/// engine mixes the sounds that are playing into them. None of OUR code runs there: no
/// callback of this class is registered with miniaudio. play, setLoop, stopAll and the
/// volumes only set numbers that the audio thread reads on its next turn (miniaudio
/// guards them itself), so this class needs no mutex, and nothing the game does (a slow
/// frame, loading a maze) can make the sound stutter. A fade is such a number too: it
/// is counted by the audio thread, sample by sample. All functions of this class are
/// called from one thread, the thread of the game.
class AudioEngine {
public:
    /// Opens the default sound device and starts it. It does not throw when that fails:
    /// ask isAvailable() and status(). With AudioOutput::None no device is opened and
    /// nothing is ever heard: the mixed sound is fetched with render.
    explicit AudioEngine(AudioOutput output = AudioOutput::Device);

    /// Stops the sounds, then the device.
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// Reads the sound files and decodes each of them completely into memory: they are
    /// short, and a decoded sound starts without any work at the moment it is played.
    /// The sound of files[i] is number i for play and setLoop. A file that cannot be
    /// loaded keeps its number, so the numbers of the others do not move. Calling it
    /// again replaces the list, and every loop is off afterwards. Does nothing without
    /// a device.
    void load(std::span<const SoundFile> files);

    /// Plays sound number index from its beginning, once. volume is how loud this one
    /// play is, from 0 to 1 (1 is the file as it is, and values outside are brought
    /// into the range): a step far away is the same file as a step near by, played
    /// quieter. A sound that is still ringing is not cut off: every sound has a few
    /// voices that are used in turn, so the new copy plays over the one before, which
    /// rings out at the volume it was started with (cutting a ringing sound would be
    /// heard as a click). Only when one sound is played more often within its own
    /// length than it has voices does the oldest copy start again. Different sounds
    /// always play over each other. A sound that was started plays to its end, unless
    /// stopAll ends it. Does nothing without a device, for a number that was not loaded
    /// and for a sound whose file failed to load.
    void play(std::size_t index, float volume = 1.0F);

    /// Switches the loop of sound number index on or off. A loop plays its file again
    /// and again without a gap, so the file has to end where it begins.
    ///
    /// On: the sound fades in over fadeSeconds and then goes round until it is switched
    /// off. Off: it fades out over fadeSeconds and stops. Switched on again it goes on
    /// from the place it stopped at, and a fade that is turned round half way starts
    /// from the loudness it has reached. Nothing ever starts or ends at once, so there
    /// is no click (unless fadeSeconds is 0).
    ///
    /// Asking for the state the loop already has does nothing, so the owner can simply
    /// say in every frame what it wants. A sound should be used as a loop or with
    /// play, not both. Does nothing without a device and for a sound that is not
    /// loaded.
    void setLoop(std::size_t index, bool on, float fadeSeconds);

    /// True while the loop of sound number index is switched on (also while it still
    /// fades in). False from the moment it is switched off, although it is heard
    /// a moment longer while it fades out.
    bool isLooping(std::size_t index) const;

    /// How many loops are switched on, for the debug window.
    std::size_t loopCount() const;

    /// Stops every sound that was started with play, at once. A sound that is stopped
    /// in the middle is cut without a fade, so this is for the few places where silence
    /// matters more than a clean end: the long wind of the intro must not go on over
    /// the main menu. Each sound can be played again afterwards, from its beginning.
    ///
    /// The rule for loops: stopAll never touches one. A loop that is on stays on, and
    /// one that is fading out finishes its fade. Only setLoop ends a loop, so whoever
    /// decides about the loops (the rule game::mazeWindPlays, asked in every frame) is
    /// the one place that can leave one running or end it.
    /// Does nothing without a device.
    void stopAll();

    /// The loudness of everything together: 0 is silent, 1 is the files as they are.
    /// Values outside of that range are brought into it.
    void setMasterVolume(float volume);
    float masterVolume() const { return m_masterVolume; }

    /// The loudness of one group under the master volume, with the same range. The
    /// number is kept also without a device, so the debug window can show it.
    void setGroupVolume(SoundGroup group, float volume);
    float groupVolume(SoundGroup group) const {
        return m_groupVolumes.at(static_cast<std::size_t>(group));
    }

    /// True when sounds can be played: the sound device was opened, or the engine was
    /// made without one on purpose (AudioOutput::None).
    bool isAvailable() const { return m_backend != nullptr; }

    /// One line for the debug window: the name of the device, its format and how many
    /// sounds are loaded, or the reason why there is no sound.
    const std::string& status() const { return m_status; }

    /// How many frames the engine mixes per second (one frame is one sample per
    /// channel), and how many channels a frame has. Both 0 without a device.
    unsigned sampleRate() const;
    unsigned channels() const;

    /// Mixes the next frameCount frames and returns their samples, channels() numbers
    /// per frame. Only for an engine made with AudioOutput::None, where nothing else
    /// asks for sound: time passes for such an engine only in this call, so a fade of
    /// one second is over after sampleRate() frames. With a device the audio thread
    /// does the mixing, and this returns an empty list.
    std::vector<float> render(std::size_t frameCount);

private:
    // The objects of miniaudio (the engine, the groups and the loaded sounds), defined
    // in AudioEngine.cpp. A null pointer when there is no device.
    struct Backend;
    std::unique_ptr<Backend> m_backend;

    std::string m_status;
    float m_masterVolume = 1.0F;
    // In the order of SoundGroup.
    std::array<float, SOUND_GROUP_COUNT> m_groupVolumes = {1.0F, 1.0F};
};

} // namespace audio
