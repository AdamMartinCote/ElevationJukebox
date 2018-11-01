#ifndef MP3_LIST_SONGLIST_HPP
#define MP3_LIST_SONGLIST_HPP

#include <experimental/filesystem>
#include <list>
#include <mutex>

namespace elevation {

/**
 * @brief Thread-safe song list class.
 */
class SongList {
public:
    explicit SongList(std::size_t maxSongs);
    SongList(const SongList&) = delete;
    SongList(SongList&&) = delete;
    virtual ~SongList();

    SongList& operator=(const SongList&) = delete;
    SongList& operator=(SongList&&) = delete;

    void push(const std::experimental::filesystem::path& songPath);
    void remove(const std::experimental::filesystem::path& songPath);
    void swap(std::size_t songAPosition, std::size_t songBPosition);
    std::list<std::experimental::filesystem::path> getSongList() const { ///< @note This method is not thread-safe.
        return m_songs;
    }

protected:
    std::list<std::experimental::filesystem::path> m_songs;
    std::size_t m_maxSongs;
    std::mutex m_songListMutex;
};

} // namespace elevation

#endif // !MP3_LIST_SONGLIST_HPP
