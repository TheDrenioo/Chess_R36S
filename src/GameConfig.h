#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

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

struct GameConfig
{
    GameMode mode =
        GameMode::PlayerVsComputer;

    bool humanIsWhite =
        true;

    Difficulty difficulty =
        Difficulty::Medium;

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
};

#endif