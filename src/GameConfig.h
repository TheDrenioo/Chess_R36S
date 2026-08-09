#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <string>

enum class GameMode
{
    PlayerVsComputer,
    PlayerVsPlayer
};

enum class Difficulty
{
    Easy,
    Medium,
    Hard,
    Expert,
    Master
};

enum class TimeControl
{
    NoClock,
    Bullet1,
    Blitz3,
    Blitz3Plus2,
    Blitz5,
    Rapid10
};

struct GameConfig
{
    GameMode mode =
        GameMode::PlayerVsComputer;

    bool humanIsWhite =
        true;

    Difficulty difficulty =
        Difficulty::Medium;

    TimeControl timeControl =
        TimeControl::Rapid10;

    std::string opponentName =
        "STOCKFISH";

    int getStockfishSkill() const
    {
        switch (difficulty)
        {
            case Difficulty::Easy:
                return 0;

            case Difficulty::Medium:
                return 5;

            case Difficulty::Hard:
                return 10;

            case Difficulty::Expert:
                return 15;

            case Difficulty::Master:
                return 20;
        }

        return 5;
    }

    int getStockfishMoveTime() const
    {
        switch (difficulty)
        {
            case Difficulty::Easy:
                return 100;

            case Difficulty::Medium:
                return 250;

            case Difficulty::Hard:
                return 400;

            case Difficulty::Expert:
                return 600;

            case Difficulty::Master:
                return 900;
        }

        return 250;
    }

    int getInitialTimeSeconds() const
    {
        switch (timeControl)
        {
            case TimeControl::NoClock:
                return 0;

            case TimeControl::Bullet1:
                return 60;

            case TimeControl::Blitz3:
                return 180;

            case TimeControl::Blitz3Plus2:
                return 180;

            case TimeControl::Blitz5:
                return 300;

            case TimeControl::Rapid10:
                return 600;
        }

        return 600;
    }

    int getIncrementSeconds() const
    {
        if (
            timeControl ==
            TimeControl::Blitz3Plus2)
        {
            return 2;
        }

        return 0;
    }
};

#endif