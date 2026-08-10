#include "AudioManager.h"
#include "ChessGame.h"
#include "GameConfig.h"
#include "Menu.h"
#include "Renderer.h"
#include "StockfishEngine.h"
#include "ChessClock.h"
#include "LichessClient.h"

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

void updateClockAfterMove(
    bool previousTurn,
    ChessGame& game,
    ChessClock& chessClock)
{
    // No actual move occurred.
    if (
        previousTurn ==
        game.isWhiteTurn())
    {
        return;
    }

    if (!chessClock.isEnabled())
    {
        return;
    }

    // If the move ended the game,
    // the clock should remain stopped.
    if (game.isGameOver())
    {
        chessClock.stop();

        return;
    }

    // ========================================================
    // FIRST MOVE OF THE GAME
    //
    // White has just moved.
    // Start the clock and immediately switch it to Black.
    // ========================================================

    if (!chessClock.isRunning())
    {
        if (previousTurn)
        {
            // Start on White with zero elapsed time.
            chessClock.start(true);

            // Apply White's increment and pass clock to Black.
            chessClock.onMove();
        }

        return;
    }

    // ========================================================
    // NORMAL MOVE
    // ========================================================

    chessClock.onMove();
}

void makeComputerMove(
    ChessGame& game,
    StockfishEngine& stockfish,
    AudioManager& audio,
    ChessClock& chessClock,
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

    bool previousTurn =
        game.isWhiteTurn();

    MoveSound sound =
        game.makeUCIMove(
            bestMove);

    updateClockAfterMove(
        previousTurn,
        game,
        chessClock);

    audio.playMoveSound(
        sound);
}

