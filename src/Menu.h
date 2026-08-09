#ifndef MENU_H
#define MENU_H

#include "GameConfig.h"

enum class MenuScreen
{
    Main,
    Settings
};

enum class MenuAction
{
    None,
    StartGame,
    Exit
};

class Menu
{
public:
    Menu();

    void reset();

    void moveUp();

    void moveDown();

    void moveLeft();

    void moveRight();

    MenuAction select();

    void back();

    MenuScreen getScreen() const;

    int getSelectedOption() const;

    const GameConfig&
    getConfig() const;

    GameConfig&
    getConfig();

private:
    MenuScreen screen =
        MenuScreen::Main;

    int selectedOption = 0;

    GameConfig config;

    void changeDifficulty(
        int direction);

    void changeColor();
};

#endif