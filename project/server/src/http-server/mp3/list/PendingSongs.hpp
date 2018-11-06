#ifndef MP3_PLAYER_PENDINGSONGS_HPP
#define MP3_PLAYER_PENDINGSONGS_HPP

#include <experimental/filesystem>
#include <list>
#include <mutex>
#include <thread>
#include <memory>
#include <future>
#include <chrono>

#include "mp3/player/Mp3Player.hpp"

namespace elevation {

/**
 * @brief A class to which we simply add or remove songs.
 * It automagically starts playing a song when we add one, and plays the next
 * one when the first one is finished.
 *
 * @note Unlike #Mp3Player, destruction does (should) not wait until
 * the song is finished playing ; it should just stop the current song.
 */
class PendingSongs {
protected:
    typedef ::std::experimental::filesystem::path path;

protected:
    static constexpr std::chrono::seconds NEXT_SONG_POLLING_DELAY{1};

public:
    /**
     * @brief Constructor.
     * @param[in] nextSongGetter Callback which will be called when a new song should be started.
     * Will be called periodically until the return value is not an empty path.
     * @param[in] songRemover Callback which will be called when a song finishes and needs to be removed
     * from the song list.
     */
    explicit PendingSongs(
        std::function<path()> nextSongGetter,
        std::function<void(path)> songRemover
    );
    PendingSongs(const PendingSongs&) = delete;
    PendingSongs(PendingSongs&&) = delete; ///< Cannot move() because we own a thread, whose 'this' pointer isn't updated when we move.
    virtual ~PendingSongs();

    PendingSongs& operator=(const PendingSongs&) = delete;
    PendingSongs& operator=(PendingSongs&&) = delete; ///< Cannot move() because we own a thread, whose 'this' pointer isn't updated when we move.

    /**
     * @brief Stops the song that is currently playing. If there is none,
     * this method does nothing.
     */
    void stopSong();

protected:
    /**
     * @brief Thread method which starts the songs on the #Mp3Player.
     * Asynchronously waits until a song is added to start playing,
     * and waits until the song finishes.
     */
    void songStarter_();
    void sendTerminate_();
    void sendStartSignal();

protected:
    Mp3Player m_player;
    std::thread m_playerThread;

    std::function<std::experimental::filesystem::path()> m_nextSongGetter;
    std::function<void(std::experimental::filesystem::path)> m_songRemover;

    std::atomic<bool> m_terminateRequested;
};

} // namespace elevation

#endif // !MP3_PLAYER_PENDINGSONGS_HPP
