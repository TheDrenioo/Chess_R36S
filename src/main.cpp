#include "AudioManager.h"
#include "ChessGame.h"
#include "Renderer.h"
#include "StockfishEngine.h"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <iostream>
#include <string>

// ============================================================
// COMPUTER SETTINGS
// ============================================================

constexpr bool PLAY_VS_COMPUTER = true;

constexpr bool HUMAN_IS_WHITE = true;

constexpr int STOCKFISH_SKILL = 5;

constexpr int STOCKFISH_MOVE_TIME_MS = 300;

// ============================================================
// ENGINE MOVE
// ============================================================

void makeComputerMove(
    ChessGame& game,
    StockfishEngine& engine,
    AudioManager& audio)
{
    if (!PLAY_VS_COMPUTER)
    {
        return;
    }

    if (!engine.isRunning())
    {
        return;
    }

    if (game.isGameOver())
    {
        return;
    }

    if (game.isPromotionPending())
    {
        return;
    }

    bool computerTurn =
        HUMAN_IS_WHITE
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
        engine.getBestMove(
            fen,
            STOCKFISH_MOVE_TIME_MS);

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
        << "Stockfish: "
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
    // RENDERER
    // ========================================================

    Renderer renderer;

    if (!renderer.initialize())
    {
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    // ========================================================
    // AUDIO
    // ========================================================

    AudioManager audio;

    audio.initialize();

    // ========================================================
    // GAME
    // ========================================================

    ChessGame game;

    audio.play(
        "start");

    // ========================================================
    // STOCKFISH
    // ========================================================

    StockfishEngine stockfish;

    bool stockfishAvailable =
        false;

    if (PLAY_VS_COMPUTER)
    {
        stockfishAvailable =
            stockfish.start(
                "engine/stockfish.exe");

        if (stockfishAvailable)
        {
            stockfish.setSkillLevel(
                STOCKFISH_SKILL);

            std::cout
                << "Stockfish skill: "
                << STOCKFISH_SKILL
                << std::endl;
        }
        else
        {
            std::cerr
                << "Stockfish unavailable."
                << std::endl;
        }
    }

    // ========================================================
    // CURSOR
    // ========================================================

    int cursorRow = 7;
    int cursorCol = 4;

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

    bool computerMovePending =
        false;

    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (
                event.type ==
                SDL_QUIT)
            {
                running = false;
            }

            // =================================================
            // KEYBOARD
            // =================================================

            if (
                event.type ==
                SDL_KEYDOWN)
            {
                // ---------------------------------------------
                // PROMOTION
                // ---------------------------------------------

                if (game.isPromotionPending())
                {
                    switch (
                        event.key.keysym.sym)
                    {
                        case SDLK_UP:
                            game.changePromotionChoice(
                                -1);

                            audio.play(
                                "click");

                            break;

                        case SDLK_DOWN:
                            game.changePromotionChoice(
                                1);

                            audio.play(
                                "click");

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
                            running = false;
                            break;
                    }

                    continue;
                }

                // ---------------------------------------------
                // NORMAL GAME
                // ---------------------------------------------

                switch (
                    event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        running = false;
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
                        // Do not allow player input
                        // during Stockfish's side.

                        bool humanTurn =
                            HUMAN_IS_WHITE
                                ? game.isWhiteTurn()
                                : !game.isWhiteTurn();

                        if (!humanTurn)
                        {
                            audio.playMoveSound(
                                MoveSound::Illegal);

                            break;
                        }

                        bool wasWhiteTurn =
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

                        // Detect if a real move changed turn.
                        if (
                            !game.isPromotionPending() &&
                            wasWhiteTurn !=
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

                    // -----------------------------------------
                    // RESTART
                    // -----------------------------------------

                    case SDLK_r:
                        game.newGame();

                        cursorRow = 7;
                        cursorCol = 4;

                        computerMovePending =
                            !HUMAN_IS_WHITE;

                        audio.play(
                            "start");

                        break;
                }
            }

            // =================================================
            // CONTROLLER
            // =================================================

            if (
                event.type ==
                SDL_CONTROLLERBUTTONDOWN)
            {
                // ---------------------------------------------
                // PROMOTION
                // ---------------------------------------------

                if (game.isPromotionPending())
                {
                    switch (
                        event.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:
                            game.changePromotionChoice(
                                -1);

                            audio.play(
                                "click");

                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                            game.changePromotionChoice(
                                1);

                            audio.play(
                                "click");

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

                        case SDL_CONTROLLER_BUTTON_START:
                            running = false;
                            break;
                    }

                    continue;
                }

                // ---------------------------------------------
                // NORMAL INPUT
                // ---------------------------------------------

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
                        bool humanTurn =
                            HUMAN_IS_WHITE
                                ? game.isWhiteTurn()
                                : !game.isWhiteTurn();

                        if (!humanTurn)
                        {
                            audio.playMoveSound(
                                MoveSound::Illegal);

                            break;
                        }

                        bool wasWhiteTurn =
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
                            !game.isPromotionPending() &&
                            wasWhiteTurn !=
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

                    // SELECT
                    case SDL_CONTROLLER_BUTTON_BACK:

                        game.newGame();

                        cursorRow = 7;
                        cursorCol = 4;

                        computerMovePending =
                            !HUMAN_IS_WHITE;

                        audio.play(
                            "start");

                        break;

                    case SDL_CONTROLLER_BUTTON_START:

                        running = false;

                        break;
                }
            }
        }

        // =====================================================
        // RENDER BEFORE ENGINE THINKS
        // =====================================================

        renderer.render(
            game,
            cursorRow,
            cursorCol);

        // =====================================================
        // STOCKFISH TURN
        // =====================================================

        if (
            computerMovePending &&
            stockfishAvailable &&
            !game.isGameOver() &&
            !game.isPromotionPending())
        {
            bool computerTurn =
                HUMAN_IS_WHITE
                    ? !game.isWhiteTurn()
                    : game.isWhiteTurn();

            if (computerTurn)
            {
                makeComputerMove(
                    game,
                    stockfish,
                    audio);
            }

            computerMovePending =
                false;
        }

        // If computer starts as white later.
        if (
            PLAY_VS_COMPUTER &&
            stockfishAvailable &&
            !game.isGameOver())
        {
            bool computerTurn =
                HUMAN_IS_WHITE
                    ? !game.isWhiteTurn()
                    : game.isWhiteTurn();

            if (
                computerTurn &&
                !game.isPromotionPending() &&
                !computerMovePending)
            {
                computerMovePending =
                    true;
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