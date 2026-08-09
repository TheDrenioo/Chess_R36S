#include "AudioManager.h"
#include "ChessGame.h"
#include "GameConfig.h"
#include "Menu.h"
#include "Renderer.h"
#include "StockfishEngine.h"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <iostream>
#include <string>

// ============================================================
// APPLICATION STATE
// ============================================================

enum class AppState
{
    Menu,
    Playing
};

// ============================================================
// COMPUTER MOVE
// ============================================================

void makeComputerMove(
    ChessGame& game,
    StockfishEngine& stockfish,
    AudioManager& audio,
    const GameConfig& config)
{
    if (
        config.mode !=
        GameMode::PlayerVsComputer)
    {
        return;
    }

    if (!stockfish.isRunning())
    {
        return;
    }

    if (
        game.isGameOver() ||
        game.isPromotionPending())
    {
        return;
    }

    bool computerTurn =
        config.humanIsWhite
            ? !game.isWhiteTurn()
            : game.isWhiteTurn();

    if (!computerTurn)
    {
        return;
    }

    std::string fen =
        game.getFEN();

    std::cout
        << "FEN: "
        << fen
        << std::endl;

    std::string bestMove =
        stockfish.getBestMove(
            fen,
            config.getStockfishMoveTime());

    if (
        bestMove.empty() ||
        bestMove == "(none)")
    {
        std::cerr
            << "Stockfish returned no move."
            << std::endl;

        return;
    }

    std::cout
        << "Stockfish move: "
        << bestMove
        << std::endl;

    MoveSound sound =
        game.makeUCIMove(
            bestMove);

    audio.playMoveSound(
        sound);
}

// ============================================================
// MAIN
// ============================================================

