#include "Menu.h"

Menu::Menu()
{
    reset();
}

void Menu::reset()
{
    screen =
        MenuScreen::Main;

    selectedOption = 0;
}

void Menu::moveUp()
{
    if (screen == MenuScreen::Main)
    {
        selectedOption--;

        if (selectedOption < 0)
        {
            selectedOption = 3;
        }
    }
    else if (
        screen ==
        MenuScreen::Settings)
    {
        selectedOption--;

        if (selectedOption < 0)
        {
            selectedOption = 2;
        }
    }
}

void Menu::moveDown()
{
    if (screen == MenuScreen::Main)
    {
        selectedOption++;

        if (selectedOption > 3)
        {
            selectedOption = 0;
        }
    }
    else if (
        screen ==
        MenuScreen::Settings)
    {
        selectedOption++;

        if (selectedOption > 2)
        {
            selectedOption = 0;
        }
    }
}

void Menu::moveLeft()
{
    if (
        screen !=
        MenuScreen::Settings)
    {
        return;
    }

    switch (selectedOption)
    {
        case 0:
            changeColor();
            break;

        case 1:
            changeDifficulty(-1);
            break;
    }
}

void Menu::moveRight()
{
    if (
        screen !=
        MenuScreen::Settings)
    {
        return;
    }

    switch (selectedOption)
    {
        case 0:
            changeColor();
            break;

        case 1:
            changeDifficulty(1);
            break;
    }
}

MenuAction Menu::select()
{
    if (screen == MenuScreen::Main)
    {
        switch (selectedOption)
        {
            case 0:
                config.mode =
                    GameMode::
                        PlayerVsComputer;

                return
                    MenuAction::StartGame;

            case 1:
                config.mode =
                    GameMode::
                        PlayerVsPlayer;

                return
                    MenuAction::StartGame;

            case 2:
                screen =
                    MenuScreen::Settings;

                selectedOption = 0;

                return
                    MenuAction::None;

            case 3:
                return
                    MenuAction::Exit;
        }
    }

    if (
        screen ==
        MenuScreen::Settings)
    {
        switch (selectedOption)
        {
            case 0:
                changeColor();
                break;

            case 1:
                changeDifficulty(1);
                break;

            case 2:
                screen =
                    MenuScreen::Main;

                selectedOption = 0;
                break;
        }
    }

    return MenuAction::None;
}

void Menu::back()
{
    if (
        screen ==
        MenuScreen::Settings)
    {
        screen =
            MenuScreen::Main;

        selectedOption = 0;
    }
}

MenuScreen Menu::getScreen() const
{
    return screen;
}

int Menu::getSelectedOption() const
{
    return selectedOption;
}

const GameConfig&
Menu::getConfig() const
{
    return config;
}

GameConfig&
Menu::getConfig()
{
    return config;
}

void Menu::changeColor()
{
    config.humanIsWhite =
        !config.humanIsWhite;
}

void Menu::changeDifficulty(
    int direction)
{
    int value =
        static_cast<int>(
            config.difficulty);

    value += direction;

    if (value < 0)
    {
        value = 4;
    }

    if (value > 4)
    {
        value = 0;
    }

    config.difficulty =
        static_cast<Difficulty>(
            value);
}