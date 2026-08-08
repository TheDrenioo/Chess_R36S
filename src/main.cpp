#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <iostream>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;

const int BOARD_SIZE = 8;
const int SQUARE_SIZE = WINDOW_WIDTH / BOARD_SIZE;

void drawBoard(SDL_Renderer* renderer)
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            SDL_Rect square;

            square.x = col * SQUARE_SIZE;
            square.y = row * SQUARE_SIZE;
            square.w = SQUARE_SIZE;
            square.h = SQUARE_SIZE;

            if ((row + col) % 2 == 0)
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    240, 217, 181, 255
                );
            }
            else
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    181, 136, 99, 255
                );
            }

            SDL_RenderFillRect(renderer, &square);
        }
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "SDL_Init error: "
                  << SDL_GetError() << std::endl;

        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chess R36S",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        std::cerr << "Error creando ventana: "
                  << SDL_GetError() << std::endl;

        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer)
    {
        std::cerr << "Error creando renderer: "
                  << SDL_GetError() << std::endl;

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;

    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(
            renderer,
            0, 0, 0, 255
        );

        SDL_RenderClear(renderer);

        drawBoard(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}