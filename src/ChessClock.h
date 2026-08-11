#ifndef CHESS_CLOCK_H
#define CHESS_CLOCK_H

#include <chrono>

class ChessClock
{
public:
    ChessClock();

    void configure(
        int initialSeconds,
        int incrementSeconds);

    void start(
        bool whiteStarts = true);

    void stop();

    void reset();

    void update();

    void onMove();

    bool isRunning() const;

    bool isEnabled() const;

    bool whiteFlagged() const;

    bool blackFlagged() const;

    int getWhiteSeconds() const;

    int getBlackSeconds() const;

    int getIncrementSeconds() const;

    bool isWhiteActive() const;

    void syncFromServer(
        int whiteTimeMs,
        int blackTimeMs,
        bool whiteToMove,
        bool shouldRun);

    void onOnlineMove();

private:
    using Clock =
        std::chrono::steady_clock;

    long long whiteMilliseconds = 0;
    long long blackMilliseconds = 0;

    long long initialMilliseconds = 0;
    long long incrementMilliseconds = 0;

    bool running = false;
    bool enabled = false;
    bool whiteActive = true;

    Clock::time_point lastUpdate;

    void subtractElapsedTime();
};

#endif