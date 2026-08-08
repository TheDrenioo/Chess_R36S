#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <cmath>

#include <iostream>
#include <map>
#include <string>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

const int BOARD_SIZE = 480;
const int TILE_SIZE = BOARD_SIZE / 8;

const int BOARD_X = 80;
const int BOARD_Y = 0;

struct Piece
{
    char type;
    bool white;
};

Piece board[8][8];

std::map<std::string, SDL_Texture*> textures;

int cursorRow = 7;
int cursorCol = 0;

bool pieceSelected = false;
int selectedRow = -1;
int selectedCol = -1;

// ----------------------------------------------------
// Board setup
// ----------------------------------------------------

void setupBoard()
{
    board[0][0] = {'r', false};
    board[0][1] = {'n', false};
    board[0][2] = {'b', false};
    board[0][3] = {'q', false};
    board[0][4] = {'k', false};
    board[0][5] = {'b', false};
    board[0][6] = {'n', false};
    board[0][7] = {'r', false};

    for (int col = 0; col < 8; col++)
    {
        board[1][col] = {'p', false};
    }

    for (int row = 2; row < 6; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            board[row][col] = {' ', false};
        }
    }

    for (int col = 0; col < 8; col++)
    {
        board[6][col] = {'p', true};
    }

    board[7][0] = {'r', true};
    board[7][1] = {'n', true};
    board[7][2] = {'b', true};
    board[7][3] = {'q', true};
    board[7][4] = {'k', true};
    board[7][5] = {'b', true};
    board[7][6] = {'n', true};
    board[7][7] = {'r', true};
}

// ----------------------------------------------------
// Texture loading
// ----------------------------------------------------

SDL_Texture* loadTexture(
    SDL_Renderer* renderer,
    const std::string& path)
{
    SDL_Surface* surface = IMG_Load(path.c_str());

    if (!surface)
    {
        std::cerr
            << "Error loading "
            << path
            << ": "
            << IMG_GetError()
            << std::endl;

        return nullptr;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}

void loadPieceTextures(SDL_Renderer* renderer)
{
    textures["wk"] =
        loadTexture(renderer, "assets/pieces/wk.png");

    textures["wq"] =
        loadTexture(renderer, "assets/pieces/wq.png");

    textures["wr"] =
        loadTexture(renderer, "assets/pieces/wr.png");

    textures["wb"] =
        loadTexture(renderer, "assets/pieces/wb.png");

    textures["wn"] =
        loadTexture(renderer, "assets/pieces/wn.png");

    textures["wp"] =
        loadTexture(renderer, "assets/pieces/wp.png");

    textures["bk"] =
        loadTexture(renderer, "assets/pieces/bk.png");

    textures["bq"] =
        loadTexture(renderer, "assets/pieces/bq.png");

    textures["br"] =
        loadTexture(renderer, "assets/pieces/br.png");

    textures["bb"] =
        loadTexture(renderer, "assets/pieces/bb.png");

    textures["bn"] =
        loadTexture(renderer, "assets/pieces/bn.png");

    textures["bp"] =
        loadTexture(renderer, "assets/pieces/bp.png");
}

std::string getPieceKey(const Piece& piece)
{
    if (piece.type == ' ')
    {
        return "";
    }

    std::string key;

    key += piece.white ? 'w' : 'b';
    key += piece.type;

    return key;
}

// ----------------------------------------------------
// Rendering
// ----------------------------------------------------

void drawBoard(SDL_Renderer* renderer)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            bool lightSquare =
                (row + col) % 2 == 0;

            if (lightSquare)
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    238,
                    238,
                    210,
                    255);
            }
            else
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    118,
                    150,
                    86,
                    255);
            }

            SDL_Rect square = {
                BOARD_X + col * TILE_SIZE,
                BOARD_Y + row * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE
            };

            SDL_RenderFillRect(
                renderer,
                &square);
        }
    }
}

void drawPieces(SDL_Renderer* renderer)
{
    const int PIECE_PADDING = 3;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            const Piece& piece =
                board[row][col];

            if (piece.type == ' ')
            {
                continue;
            }

            std::string key =
                getPieceKey(piece);

            auto it =
                textures.find(key);

            if (it == textures.end())
            {
                continue;
            }

            SDL_Texture* texture =
                it->second;

            SDL_Rect destination = {
                BOARD_X +
                    col * TILE_SIZE +
                    PIECE_PADDING,

                BOARD_Y +
                    row * TILE_SIZE +
                    PIECE_PADDING,

                TILE_SIZE -
                    PIECE_PADDING * 2,

                TILE_SIZE -
                    PIECE_PADDING * 2
            };

            SDL_RenderCopy(
                renderer,
                texture,
                nullptr,
                &destination);
        }
    }
}

