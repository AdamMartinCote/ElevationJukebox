#ifndef MP3_PLAYER_PULSE_PULSEVOLUME_HPP
#define MP3_PLAYER_PULSE_PULSEVOLUME_HPP

#include <memory>
#include <limits>

#include <pulse/pulseaudio.h>

#include <common/mp3/definitions/Volume.hpp>

namespace elevation {

class PulseDevice; // Forward-declaration to brea circular dependencies

/**
 * @brief Interacts with the PulseAudio volume.
 *
 * @note The volume that is returned is the volume of
 * the average channel of the default output sink.
 */
class PulseVolume {
protected:
    static constexpr double K = 0.5; ///< Linear <-> LogScale parameter that can be tweaked. Must be strictly positive.
    static constexpr double F0 = 0.00001; ///< Linear <-> LogScale parameter that can be tweaked. Must be beween in ]0, 1], but should probably be close to 0.

protected:
    static double toLogScale_(double linearFactor);
    static double fromLogScale_(double logFactor);

protected:
    class SinkState {
    public:
        explicit SinkState(bool isMuted = true, volumePercent_t volume = std::numeric_limits<volumePercent_t>::max()) : m_isMuted(isMuted), m_volume(volume) { }
        bool isMuted() const { return m_isMuted; }
        volumePercent_t getVolume() const { return m_volume; }

    private:
        bool m_isMuted;
        volumePercent_t m_volume;
    };

public:
    explicit PulseVolume();
    PulseVolume(const PulseVolume&) = delete;
    PulseVolume(PulseVolume&&) = delete;
    virtual ~PulseVolume();

    PulseVolume& operator=(const PulseVolume&) = delete;
    PulseVolume& operator=(PulseVolume&&) = delete;

    volumePercent_t getVolume() const;
    bool isMuted() const;
    void setVolume(volumePercent_t newVolume);
    void mute();
    void unmute();

protected:
    void initializeContext_();
    void initializeSinkData_();

    ::pa_cvolume makePulseVolumeStructure_(volumePercent_t volume) const;

    SinkState getSinkState_() const;
    void muteOrUnmute_(bool mute);

protected:
    ::pa_context* m_context;
    ::pa_mainloop* m_mainloop;
    uint32_t m_sinkIndex;
    uint8_t m_numSinkChannels;
};

} // namespace elevation

#endif // !MP3_PLAYER_PULSE_PULSEVOLUME_HPP
