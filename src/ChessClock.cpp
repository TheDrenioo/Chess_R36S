#include "ChessClock.h"

#include <algorithm>

ChessClock::ChessClock()
{
}

void ChessClock::configure(
    int initialSeconds,
    int incrementSeconds)
{
    initialMilliseconds =
        static_cast<long long>(
            initialSeconds) *
        1000;

    incrementMilliseconds =
        static_cast<long long>(
            incrementSeconds) *
        1000;

    enabled =
        initialSeconds > 0;

    reset();
}

void ChessClock::reset()
{
    whiteMilliseconds =
        initialMilliseconds;

    blackMilliseconds =
        initialMilliseconds;

    whiteActive = true;

    running = false;
}

void ChessClock::start(
    bool whiteStarts)
{
    if (!enabled)
    {
        return;
    }

    whiteActive =
        whiteStarts;

    running = true;

    lastUpdate =
        Clock::now();
}

void ChessClock::stop()
{
    if (running)
    {
        update();
    }

    running = false;
}

void ChessClock::subtractElapsedTime()
{
    if (
        !running ||
        !enabled)
    {
        return;
    }

    auto now =
        Clock::now();

    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                now -
                lastUpdate)
            .count();

    lastUpdate =
        now;

    if (whiteActive)
    {
        whiteMilliseconds -=
            elapsed;

        whiteMilliseconds =
            std::max(
                0LL,
                whiteMilliseconds);
    }
    else
    {
        blackMilliseconds -=
            elapsed;

        blackMilliseconds =
            std::max(
                0LL,
                blackMilliseconds);
    }
}

void ChessClock::update()
{
    subtractElapsedTime();

    if (
        whiteMilliseconds <= 0 ||
        blackMilliseconds <= 0)
    {
        running = false;
    }
}

void ChessClock::onMove()
{
    if (
        !running ||
        !enabled)
    {
        return;
    }

    // Count all time used before the move.
    subtractElapsedTime();

    if (whiteActive)
    {
        whiteMilliseconds +=
            incrementMilliseconds;
    }
    else
    {
        blackMilliseconds +=
            incrementMilliseconds;
    }

    whiteActive =
        !whiteActive;

    lastUpdate =
        Clock::now();
}

bool ChessClock::isRunning() const
{
    return running;
}

bool ChessClock::isEnabled() const
{
    return enabled;
}

bool ChessClock::whiteFlagged() const
{
    return
        enabled &&
        whiteMilliseconds <= 0;
}

bool ChessClock::blackFlagged() const
{
    return
        enabled &&
        blackMilliseconds <= 0;
}

int ChessClock::getWhiteSeconds() const
{
    return static_cast<int>(
        (whiteMilliseconds + 999) /
        1000);
}

int ChessClock::getBlackSeconds() const
{
    return static_cast<int>(
        (blackMilliseconds + 999) /
        1000);
}

int ChessClock::getIncrementSeconds() const
{
    return static_cast<int>(
        incrementMilliseconds /
        1000);
}

bool ChessClock::isWhiteActive() const
{
    return whiteActive;
}

void ChessClock::syncFromServer(
    int whiteTimeMs,
    int blackTimeMs,
    bool whiteToMove,
    bool shouldRun)
{
    whiteMilliseconds =
        std::max(
            0LL,
            static_cast<long long>(
                whiteTimeMs));

    blackMilliseconds =
        std::max(
            0LL,
            static_cast<long long>(
                blackTimeMs));

    enabled =
        whiteTimeMs > 0 ||
        blackTimeMs > 0;

    whiteActive =
        whiteToMove;

    running =
        enabled &&
        shouldRun &&
        whiteMilliseconds > 0 &&
        blackMilliseconds > 0;

    lastUpdate =
        Clock::now();
}

void ChessClock::onOnlineMove()
{
    if (
        !enabled ||
        !running)
    {
        return;
    }

    subtractElapsedTime();

    whiteActive =
        !whiteActive;

    lastUpdate =
        Clock::now();
}