int main(
    int argc,
    char* argv[])
{
    (void)argc;
    (void)argv;

    // ========================================================
    // SDL
    // ========================================================

    if (
        SDL_Init(
            SDL_INIT_VIDEO |
            SDL_INIT_AUDIO |
            SDL_INIT_GAMECONTROLLER |
            SDL_INIT_JOYSTICK) != 0)
    {
        std::cerr
            << "SDL_Init error: "
            << SDL_GetError()
            << std::endl;

        return 1;
    }

    if (
        (IMG_Init(IMG_INIT_PNG) &
         IMG_INIT_PNG) !=
        IMG_INIT_PNG)
    {
        std::cerr
            << "SDL_image error: "
            << IMG_GetError()
            << std::endl;

        SDL_Quit();

        return 1;
    }

    // ========================================================
    // SYSTEMS
    // ========================================================

    Renderer renderer;

    if (!renderer.initialize())
    {
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    AudioManager audio;

    audio.initialize();

    ChessGame game;

    Menu menu;

    StockfishEngine stockfish;

    // ========================================================
    // APPLICATION
    // ========================================================

    AppState appState =
        AppState::Menu;

    GameConfig currentConfig;

    int cursorRow = 7;
    int cursorCol = 4;

    bool computerMovePending =
        false;

    // ========================================================
    // CONTROLLER
    // ========================================================

    SDL_GameController* controller =
        nullptr;

    for (int i = 0;
         i < SDL_NumJoysticks();
         i++)
    {
        if (SDL_IsGameController(i))
        {
            controller =
                SDL_GameControllerOpen(i);

            if (controller)
            {
                std::cout
                    << "Controller detected: "
                    << SDL_GameControllerName(
                           controller)
                    << std::endl;

                break;
            }
        }
    }

    // ========================================================
    // MAIN LOOP
    // ========================================================

    bool running = true;

    SDL_Event event;

    while (running)
    {
        // ====================================================
        // EVENTS
        // ====================================================

        while (SDL_PollEvent(&event))
        {
            if (
                event.type ==
                SDL_QUIT)
            {
                running = false;

                continue;
            }

            // =================================================
            // MENU
            // =================================================

            if (
                appState ==
                AppState::Menu)
            {
                if (
                    event.type ==
                    SDL_KEYDOWN)
                {
                    switch (
                        event.key.keysym.sym)
                    {
                        case SDLK_UP:
                            menu.moveUp();
                            audio.play("click");
                            break;

                        case SDLK_DOWN:
                            menu.moveDown();
                            audio.play("click");
                            break;

                        case SDLK_LEFT:
                            menu.moveLeft();
                            audio.play("click");
                            break;

                        case SDLK_RIGHT:
                            menu.moveRight();
                            audio.play("click");
                            break;

                        case SDLK_RETURN:
                        case SDLK_SPACE:
                        {
                            MenuAction action =
                                menu.select();

                            audio.play("click");

                            if (
                                action ==
                                MenuAction::Exit)
                            {
                                running =
                                    false;
                            }

                            if (
                                action ==
                                MenuAction::StartGame)
                            {
                                currentConfig =
                                    menu.getConfig();

                                game.newGame();

                                cursorRow = 7;
                                cursorCol = 4;

                                computerMovePending =
                                    false;

                                stockfish.stop();

                                if (
                                    currentConfig.mode ==
                                    GameMode::
                                        PlayerVsComputer)
                                {
                                    if (
                                        stockfish.start(
                                            "engine/stockfish.exe"))
                                    {
                                        stockfish.setSkillLevel(
                                            currentConfig
                                                .getStockfishSkill());

                                        std::cout
                                            << "Stockfish skill: "
                                            << currentConfig
                                                   .getStockfishSkill()
                                            << std::endl;
                                    }
                                    else
                                    {
                                        std::cerr
                                            << "Stockfish could not start."
                                            << std::endl;
                                    }
                                }

                                appState =
                                    AppState::Playing;

                                audio.play(
                                    "start");

                                // Computer starts if human selected black.
                                if (
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig.humanIsWhite)
                                {
                                    computerMovePending =
                                        true;
                                }
                            }

                            break;
                        }

                        case SDLK_BACKSPACE:
                        case SDLK_ESCAPE:
                            menu.back();
                            break;
                    }
                }

                if (
                    event.type ==
                    SDL_CONTROLLERBUTTONDOWN)
                {
                    switch (
                        event.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:
                            menu.moveUp();
                            audio.play("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                            menu.moveDown();
                            audio.play("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                            menu.moveLeft();
                            audio.play("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                            menu.moveRight();
                            audio.play("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_A:
                        {
                            MenuAction action =
                                menu.select();

                            audio.play("click");

                            if (
                                action ==
                                MenuAction::Exit)
                            {
                                running =
                                    false;
                            }

                            if (
                                action ==
                                MenuAction::StartGame)
                            {
                                currentConfig =
                                    menu.getConfig();

                                game.newGame();

                                cursorRow = 7;
                                cursorCol = 4;

                                computerMovePending =
                                    false;

                                stockfish.stop();

                                if (
                                    currentConfig.mode ==
                                    GameMode::
                                        PlayerVsComputer)
                                {
                                    if (
                                        stockfish.start(
                                            "engine/stockfish.exe"))
                                    {
                                        stockfish.setSkillLevel(
                                            currentConfig
                                                .getStockfishSkill());
                                    }
                                }

                                appState =
                                    AppState::Playing;

                                audio.play(
                                    "start");

                                if (
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig.humanIsWhite)
                                {
                                    computerMovePending =
                                        true;
                                }
                            }

                            break;
                        }

                        case SDL_CONTROLLER_BUTTON_B:
                            menu.back();
                            audio.play("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_START:
                            running = false;
                            break;
                    }
                }

                continue;
            }

            // =================================================
            // PLAYING
            // =================================================

            if (
                appState ==
                AppState::Playing)
            {
                // ---------------------------------------------
                // KEYBOARD
                // ---------------------------------------------

                if (
                    event.type ==
                    SDL_KEYDOWN)
                {
                    // Promotion
                    if (
                        game.isPromotionPending())
                    {
                        switch (
                            event.key.keysym.sym)
                        {
                            case SDLK_UP:
                                game.changePromotionChoice(
                                    -1);

                                audio.play("click");
                                break;

                            case SDLK_DOWN:
                                game.changePromotionChoice(
                                    1);

                                audio.play("click");
                                break;

                            case SDLK_RETURN:
                            case SDLK_SPACE:
                            {
                                MoveSound sound =
                                    game.confirmPromotion();

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDLK_ESCAPE:
                                game.cancelSelection();

                                stockfish.stop();

                                menu.reset();

                                appState =
                                    AppState::Menu;

                                break;
                        }

                        continue;
                    }

                    switch (
                        event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:

                            game.cancelSelection();

                            stockfish.stop();

                            menu.reset();

                            appState =
                                AppState::Menu;

                            break;

                        case SDLK_UP:

                            if (!game.isGameOver())
                            {
                                cursorRow =
                                    std::max(
                                        0,
                                        cursorRow - 1);
                            }

                            break;

                        case SDLK_DOWN:

                            if (!game.isGameOver())
                            {
                                cursorRow =
                                    std::min(
                                        7,
                                        cursorRow + 1);
                            }

                            break;

                        case SDLK_LEFT:

                            if (!game.isGameOver())
                            {
                                cursorCol =
                                    std::max(
                                        0,
                                        cursorCol - 1);
                            }

                            break;

                        case SDLK_RIGHT:

                            if (!game.isGameOver())
                            {
                                cursorCol =
                                    std::min(
                                        7,
                                        cursorCol + 1);
                            }

                            break;

                        case SDLK_RETURN:
                        case SDLK_SPACE:
                        {
                            bool humanCanMove =
                                true;

                            if (
                                currentConfig.mode ==
                                GameMode::
                                    PlayerVsComputer)
                            {
                                humanCanMove =
                                    currentConfig.humanIsWhite
                                        ? game.isWhiteTurn()
                                        : !game.isWhiteTurn();
                            }

                            if (!humanCanMove)
                            {
                                audio.playMoveSound(
                                    MoveSound::Illegal);

                                break;
                            }

                            bool previousTurn =
                                game.isWhiteTurn();

                            MoveSound sound =
                                game.moveSelectedPiece(
                                    cursorRow,
                                    cursorCol);

                            if (
                                sound ==
                                MoveSound::None)
                            {
                                audio.play(
                                    "click");
                            }
                            else
                            {
                                audio.playMoveSound(
                                    sound);
                            }

                            if (
                                currentConfig.mode ==
                                    GameMode::
                                        PlayerVsComputer &&
                                !game.isPromotionPending() &&
                                previousTurn !=
                                    game.isWhiteTurn())
                            {
                                computerMovePending =
                                    true;
                            }

                            break;
                        }

                        case SDLK_BACKSPACE:

                            game.cancelSelection();

                            break;

                        case SDLK_r:

                            game.newGame();

                            cursorRow = 7;
                            cursorCol = 4;

                            audio.play(
                                "start");

                            computerMovePending =
                                (
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig.humanIsWhite
                                );

                            break;
                    }
                }

                // ---------------------------------------------
                // CONTROLLER
                // ---------------------------------------------

                if (
                    event.type ==
                    SDL_CONTROLLERBUTTONDOWN)
                {
                    if (
                        game.isPromotionPending())
                    {
                        switch (
                            event.cbutton.button)
                        {
                            case SDL_CONTROLLER_BUTTON_DPAD_UP:

                                game.changePromotionChoice(
                                    -1);

                                audio.play("click");

                                break;

                            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:

                                game.changePromotionChoice(
                                    1);

                                audio.play("click");

                                break;

                            case SDL_CONTROLLER_BUTTON_A:
                            {
                                MoveSound sound =
                                    game.confirmPromotion();

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDL_CONTROLLER_BUTTON_B:

                                stockfish.stop();

                                menu.reset();

                                appState =
                                    AppState::Menu;

                                break;
                        }

                        continue;
                    }

                    switch (
                        event.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:

                            if (!game.isGameOver())
                            {
                                cursorRow =
                                    std::max(
                                        0,
                                        cursorRow - 1);
                            }

                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:

                            if (!game.isGameOver())
                            {
                                cursorRow =
                                    std::min(
                                        7,
                                        cursorRow + 1);
                            }

                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:

                            if (!game.isGameOver())
                            {
                                cursorCol =
                                    std::max(
                                        0,
                                        cursorCol - 1);
                            }

                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:

                            if (!game.isGameOver())
                            {
                                cursorCol =
                                    std::min(
                                        7,
                                        cursorCol + 1);
                            }

                            break;

                        case SDL_CONTROLLER_BUTTON_A:
                        {
                            bool humanCanMove =
                                true;

                            if (
                                currentConfig.mode ==
                                GameMode::
                                    PlayerVsComputer)
                            {
                                humanCanMove =
                                    currentConfig.humanIsWhite
                                        ? game.isWhiteTurn()
                                        : !game.isWhiteTurn();
                            }

                            if (!humanCanMove)
                            {
                                audio.playMoveSound(
                                    MoveSound::Illegal);

                                break;
                            }

                            bool previousTurn =
                                game.isWhiteTurn();

                            MoveSound sound =
                                game.moveSelectedPiece(
                                    cursorRow,
                                    cursorCol);

                            if (
                                sound ==
                                MoveSound::None)
                            {
                                audio.play(
                                    "click");
                            }
                            else
                            {
                                audio.playMoveSound(
                                    sound);
                            }

                            if (
                                currentConfig.mode ==
                                    GameMode::
                                        PlayerVsComputer &&
                                !game.isPromotionPending() &&
                                previousTurn !=
                                    game.isWhiteTurn())
                            {
                                computerMovePending =
                                    true;
                            }

                            break;
                        }

                        case SDL_CONTROLLER_BUTTON_B:

                            game.cancelSelection();

                            break;

                        case SDL_CONTROLLER_BUTTON_BACK:

                            game.newGame();

                            cursorRow = 7;
                            cursorCol = 4;

                            audio.play(
                                "start");

                            computerMovePending =
                                (
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig.humanIsWhite
                                );

                            break;

                        case SDL_CONTROLLER_BUTTON_START:

                            stockfish.stop();

                            menu.reset();

                            appState =
                                AppState::Menu;

                            break;
                    }
                }
            }
        }

        // ====================================================
        // RENDER
        // ====================================================

        if (
            appState ==
            AppState::Menu)
        {
            renderer.renderMenu(
                menu);
        }
        else
        {
            renderer.render(
                game,
                cursorRow,
                cursorCol);

            // ================================================
            // STOCKFISH
            // ================================================

            if (
                computerMovePending &&
                currentConfig.mode ==
                    GameMode::
                        PlayerVsComputer &&
                stockfish.isRunning() &&
                !game.isGameOver() &&
                !game.isPromotionPending())
            {
                bool computerTurn =
                    currentConfig.humanIsWhite
                        ? !game.isWhiteTurn()
                        : game.isWhiteTurn();

                if (computerTurn)
                {
                    makeComputerMove(
                        game,
                        stockfish,
                        audio,
                        currentConfig);
                }

                computerMovePending =
                    false;
            }
        }
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    stockfish.stop();

    if (controller)
    {
        SDL_GameControllerClose(
            controller);
    }

    IMG_Quit();

    SDL_Quit();

    return 0;
}