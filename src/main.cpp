#include "AudioManager.h"
#include "ChessGame.h"
#include "Renderer.h"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <iostream>

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

    audio.play("start");

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
    // LOOP
    // ========================================================

    bool running = true;

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
                // PROMOTION MENU
                // ---------------------------------------------

                if (game.isPromotionPending())
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
                        MoveSound sound =
                            game.moveSelectedPiece(
                                cursorRow,
                                cursorCol);

                        if (
                            sound ==
                            MoveSound::None)
                        {
                            audio.play("click");
                        }
                        else
                        {
                            audio.playMoveSound(
                                sound);
                        }

                        break;
                    }

                    case SDLK_BACKSPACE:
                        game.cancelSelection();
                        break;

                    // Restart
                    case SDLK_r:
                        game.newGame();

                        cursorRow = 7;
                        cursorCol = 4;

                        audio.play("start");

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
                // Promotion menu
                if (game.isPromotionPending())
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

                            break;
                        }

                        case SDL_CONTROLLER_BUTTON_START:
                            running = false;
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
                        MoveSound sound =
                            game.moveSelectedPiece(
                                cursorRow,
                                cursorCol);

                        if (
                            sound ==
                            MoveSound::None)
                        {
                            audio.play("click");
                        }
                        else
                        {
                            audio.playMoveSound(
                                sound);
                        }

                        break;
                    }

                    case SDL_CONTROLLER_BUTTON_B:
                        game.cancelSelection();
                        break;

                    // SELECT = restart
                    case SDL_CONTROLLER_BUTTON_BACK:
                        game.newGame();

                        cursorRow = 7;
                        cursorCol = 4;

                        audio.play("start");
                        break;

                    case SDL_CONTROLLER_BUTTON_START:
                        running = false;
                        break;
                }
            }
        }

        renderer.render(
            game,
            cursorRow,
            cursorCol);
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    if (controller)
    {
        SDL_GameControllerClose(
            controller);
    }

    IMG_Quit();

    SDL_Quit();

    return 0;
}