void moveVisualCursor(
    int& cursorRow,
    int& cursorCol,
    int visualRowDirection,
    int visualColDirection,
    bool boardFlipped)
{
    if (boardFlipped)
    {
        visualRowDirection =
            -visualRowDirection;

        visualColDirection =
            -visualColDirection;
    }

    cursorRow =
        std::clamp(
            cursorRow +
                visualRowDirection,
            0,
            7);

    cursorCol =
        std::clamp(
            cursorCol +
                visualColDirection,
            0,
            7);
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

    ChessClock chessClock;

    Menu menu;
    
    StockfishEngine stockfish;

    LichessClient lichess;

    if (!lichess.initialize())
    {
        std::cerr
            << "Could not initialize Lichess client: "
            << lichess.getLastError()
            << std::endl;
    }
    else
    {
        if (!lichess.authenticate())
        {
            std::cerr
                << "Lichess authentication failed: "
                << lichess.getLastError()
                << std::endl;
        }
    }


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

        if (
            appState ==
                AppState::Playing &&
            !game.isGameOver())
        {
            chessClock.update();

            if (chessClock.whiteFlagged())
            {
                game.declareTimeout(true);

                chessClock.stop();

                audio.play(
                    "checkmate");
            }
            else if (
                chessClock.blackFlagged())
            {
                game.declareTimeout(false);

                chessClock.stop();

                audio.play(
                    "checkmate");
            }
        }

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

                                if (
                                    currentConfig.mode ==
                                    GameMode::PlayerVsComputer)
                                {
                                    currentConfig.opponentName =
                                        "STOCKFISH";
                                }
                                else
                                {
                                    currentConfig.opponentName =
                                        "PLAYER 2";
                                }

                                game.newGame();

                                chessClock.configure(
                                    currentConfig
                                        .getInitialTimeSeconds(),
                                    currentConfig
                                        .getIncrementSeconds());

                                cursorRow = 7;
                                cursorCol = 4;

                                computerMovePending =
                                    false;

                                stockfish.stop();
                                lichess.shutdown();

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

                            if (
                                    currentConfig.mode ==
                                    GameMode::PlayerVsComputer)
                                {
                                    currentConfig.opponentName =
                                        "STOCKFISH";
                                }
                                else
                                {
                                    currentConfig.opponentName =
                                        "PLAYER 2";
                                }

                                game.newGame();

                                chessClock.configure(
                                    currentConfig
                                        .getInitialTimeSeconds(),
                                    currentConfig
                                        .getIncrementSeconds());

                                cursorRow = 7;
                                cursorCol = 4;

                                computerMovePending =
                                    false;

                                stockfish.stop();
                                lichess.shutdown();

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
                                bool previousTurn =
                                    game.isWhiteTurn();

                                MoveSound sound =
                                    game.confirmPromotion();

                                updateClockAfterMove(
                                    previousTurn,
                                    game,
                                    chessClock);

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDLK_ESCAPE:
                                game.cancelSelection();

                                chessClock.stop();

                                stockfish.stop();
                                lichess.shutdown();

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
                            chessClock.stop();

                            stockfish.stop();
                            lichess.shutdown();

                            menu.reset();

                            appState =
                                AppState::Menu;

                            break;

                        case SDLK_UP:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    -1,
                                    0,
                                    flipped);
                            }

                            break;

                        case SDLK_DOWN:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    1,
                                    0,
                                    flipped);
                            }

                            break;

                        case SDLK_LEFT:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    0,
                                    -1,
                                    flipped);
                            }

                            break;

                        case SDLK_RIGHT:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    0,
                                    1,
                                    flipped);
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

                            if (!game.isPromotionPending())
                            {
                                updateClockAfterMove(
                                    previousTurn,
                                    game,
                                    chessClock);
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

                            chessClock.configure(
                                currentConfig
                                    .getInitialTimeSeconds(),
                                currentConfig
                                    .getIncrementSeconds());


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
                                bool previousTurn =
                                    game.isWhiteTurn();

                                MoveSound sound =
                                    game.confirmPromotion();

                                updateClockAfterMove(
                                    previousTurn,
                                    game,
                                    chessClock);

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDL_CONTROLLER_BUTTON_B:
                                chessClock.stop();

                                stockfish.stop();
                                lichess.shutdown();

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
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    -1,
                                    0,
                                    flipped);
                            }

                            break;


                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    1,
                                    0,
                                    flipped);
                            }

                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:

                            if (!game.isGameOver())
                                {
                                    bool flipped =
                                        currentConfig.mode ==
                                            GameMode::
                                                PlayerVsComputer &&
                                        !currentConfig
                                            .humanIsWhite;

                                    moveVisualCursor(
                                        cursorRow,
                                        cursorCol,
                                        0,
                                        -1,
                                        flipped);
                                }

                                break;

                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    currentConfig.mode ==
                                        GameMode::
                                            PlayerVsComputer &&
                                    !currentConfig
                                        .humanIsWhite;

                                moveVisualCursor(
                                    cursorRow,
                                    cursorCol,
                                    0,
                                    1,
                                    flipped);
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

                            if (!game.isPromotionPending())
                            {
                                updateClockAfterMove(
                                    previousTurn,
                                    game,
                                    chessClock);
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

                            chessClock.configure(
                                currentConfig
                                    .getInitialTimeSeconds(),
                                currentConfig
                                    .getIncrementSeconds());

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
                            chessClock.stop();

                            stockfish.stop();
                            lichess.shutdown();

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
                menu,
                lichess);
        }
        else
        {
            // =================================================
            // BOARD ORIENTATION
            // =================================================

            bool boardFlipped = false;

            if (
                currentConfig.mode ==
                GameMode::PlayerVsComputer)
            {
                boardFlipped =
                    !currentConfig.humanIsWhite;
            }
            else if (
                currentConfig.mode ==
                    GameMode::Online &&
                lichess.getCurrentGame().active)
            {
                boardFlipped =
                    !lichess
                        .getCurrentGame()
                        .playerIsWhite;
            }

            // =================================================
            // OPPONENT INFORMATION
            // =================================================

            // =================================================
            // LOCAL PLAYER NAME
            // =================================================

            std::string playerName =
                "PLAYER";

            if (
                lichess.isConnected() &&
                !lichess.getUsername().empty())
            {
                playerName =
                    lichess.getUsername();
            }

            std::string opponentName =
                currentConfig.opponentName;

            // -------------------------------------------------
            // VS STOCKFISH
            // -------------------------------------------------

            if (
                currentConfig.mode ==
                GameMode::PlayerVsComputer)
            {
                opponentName =
                    "STOCKFISH";
            }

            // -------------------------------------------------
            // LOCAL TWO PLAYER
            // -------------------------------------------------

            else if (
                currentConfig.mode ==
                GameMode::PlayerVsPlayer)
            {
                opponentName =
                    "PLAYER 2";
            }

            // -------------------------------------------------
            // ONLINE LICHESS
            // -------------------------------------------------

            else if (
                currentConfig.mode ==
                    GameMode::Online &&
                lichess.getCurrentGame().active)
            {
                const LichessGameInfo& onlineGame =
                    lichess.getCurrentGame();

                if (onlineGame.playerIsWhite)
                {
                    // We are White.
                    // Opponent is Black.
                    opponentName =
                        onlineGame.blackUsername;
                }
                else
                {
                    // We are Black.
                    // Opponent is White.
                    opponentName =
                        onlineGame.whiteUsername;
                }
            }

            // =================================================
            // DRAW GAME
            // =================================================

            renderer.render(
                game,
                chessClock,
                cursorRow,
                cursorCol,
                boardFlipped,
                playerName,
                opponentName);

            // =================================================
            // STOCKFISH
            // =================================================

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
                        chessClock,
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
    lichess.shutdown();

    if (controller)
    {
        SDL_GameControllerClose(
            controller);
    }

    IMG_Quit();

    SDL_Quit();

    return 0;
}