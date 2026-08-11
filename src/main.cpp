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

void synchronizeOnlineGame(
    ChessGame& game,
    const std::string& moves)
{
    game.newGame();

    if (moves.empty())
    {
        return;
    }

    std::size_t start = 0;

    while (start < moves.length())
    {
        std::size_t end =
            moves.find(
                ' ',
                start);

        std::string move;

        if (
            end ==
            std::string::npos)
        {
            move =
                moves.substr(start);

            start =
                moves.length();
        }
        else
        {
            move =
                moves.substr(
                    start,
                    end - start);

            start =
                end + 1;
        }

        if (move.empty())
        {
            continue;
        }

        MoveSound result =
            game.makeUCIMove(
                move);

        if (
            result ==
            MoveSound::Illegal)
        {
            std::cerr
                << "Could not synchronize Lichess move: "
                << move
                << std::endl;

            break;
        }
    }
}

bool submitOnlineMove(
    ChessGame& game,
    LichessClient& lichess,
    ChessClock& chessClock,
    const std::string& confirmedServerMoves)
{
    LichessGameInfo onlineGame =
        lichess.getCurrentGameSnapshot();

    if (
        !onlineGame.active ||
        onlineGame.gameId.empty())
    {
        return false;
    }

    const std::string& move =
        game.getLastMoveUCI();

    if (move.empty())
    {
        return false;
    }

    if (!lichess.sendMove(
            onlineGame.gameId,
            move))
    {
        // Server rejected it.
        // Restore official Lichess position.
        synchronizeOnlineGame(
            game,
            confirmedServerMoves);

        return false;
    }

    chessClock.onOnlineMove();

    return true;
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
        else
        {
            if (!lichess.startEventStream())
            {
                std::cerr
                    << "Could not start Lichess event stream: "
                    << lichess.getLastError()
                    << std::endl;
            }
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

    std::string lastOnlineMoves;

    bool onlineGameLoaded =
        false;

    std::string requestedGameStreamId;

    int lastOnlineWhiteTimeMs =
        -1;

    int lastOnlineBlackTimeMs =
        -1;

    bool onlineResignConfirm =
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

        // ========================================================
        // LICHESS GAME STREAM START
        // ========================================================

        {
            LichessGameInfo onlineGame =
                lichess.getCurrentGameSnapshot();

            if (
                onlineGame.active &&
                !onlineGame.gameId.empty() &&
                !lichess.isGameStreamRunning() &&
                requestedGameStreamId !=
                    onlineGame.gameId)
            {
                std::cout
                    << "[LICHESS] Requesting game stream for: "
                    << onlineGame.gameId
                    << std::endl;

                if (
                    lichess.startGameStream(
                        onlineGame.gameId))
                {
                    requestedGameStreamId =
                        onlineGame.gameId;
                }
                else
                {
                    std::cerr
                        << "[LICHESS] Could not start game stream: "
                        << lichess.getLastError()
                        << std::endl;
                }
            }
        }

        // ========================================================
        // LICHESS GAME SYNCHRONIZATION
        // ========================================================

        {
            LichessGameInfo onlineGame =
                lichess.getCurrentGameSnapshot();

            if (
                onlineGame.active &&
                onlineGame.initialized)
            {
                // ----------------------------------------------------
                // FIRST LOAD
                // ----------------------------------------------------

                if (!onlineGameLoaded)
                {
                    currentConfig.mode =
                        GameMode::Online;

                    currentConfig.humanIsWhite =
                        onlineGame.playerIsWhite;

                    if (onlineGame.playerIsWhite)
                    {
                        currentConfig.opponentName =
                            onlineGame.blackUsername;
                    }
                    else
                    {
                        currentConfig.opponentName =
                            onlineGame.whiteUsername;
                    }

                    synchronizeOnlineGame(
                        game,
                        onlineGame.moves);

                    lastOnlineMoves =
                        onlineGame.moves;

                    chessClock.syncFromServer(
                        onlineGame.whiteTimeMs,
                        onlineGame.blackTimeMs,
                        game.isWhiteTurn(),
                        onlineGame.status ==
                            "started");

                    lastOnlineWhiteTimeMs =
                        onlineGame.whiteTimeMs;

                    lastOnlineBlackTimeMs =
                        onlineGame.blackTimeMs;

                    cursorRow =
                        onlineGame.playerIsWhite
                            ? 7
                            : 0;

                    cursorCol = 4;

                    computerMovePending =
                        false;

                    stockfish.stop();

                    appState =
                        AppState::Playing;

                    onlineGameLoaded =
                        true;

                    audio.play(
                        "start");
                }

                // ----------------------------------------------------
                // NEW REMOTE MOVE
                // ----------------------------------------------------

                else if (
                    onlineGame.moves !=
                        lastOnlineMoves ||
                    onlineGame.whiteTimeMs !=
                        lastOnlineWhiteTimeMs ||
                    onlineGame.blackTimeMs !=
                        lastOnlineBlackTimeMs)
                {
                    bool movesChanged =
                        onlineGame.moves !=
                        lastOnlineMoves;

                    if (movesChanged)
                    {
                        synchronizeOnlineGame(
                            game,
                            onlineGame.moves);

                        lastOnlineMoves =
                            onlineGame.moves;

                        // If after synchronization it is our turn,
                        // the opponent was the player who just moved.
                        bool humanTurnNow =
                            currentConfig.humanIsWhite
                                ? game.isWhiteTurn()
                                : !game.isWhiteTurn();

                        if (humanTurnNow)
                        {
                            audio.play(
                                "move-opponent");
                        }
                    }

                    chessClock.syncFromServer(
                        onlineGame.whiteTimeMs,
                        onlineGame.blackTimeMs,
                        game.isWhiteTurn(),
                        onlineGame.status ==
                            "started");

                    lastOnlineWhiteTimeMs =
                        onlineGame.whiteTimeMs;

                    lastOnlineBlackTimeMs =
                        onlineGame.blackTimeMs;
                }
            }
        }

        // ====================================================
        // EVENTS
        // ====================================================

        if (
            appState ==
                AppState::Playing &&
            !game.isGameOver())
        {
            chessClock.update();

            // In Online mode Lichess decides
            // the official result.
            if (
                currentConfig.mode !=
                GameMode::Online)
            {
                if (chessClock.whiteFlagged())
                {
                    game.declareTimeout(
                        true);

                    chessClock.stop();

                    audio.play(
                        "checkmate");
                }
                else if (
                    chessClock.blackFlagged())
                {
                    game.declareTimeout(
                        false);

                    chessClock.stop();

                    audio.play(
                        "checkmate");
                }
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
                    // =================================================
                    // LICHESS INCOMING CHALLENGE
                    // =================================================

                    if (
                        menu.getScreen() ==
                            MenuScreen::Online &&
                        lichess.hasIncomingChallenge())
                    {
                        // ---------------------------------------------
                        // ACCEPT CHALLENGE
                        // ENTER / SPACE
                        // ---------------------------------------------

                        if (
                            event.key.keysym.sym ==
                                SDLK_RETURN ||
                            event.key.keysym.sym ==
                                SDLK_SPACE)
                        {
                            if (
                                lichess.acceptIncomingChallenge())
                            {
                                audio.play(
                                    "click");

                                std::cout
                                    << "Lichess challenge accepted."
                                    << std::endl;
                            }
                            else
                            {
                                std::cerr
                                    << "Could not accept challenge: "
                                    << lichess.getLastError()
                                    << std::endl;

                                audio.playMoveSound(
                                    MoveSound::Illegal);
                            }

                            continue;
                        }

                        // ---------------------------------------------
                        // DECLINE CHALLENGE
                        // BACKSPACE / ESC
                        // ---------------------------------------------

                        if (
                            event.key.keysym.sym ==
                                SDLK_BACKSPACE ||
                            event.key.keysym.sym ==
                                SDLK_ESCAPE)
                        {
                            if (
                                lichess.declineIncomingChallenge())
                            {
                                audio.play(
                                    "click");

                                std::cout
                                    << "Lichess challenge declined."
                                    << std::endl;
                            }
                            else
                            {
                                std::cerr
                                    << "Could not decline challenge: "
                                    << lichess.getLastError()
                                    << std::endl;

                                audio.playMoveSound(
                                    MoveSound::Illegal);
                            }

                            continue;
                        }
                    }

                    // =================================================
                    // NORMAL MENU CONTROLS
                    // =================================================

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
                    // =================================================
                    // LICHESS INCOMING CHALLENGE
                    // =================================================

                    if (
                        menu.getScreen() ==
                            MenuScreen::Online &&
                        lichess.hasIncomingChallenge())
                    {
                        // A = ACCEPT
                        if (
                            event.cbutton.button ==
                                SDL_CONTROLLER_BUTTON_A)
                        {
                            if (
                                lichess.acceptIncomingChallenge())
                            {
                                audio.play(
                                    "click");

                                std::cout
                                    << "Lichess challenge accepted."
                                    << std::endl;
                            }
                            else
                            {
                                std::cerr
                                    << "Could not accept challenge: "
                                    << lichess.getLastError()
                                    << std::endl;

                                audio.playMoveSound(
                                    MoveSound::Illegal);
                            }

                            continue;
                        }

                        // B = DECLINE
                        if (
                            event.cbutton.button ==
                                SDL_CONTROLLER_BUTTON_B)
                        {
                            if (
                                lichess.declineIncomingChallenge())
                            {
                                audio.play(
                                    "click");

                                std::cout
                                    << "Lichess challenge declined."
                                    << std::endl;
                            }
                            else
                            {
                                std::cerr
                                    << "Could not decline challenge: "
                                    << lichess.getLastError()
                                    << std::endl;

                                audio.playMoveSound(
                                    MoveSound::Illegal);
                            }

                            continue;
                        }
                    }

                    // =================================================
                    // NORMAL MENU CONTROLS
                    // =================================================

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
                if (
                    currentConfig.mode ==
                        GameMode::Online &&
                    onlineResignConfirm)
                {
                    // ENTER / SPACE = YES
                    if (
                        event.key.keysym.sym ==
                            SDLK_RETURN ||
                        event.key.keysym.sym ==
                            SDLK_SPACE)
                    {
                        LichessGameInfo onlineGame =
                            lichess.getCurrentGameSnapshot();

                        if (
                            lichess.resignGame(
                                onlineGame.gameId))
                        {
                            chessClock.stop();

                            lichess.stopGameStream();

                            requestedGameStreamId.clear();
                            lastOnlineMoves.clear();

                            lastOnlineWhiteTimeMs =
                                -1;

                            lastOnlineBlackTimeMs =
                                -1;

                            onlineGameLoaded =
                                false;

                            onlineResignConfirm =
                                false;

                            lichess.clearCurrentGame();

                            menu.reset();

                            appState =
                                AppState::Menu;

                            audio.play(
                                "game-end");
                        }

                        continue;
                    }

                    // ESC / BACKSPACE = NO
                    if (
                        event.key.keysym.sym ==
                            SDLK_ESCAPE ||
                        event.key.keysym.sym ==
                            SDLK_BACKSPACE)
                    {
                        onlineResignConfirm =
                            false;

                        continue;
                    }

                    continue;
                }

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

                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    if (!submitOnlineMove(
                                            game,
                                            lichess,
                                            chessClock,
                                            lastOnlineMoves))
                                    {
                                        audio.playMoveSound(
                                            MoveSound::Illegal);
                                    }
                                }
                                else
                                {
                                    updateClockAfterMove(
                                        previousTurn,
                                        game,
                                        chessClock);
                                }

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDLK_ESCAPE:
                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    onlineResignConfirm =
                                        true;

                                    break;
                                }
                                game.cancelSelection();

                                chessClock.stop();

                                stockfish.stop();

                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    lichess.stopGameStream();

                                    requestedGameStreamId.clear();

                                    lastOnlineMoves.clear();

                                    onlineGameLoaded =
                                        false;
                                }

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

                            if (
                                currentConfig.mode ==
                                GameMode::Online)
                            {
                                lichess.stopGameStream();

                                requestedGameStreamId.clear();

                                lastOnlineMoves.clear();

                                onlineGameLoaded =
                                    false;
                            }

                            menu.reset();

                            appState =
                                AppState::Menu;

                            break;

                        case SDLK_UP:

                            if (!game.isGameOver())
                            {
                                bool flipped =
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    GameMode::PlayerVsComputer ||
                                currentConfig.mode ==
                                    GameMode::Online)
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
                                !game.isPromotionPending() &&
                                previousTurn !=
                                    game.isWhiteTurn())
                            {
                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    if (!submitOnlineMove(
                                            game,
                                            lichess,
                                            chessClock,
                                            lastOnlineMoves))
                                    {
                                        audio.playMoveSound(
                                            MoveSound::Illegal);
                                    }
                                }
                                else
                                {
                                    updateClockAfterMove(
                                        previousTurn,
                                        game,
                                        chessClock);
                                }
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
                            if (
                                currentConfig.mode ==
                                GameMode::Online)
                            {
                                break;
                            }

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
                        currentConfig.mode ==
                            GameMode::Online &&
                        onlineResignConfirm)
                    {
                        // A = YES
                        if (
                            event.cbutton.button ==
                            SDL_CONTROLLER_BUTTON_A)
                        {
                            LichessGameInfo onlineGame =
                                lichess.getCurrentGameSnapshot();

                            if (
                                lichess.resignGame(
                                    onlineGame.gameId))
                            {
                                chessClock.stop();

                                lichess.stopGameStream();

                                requestedGameStreamId.clear();
                                lastOnlineMoves.clear();

                                lastOnlineWhiteTimeMs =
                                    -1;

                                lastOnlineBlackTimeMs =
                                    -1;

                                onlineGameLoaded =
                                    false;

                                onlineResignConfirm =
                                    false;

                                lichess.clearCurrentGame();

                                menu.reset();

                                appState =
                                    AppState::Menu;

                                audio.play(
                                    "game-end");
                            }

                            continue;
                        }

                        // B = NO
                        if (
                            event.cbutton.button ==
                            SDL_CONTROLLER_BUTTON_B)
                        {
                            onlineResignConfirm =
                                false;

                            audio.play(
                                "click");

                            continue;
                        }

                        continue;
                    }
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

                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    if (!submitOnlineMove(
                                            game,
                                            lichess,
                                            chessClock,
                                            lastOnlineMoves))
                                    {
                                        audio.playMoveSound(
                                            MoveSound::Illegal);
                                    }
                                }
                                else
                                {
                                    updateClockAfterMove(
                                        previousTurn,
                                        game,
                                        chessClock);
                                }

                                audio.playMoveSound(
                                    sound);

                                computerMovePending =
                                    true;

                                break;
                            }

                            case SDL_CONTROLLER_BUTTON_B:
                                chessClock.stop();

                                stockfish.stop();

                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    lichess.stopGameStream();

                                    requestedGameStreamId.clear();

                                    lastOnlineMoves.clear();

                                    onlineGameLoaded =
                                        false;
                                }

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    (
                                        currentConfig.mode ==
                                            GameMode::PlayerVsComputer ||
                                        currentConfig.mode ==
                                            GameMode::Online
                                    ) &&
                                    !currentConfig.humanIsWhite;

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
                                    GameMode::PlayerVsComputer ||
                                currentConfig.mode ==
                                    GameMode::Online)
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
                                !game.isPromotionPending() &&
                                previousTurn !=
                                    game.isWhiteTurn())
                            {
                                if (
                                    currentConfig.mode ==
                                    GameMode::Online)
                                {
                                    if (!submitOnlineMove(
                                            game,
                                            lichess,
                                            chessClock,
                                            lastOnlineMoves))
                                    {
                                        audio.playMoveSound(
                                            MoveSound::Illegal);
                                    }
                                }
                                else
                                {
                                    updateClockAfterMove(
                                        previousTurn,
                                        game,
                                        chessClock);
                                }
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
                            if (
                                currentConfig.mode ==
                                GameMode::Online)
                            {
                                break;
                            }

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

                            if (
                                currentConfig.mode ==
                                GameMode::Online)
                            {
                                onlineResignConfirm =
                                    true;
                            }
                            else
                            {
                                chessClock.stop();

                                stockfish.stop();

                                menu.reset();

                                appState =
                                    AppState::Menu;
                            }

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
                opponentName,
                onlineResignConfirm);

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