void drawCursor(SDL_Renderer* renderer)
{
    SDL_Rect cursorRect = {
        BOARD_X + cursorCol * TILE_SIZE,
        BOARD_Y + cursorRow * TILE_SIZE,
        TILE_SIZE,
        TILE_SIZE
    };

    SDL_SetRenderDrawColor(
        renderer,
        255,
        215,
        0,
        255);

    for (int i = 0; i < 4; i++)
    {
        SDL_Rect border = {
            cursorRect.x + i,
            cursorRect.y + i,
            cursorRect.w - i * 2,
            cursorRect.h - i * 2
        };

        SDL_RenderDrawRect(
            renderer,
            &border);
    }
}

void drawSelectedSquare(SDL_Renderer* renderer)
{
    if (!pieceSelected)
    {
        return;
    }

    SDL_Rect selectedRect = {
        BOARD_X + selectedCol * TILE_SIZE,
        BOARD_Y + selectedRow * TILE_SIZE,
        TILE_SIZE,
        TILE_SIZE
    };

    SDL_SetRenderDrawColor(
        renderer,
        255,
        140,
        0,
        255);

    for (int i = 0; i < 5; i++)
    {
        SDL_Rect border = {
            selectedRect.x + i,
            selectedRect.y + i,
            selectedRect.w - i * 2,
            selectedRect.h - i * 2
        };

        SDL_RenderDrawRect(
            renderer,
            &border);
    }
}

// ----------------------------------------------------
// Coordinates
// ----------------------------------------------------

void drawTinyLetter(
    SDL_Renderer* renderer,
    char letter,
    int x,
    int y)
{
    const int w = 6;
    const int h = 8;

    switch (letter)
    {
        case 'a':
            SDL_RenderDrawLine(renderer, x, y + h, x + 3, y);
            SDL_RenderDrawLine(renderer, x + 3, y, x + w, y + h);
            SDL_RenderDrawLine(renderer, x + 1, y + 4, x + 5, y + 4);
            break;

        case 'b':
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y, x + 5, y);
            SDL_RenderDrawLine(renderer, x + 5, y, x + 5, y + 4);
            SDL_RenderDrawLine(renderer, x + 5, y + 4, x, y + 4);
            SDL_RenderDrawLine(renderer, x, y + 4, x + 5, y + 4);
            SDL_RenderDrawLine(renderer, x + 5, y + 4, x + 5, y + h);
            SDL_RenderDrawLine(renderer, x + 5, y + h, x, y + h);
            break;

        case 'c':
            SDL_RenderDrawLine(renderer, x + w, y, x, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            break;

        case 'd':
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y, x + 5, y);
            SDL_RenderDrawLine(renderer, x + 5, y, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y + 4, x + 5, y + h);
            SDL_RenderDrawLine(renderer, x + 5, y + h, x, y + h);
            break;

        case 'e':
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x, y + 4, x + 5, y + 4);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            break;

        case 'f':
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x, y + 4, x + 5, y + 4);
            break;

        case 'g':
            SDL_RenderDrawLine(renderer, x + w, y, x, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            SDL_RenderDrawLine(renderer, x + w, y + h, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y + 4, x + 3, y + 4);
            break;

        case 'h':
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h);
            SDL_RenderDrawLine(renderer, x, y + 4, x + w, y + 4);
            break;
    }
}

void drawTinyNumber(
    SDL_Renderer* renderer,
    int number,
    int x,
    int y)
{
    const int w = 6;
    const int h = 8;

    switch (number)
    {
        case 1:
            SDL_RenderDrawLine(renderer, x + 3, y, x + 3, y + h);
            break;

        case 2:
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x + w, y, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y + 4, x, y + h);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            break;

        case 3:
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h);
            SDL_RenderDrawLine(renderer, x, y + 4, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            break;

        case 4:
            SDL_RenderDrawLine(renderer, x, y, x, y + 4);
            SDL_RenderDrawLine(renderer, x, y + 4, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h);
            break;

        case 5:
            SDL_RenderDrawLine(renderer, x + w, y, x, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + 4);
            SDL_RenderDrawLine(renderer, x, y + 4, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y + 4, x + w, y + h);
            SDL_RenderDrawLine(renderer, x + w, y + h, x, y + h);
            break;

        case 6:
            SDL_RenderDrawLine(renderer, x + w, y, x, y);
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            SDL_RenderDrawLine(renderer, x + w, y + h, x + w, y + 4);
            SDL_RenderDrawLine(renderer, x + w, y + 4, x, y + 4);
            break;

        case 7:
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x + w, y, x, y + h);
            break;

        case 8:
            SDL_RenderDrawLine(renderer, x, y, x + w, y);
            SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
            SDL_RenderDrawLine(renderer, x, y, x, y + h);
            SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h);
            SDL_RenderDrawLine(renderer, x, y + 4, x + w, y + 4);
            break;
    }
}

