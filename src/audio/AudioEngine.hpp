// AudioEngine: opens the sound card and plays short sounds that were loaded at start.
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

namespace audio {

/// Plays short sounds through the default sound device of the computer.
///
/// It knows nothing about the game. Whoever owns it hands it a list of sound files once
/// (load) and from then on names a sound by its place in that list (play). What a sound
/// means, and when it is played, is decided elsewhere (game/SoundCues.hpp).
///
/// The work is done by the miniaudio library. This header does not include it: all of
/// its objects are in the struct Backend, which only AudioEngine.cpp knows. The code
/// that uses this class needs neither the header of miniaudio nor its include path.
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
/// callback of this class is registered with miniaudio. play and setMasterVolume only
/// set numbers that the audio thread reads on its next turn (miniaudio guards them
/// itself), so this class needs no mutex, and nothing the game does (a slow frame,
/// loading a maze) can make the sound stutter. All functions of this class are called
/// from one thread, the thread of the game.
class AudioEngine {
public:
    /// Opens the default sound device and starts it. It does not throw when that fails:
    /// ask isAvailable() and status().
    AudioEngine();

    /// Stops the sounds, then the device.
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// Reads the sound files and decodes each of them completely into memory: they are
    /// short, and a decoded sound starts without any work at the moment it is played.
    /// The sound of files[i] is number i for play. A file that cannot be loaded keeps
    /// its number, so the numbers of the others do not move. Calling it again replaces
    /// the list. Does nothing without a device.
    void load(std::span<const std::filesystem::path> files);

    /// Plays sound number index from its beginning. A sound that is still playing
    /// starts again: there is one voice per sound, which is right for clicks and
    /// keeps the same sound from piling up on itself. Different sounds play over each
    /// other. Does nothing without a device, for a number that was not loaded and for
    /// a sound whose file failed to load.
    void play(std::size_t index);

    /// The loudness of everything together: 0 is silent, 1 is the files as they are.
    /// Values outside of that range are brought into it.
    void setMasterVolume(float volume);
    float masterVolume() const { return m_masterVolume; }

    /// True when the sound device was opened: sounds can be played.
    bool isAvailable() const { return m_backend != nullptr; }

    /// One line for the debug window: the name of the device, its format and how many
    /// sounds are loaded, or the reason why there is no sound.
    const std::string& status() const { return m_status; }

private:
    // The objects of miniaudio (the engine and the loaded sounds), defined in
    // AudioEngine.cpp. A null pointer when there is no device.
    struct Backend;
    std::unique_ptr<Backend> m_backend;

    std::string m_status;
    float m_masterVolume = 1.0F;
};

} // namespace audio
