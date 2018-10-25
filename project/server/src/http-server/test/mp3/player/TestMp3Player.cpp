#define BOOST_TEST_MODULE Mp3Player
#include <boost/test/unit_test.hpp>

#include <chrono>

#include <mp3/player/Mp3Player.hpp>

#include <TestConfiguration.hpp>

namespace elevation {

class TestFixture {
public:
    Mp3Player m_player;
};

BOOST_FIXTURE_TEST_CASE(constructionDestruction, TestFixture) {
}

BOOST_FIXTURE_TEST_CASE(moveConstructor, TestFixture) {
    Mp3Player moved(std::move(m_player));
}

unsigned int TIPPERARY_SHORT_TIMEOUT_SECONDS = 10;
BOOST_AUTO_TEST_CASE(startPlaying_and_waitUntilSongFinished, *boost::unit_test::timeout(TIPPERARY_SHORT_TIMEOUT_SECONDS)) {
    using namespace std::chrono_literals;
    const auto MINIMUM_DURATION = TIPPERARY_DURATION_SECONDS - 1.0s;
    const auto MAXIMUM_DURATION = TIPPERARY_DURATION_SECONDS + 1.0s;

    Mp3Player player;
    auto startTime = std::chrono::system_clock::now();
    player.startPlaying(TIPPERARY_SHORT_PATH);
    player.waitUntilSongFinished();
    auto endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> duration(std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime).count());
    BOOST_CHECK(duration > MINIMUM_DURATION); // Using BOOST_CHECK and not BOOST_TEST because the operands are not printable using operator<<.
    BOOST_CHECK(duration < MAXIMUM_DURATION);
}

} // namespace elevation