void drawCoordinates(SDL_Renderer* renderer)
{
    for (int row = 0; row < 8; row++)
    {
        int rank = 8 - row;

        bool lightSquare =
            row % 2 == 0;

        if (lightSquare)
        {
            SDL_SetRenderDrawColor(
                renderer,
                118,
                150,
                86,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                238,
                238,
                210,
                255);
        }

        drawTinyNumber(
            renderer,
            rank,
            BOARD_X + 4,
            row * TILE_SIZE + 4);
    }

    for (int col = 0; col < 8; col++)
    {
        char file =
            static_cast<char>('a' + col);

        bool lightSquare =
            (7 + col) % 2 == 0;

        if (lightSquare)
        {
            SDL_SetRenderDrawColor(
                renderer,
                118,
                150,
                86,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                238,
                238,
                210,
                255);
        }

        drawTinyLetter(
            renderer,
            file,
            BOARD_X +
                col * TILE_SIZE +
                TILE_SIZE - 11,

            BOARD_SIZE - 12);
    }
}

// ----------------------------------------------------
// Input
// ----------------------------------------------------

void moveCursor(int rowOffset, int colOffset)
{
    cursorRow += rowOffset;
    cursorCol += colOffset;

    if (cursorRow < 0)
        cursorRow = 0;

    if (cursorRow > 7)
        cursorRow = 7;

    if (cursorCol < 0)
        cursorCol = 0;

    if (cursorCol > 7)
        cursorCol = 7;
}

void selectOrMovePiece()
{
    if (!pieceSelected)
    {
        if (board[cursorRow][cursorCol].type != ' ')
        {
            pieceSelected = true;

            selectedRow = cursorRow;
            selectedCol = cursorCol;
        }

        return;
    }

    if (selectedRow == cursorRow &&
        selectedCol == cursorCol)
    {
        pieceSelected = false;

        selectedRow = -1;
        selectedCol = -1;

        return;
    }

    board[cursorRow][cursorCol] =
        board[selectedRow][selectedCol];

    board[selectedRow][selectedCol] =
        {' ', false};

    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;
}

void cancelSelection()
{
    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;
}

// ----------------------------------------------------
// Cleanup
// ----------------------------------------------------

void destroyTextures()
{
    for (auto& item : textures)
    {
        if (item.second)
        {
            SDL_DestroyTexture(item.second);
        }
    }

    textures.clear();
}

// ----------------------------------------------------
// Main
// ----------------------------------------------------

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(
            SDL_INIT_VIDEO |
            SDL_INIT_GAMECONTROLLER |
            SDL_INIT_JOYSTICK) != 0)
    {
        std::cerr
            << "SDL_Init error: "
            << SDL_GetError()
            << std::endl;

        return 1;
    }

    int imageFlags =
        IMG_INIT_PNG;

    if ((IMG_Init(imageFlags) & imageFlags) != imageFlags)
    {
        std::cerr
            << "SDL_image error: "
            << IMG_GetError()
            << std::endl;

        SDL_Quit();

        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow(
            "Chess R36S",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN);

    if (!window)
    {
        std::cerr
            << "Window error: "
            << SDL_GetError()
            << std::endl;

        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED |
            SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        std::cerr
            << "Renderer error: "
            << SDL_GetError()
            << std::endl;

        SDL_DestroyWindow(window);

        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    setupBoard();

    loadPieceTextures(renderer);

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

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        running = false;
                        break;

                    case SDLK_UP:
                        moveCursor(-1, 0);
                        break;

                    case SDLK_DOWN:
                        moveCursor(1, 0);
                        break;

                    case SDLK_LEFT:
                        moveCursor(0, -1);
                        break;

                    case SDLK_RIGHT:
                        moveCursor(0, 1);
                        break;

                    case SDLK_RETURN:
                    case SDLK_SPACE:
                        selectOrMovePiece();
                        break;

                    case SDLK_BACKSPACE:
                        cancelSelection();
                        break;
                }
            }

            if (event.type ==
                SDL_CONTROLLERBUTTONDOWN)
            {
                switch (
                    event.cbutton.button)
                {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        moveCursor(-1, 0);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        moveCursor(1, 0);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        moveCursor(0, -1);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        moveCursor(0, 1);
                        break;

                    case SDL_CONTROLLER_BUTTON_A:
                        selectOrMovePiece();
                        break;

                    case SDL_CONTROLLER_BUTTON_B:
                        cancelSelection();
                        break;

                    case SDL_CONTROLLER_BUTTON_START:
                        running = false;
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(
            renderer,
            35,
            35,
            35,
            255);

        SDL_RenderClear(renderer);

        drawBoard(renderer);

        drawSelectedSquare(renderer);

        drawPieces(renderer);

        drawCoordinates(renderer);

        drawCursor(renderer);

        SDL_RenderPresent(renderer);
    }

    if (controller)
    {
        SDL_GameControllerClose(
            controller);
    }

    destroyTextures();

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    IMG_Quit();

    SDL_Quit();

    return 0;
}