#include "Renderer.h"

#include <SDL_image.h>

#include <iostream>

#include <cctype>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    destroyTextures();

    if (renderer)
    {
        SDL_DestroyRenderer(
            renderer);
    }

    if (window)
    {
        SDL_DestroyWindow(
            window);
    }
}

// ============================================================
// INITIALIZATION
// ============================================================

bool Renderer::initialize()
{
    window =
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

        return false;
    }

    renderer =
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

        return false;
    }

    return loadTextures();
}

SDL_Renderer*
Renderer::getSDLRenderer()
{
    return renderer;
}

// ============================================================
// TEXTURES
// ============================================================

SDL_Texture* Renderer::loadTexture(
    const std::string& path)
{
    SDL_Surface* surface =
        IMG_Load(
            path.c_str());

    if (!surface)
    {
        std::cerr
            << "Image error: "
            << path
            << "\n"
            << IMG_GetError()
            << std::endl;

        return nullptr;
    }

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface);

    SDL_FreeSurface(surface);

    return texture;
}

bool Renderer::loadTextures()
{
    const std::string base =
        "assets/pieces/";

    const char* pieces[] = {
        "wk",
        "wq",
        "wr",
        "wb",
        "wn",
        "wp",
        "bk",
        "bq",
        "br",
        "bb",
        "bn",
        "bp"
    };

    for (const char* piece :
         pieces)
    {
        std::string name =
            piece;

        textures[name] =
            loadTexture(
                base +
                name +
                ".png");

        if (!textures[name])
        {
            return false;
        }
    }

    return true;
}

void Renderer::destroyTextures()
{
    for (auto& item : textures)
    {
        if (item.second)
        {
            SDL_DestroyTexture(
                item.second);
        }
    }

    textures.clear();
}

std::string Renderer::getPieceKey(
    const Piece& piece) const
{
    if (piece.type == ' ')
    {
        return "";
    }

    std::string key;

    key +=
        piece.white
            ? 'w'
            : 'b';

    key +=
        piece.type;

    return key;
}

// ============================================================
// BOARD
// ============================================================

void Renderer::drawBoard()
{
    for (int row = 0;
         row < 8;
         row++)
    {
        for (int col = 0;
             col < 8;
             col++)
        {
            bool light =
                (row + col) % 2 == 0;

            if (light)
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
                BOARD_X +
                    col * TILE_SIZE,

                BOARD_Y +
                    row * TILE_SIZE,

                TILE_SIZE,
                TILE_SIZE
            };

            SDL_RenderFillRect(
                renderer,
                &square);
        }
    }
}

// ============================================================
// PIECES
// ============================================================

void Renderer::drawPieces(
    const ChessGame& game)
{
    constexpr int padding = 3;

    for (int row = 0;
         row < 8;
         row++)
    {
        for (int col = 0;
             col < 8;
             col++)
        {
            const Piece& piece =
                game.getPiece(
                    row,
                    col);

            if (piece.type == ' ')
            {
                continue;
            }

            std::string key =
                getPieceKey(
                    piece);

            auto it =
                textures.find(key);

            if (
                it == textures.end() ||
                !it->second)
            {
                continue;
            }

            SDL_Rect destination = {
                BOARD_X +
                    col * TILE_SIZE +
                    padding,

                BOARD_Y +
                    row * TILE_SIZE +
                    padding,

                TILE_SIZE -
                    padding * 2,

                TILE_SIZE -
                    padding * 2
            };

            SDL_RenderCopy(
                renderer,
                it->second,
                nullptr,
                &destination);
        }
    }
}

// ============================================================
// SELECTION
// ============================================================

void Renderer::drawSelectedSquare(
    const ChessGame& game)
{
    if (!game.hasSelectedPiece())
    {
        return;
    }

    SDL_SetRenderDrawColor(
        renderer,
        255,
        170,
        0,
        255);

    SDL_Rect rect = {
        BOARD_X +
            game.getSelectedCol() *
                TILE_SIZE,

        BOARD_Y +
            game.getSelectedRow() *
                TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    for (int i = 0;
         i < 4;
         i++)
    {
        SDL_Rect border = {
            rect.x + i,
            rect.y + i,
            rect.w - i * 2,
            rect.h - i * 2
        };

        SDL_RenderDrawRect(
            renderer,
            &border);
    }
}

// ============================================================
// MOVE INDICATORS
// ============================================================

void Renderer::drawCircle(
    int centerX,
    int centerY,
    int radius)
{
    for (int y = -radius;
         y <= radius;
         y++)
    {
        for (int x = -radius;
             x <= radius;
             x++)
        {
            if (
                x * x +
                y * y <=
                radius * radius)
            {
                SDL_RenderDrawPoint(
                    renderer,
                    centerX + x,
                    centerY + y);
            }
        }
    }
}

void Renderer::drawMoveHistory(
    const ChessGame& game)
{
    const auto& history =
        game.getMoveHistory();

    if (history.empty())
    {
        return;
    }

    SDL_SetRenderDrawColor(
        renderer,
        215,
        215,
        215,
        255);

    constexpr int maxVisibleMoves =
        8;

    int total =
        static_cast<int>(
            history.size());

    int start =
        std::max(
            0,
            total -
                maxVisibleMoves);

    int y = 175;

    for (int i = start;
         i < total;
         i++)
    {
        std::string text;

        // White move
        if (i % 2 == 0)
        {
            int moveNumber =
                i / 2 + 1;

            text =
                std::to_string(
                    moveNumber);

            text += ".";
            text += history[i];
        }
        else
        {
            text =
                history[i];
        }

        drawSmallText(
            text,
            5,
            y,
            1);

        y += 28;
    }
}

void Renderer::drawLegalMoves(
    const ChessGame& game)
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    for (const Move& move :
         game.getLegalMoves())
    {
        int centerX =
            BOARD_X +
            move.toCol *
                TILE_SIZE +
            TILE_SIZE / 2;

        int centerY =
            BOARD_Y +
            move.toRow *
                TILE_SIZE +
            TILE_SIZE / 2;

        bool capture =
            game.getPiece(
                move.toRow,
                move.toCol)
                    .type != ' ' ||
            move.enPassant;

        if (!capture)
        {
            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                75);

            drawCircle(
                centerX,
                centerY,
                9);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                0,
                0,
                0,
                105);

            SDL_Rect rect = {
                BOARD_X +
                    move.toCol *
                        TILE_SIZE +
                    4,

                BOARD_Y +
                    move.toRow *
                        TILE_SIZE +
                    4,

                TILE_SIZE - 8,
                TILE_SIZE - 8
            };

            for (int i = 0;
                 i < 4;
                 i++)
            {
                SDL_RenderDrawRect(
                    renderer,
                    &rect);

                rect.x++;
                rect.y++;

                rect.w -= 2;
                rect.h -= 2;
            }
        }
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

// ============================================================
// CURSOR
// ============================================================

void Renderer::drawCursor(
    int row,
    int col)
{
    SDL_SetRenderDrawColor(
        renderer,
        255,
        215,
        0,
        255);

    SDL_Rect rect = {
        BOARD_X +
            col * TILE_SIZE,

        BOARD_Y +
            row * TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    for (int i = 0;
         i < 3;
         i++)
    {
        SDL_Rect border = {
            rect.x + i,
            rect.y + i,
            rect.w - i * 2,
            rect.h - i * 2
        };

        SDL_RenderDrawRect(
            renderer,
            &border);
    }
}

// ============================================================
// CHECK
// ============================================================

void Renderer::drawCheckIndicator(
    const ChessGame& game)
{
    bool checkedWhite;

    if (game.kingInCheck(true))
    {
        checkedWhite = true;
    }
    else if (game.kingInCheck(false))
    {
        checkedWhite = false;
    }
    else
    {
        return;
    }

    int kingRow = -1;
    int kingCol = -1;

    for (int row = 0;
         row < 8;
         row++)
    {
        for (int col = 0;
             col < 8;
             col++)
        {
            const Piece& piece =
                game.getPiece(
                    row,
                    col);

            if (
                piece.type == 'k' &&
                piece.white ==
                    checkedWhite)
            {
                kingRow = row;
                kingCol = col;
            }
        }
    }

    if (kingRow < 0)
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        renderer,
        220,
        40,
        40,
        115);

    SDL_Rect rect = {
        BOARD_X +
            kingCol *
                TILE_SIZE,

        BOARD_Y +
            kingRow *
                TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_RenderFillRect(
        renderer,
        &rect);

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

// ============================================================
// SIDE PANEL
// ============================================================

void Renderer::drawSidePanel(
    const ChessGame& game)
{
    SDL_SetRenderDrawColor(
        renderer,
        28,
        28,
        28,
        255);

    SDL_Rect panel = {
        0,
        0,
        BOARD_X,
        WINDOW_HEIGHT
    };

    SDL_RenderFillRect(
        renderer,
        &panel);

    if (game.isWhiteTurn())
    {
        SDL_SetRenderDrawColor(
            renderer,
            230,
            230,
            230,
            255);
    }
    else
    {
        SDL_SetRenderDrawColor(
            renderer,
            55,
            55,
            55,
            255);
    }

    SDL_Rect turnBox = {
        18,
        18,
        44,
        44
    };

    SDL_RenderFillRect(
        renderer,
        &turnBox);

    SDL_SetRenderDrawColor(
        renderer,
        130,
        130,
        130,
        255);

    SDL_RenderDrawRect(
        renderer,
        &turnBox);

    if (
        game.kingInCheck(
            game.isWhiteTurn()))
    {
        SDL_SetRenderDrawColor(
            renderer,
            210,
            45,
            45,
            255);

        SDL_Rect checkBox = {
            18,
            76,
            44,
            12
        };

        SDL_RenderFillRect(
            renderer,
            &checkBox);
    }

    if (game.isGameOver())
    {
        if (game.isDraw())
        {
            SDL_SetRenderDrawColor(
                renderer,
                160,
                160,
                160,
                255);
        }
        else
        {
            SDL_SetRenderDrawColor(
                renderer,
                220,
                170,
                20,
                255);
        }

        SDL_Rect resultBox = {
            18,
            105,
            44,
            44
        };

        SDL_RenderFillRect(
            renderer,
            &resultBox);
    }
}

// ============================================================
// COORDINATES
// ============================================================

void Renderer::drawLine(
    int x1,
    int y1,
    int x2,
    int y2)
{
    SDL_RenderDrawLine(
        renderer,
        x1,
        y1,
        x2,
        y2);
}

void Renderer::drawLetter(
    char letter,
    int x,
    int y)
{
    constexpr int w = 6;
    constexpr int h = 8;

    switch (letter)
    {
        case 'a':
            drawLine(x, y + h, x + 3, y);
            drawLine(x + 3, y, x + w, y + h);
            drawLine(x + 1, y + 4, x + 5, y + 4);
            break;

        case 'b':
            drawLine(x, y, x, y + h);
            drawLine(x, y, x + 5, y);
            drawLine(x + 5, y, x + 5, y + 4);
            drawLine(x + 5, y + 4, x, y + 4);
            drawLine(x + 5, y + 4, x + 5, y + h);
            drawLine(x + 5, y + h, x, y + h);
            break;

        case 'c':
            drawLine(x + w, y, x, y);
            drawLine(x, y, x, y + h);
            drawLine(x, y + h, x + w, y + h);
            break;

        case 'd':
            drawLine(x, y, x, y + h);
            drawLine(x, y, x + 5, y);
            drawLine(x + 5, y, x + w, y + 4);
            drawLine(x + w, y + 4, x + 5, y + h);
            drawLine(x + 5, y + h, x, y + h);
            break;

        case 'e':
            drawLine(x, y, x, y + h);
            drawLine(x, y, x + w, y);
            drawLine(x, y + 4, x + 5, y + 4);
            drawLine(x, y + h, x + w, y + h);
            break;

        case 'f':
            drawLine(x, y, x, y + h);
            drawLine(x, y, x + w, y);
            drawLine(x, y + 4, x + 5, y + 4);
            break;

        case 'g':
            drawLine(x + w, y, x, y);
            drawLine(x, y, x, y + h);
            drawLine(x, y + h, x + w, y + h);
            drawLine(x + w, y + h, x + w, y + 4);
            drawLine(x + w, y + 4, x + 3, y + 4);
            break;

        case 'h':
            drawLine(x, y, x, y + h);
            drawLine(x + w, y, x + w, y + h);
            drawLine(x, y + 4, x + w, y + 4);
            break;
    }
}

void Renderer::drawNumber(
    int number,
    int x,
    int y)
{
    constexpr int w = 6;
    constexpr int h = 8;

    switch (number)
    {
        case 1:
            drawLine(x + 3, y, x + 3, y + h);
            break;

        case 2:
            drawLine(x, y, x + w, y);
            drawLine(x + w, y, x + w, y + 4);
            drawLine(x + w, y + 4, x, y + h);
            drawLine(x, y + h, x + w, y + h);
            break;

        case 3:
            drawLine(x, y, x + w, y);
            drawLine(x + w, y, x + w, y + h);
            drawLine(x, y + 4, x + w, y + 4);
            drawLine(x, y + h, x + w, y + h);
            break;

        case 4:
            drawLine(x, y, x, y + 4);
            drawLine(x, y + 4, x + w, y + 4);
            drawLine(x + w, y, x + w, y + h);
            break;

        case 5:
            drawLine(x + w, y, x, y);
            drawLine(x, y, x, y + 4);
            drawLine(x, y + 4, x + w, y + 4);
            drawLine(x + w, y + 4, x + w, y + h);
            drawLine(x + w, y + h, x, y + h);
            break;

        case 6:
            drawLine(x + w, y, x, y);
            drawLine(x, y, x, y + h);
            drawLine(x, y + h, x + w, y + h);
            drawLine(x + w, y + h, x + w, y + 4);
            drawLine(x + w, y + 4, x, y + 4);
            break;

        case 7:
            drawLine(x, y, x + w, y);
            drawLine(x + w, y, x, y + h);
            break;

        case 8:
            drawLine(x, y, x + w, y);
            drawLine(x, y + h, x + w, y + h);
            drawLine(x, y, x, y + h);
            drawLine(x + w, y, x + w, y + h);
            drawLine(x, y + 4, x + w, y + 4);
            break;
    }
}

void Renderer::drawCoordinates()
{
    for (int row = 0;
         row < 8;
         row++)
    {
        int rank =
            8 - row;

        bool light =
            row % 2 == 0;

        if (light)
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

        drawNumber(
            rank,
            BOARD_X + 4,
            row * TILE_SIZE +
                4);
    }

    for (int col = 0;
         col < 8;
         col++)
    {
        char file =
            static_cast<char>(
                'a' + col);

        bool light =
            (7 + col) % 2 == 0;

        if (light)
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

        drawLetter(
            file,
            BOARD_X +
                col * TILE_SIZE +
                TILE_SIZE - 11,
            BOARD_SIZE - 12);
    }
}

// ============================================================
// PROMOTION MENU
// ============================================================

void Renderer::drawPromotionMenu(
    const ChessGame& game)
{
    if (!game.isPromotionPending())
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        renderer,
        15,
        15,
        15,
        235);

    SDL_Rect panel = {
        4,
        80,
        72,
        320
    };

    SDL_RenderFillRect(
        renderer,
        &panel);

    const char choices[4] = {
        'q',
        'r',
        'b',
        'n'
    };

    bool white =
        game.getPiece(
            game.getPromotionRow(),
            game.getPromotionCol())
            .white;

    for (int i = 0;
         i < 4;
         i++)
    {
        SDL_Rect box = {
            8,
            88 + i * 76,
            64,
            64
        };

        if (
            i ==
            game.getPromotionChoice())
        {
            SDL_SetRenderDrawColor(
                renderer,
                255,
                215,
                0,
                255);

            SDL_RenderDrawRect(
                renderer,
                &box);
        }

        Piece preview = {
            choices[i],
            white,
            true
        };

        std::string key =
            getPieceKey(
                preview);

        SDL_Rect pieceRect = {
            box.x + 4,
            box.y + 4,
            56,
            56
        };

        SDL_RenderCopy(
            renderer,
            textures[key],
            nullptr,
            &pieceRect);
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

// ============================================================
// COMPLETE FRAME
// ============================================================

void Renderer::render(
    const ChessGame& game,
    int cursorRow,
    int cursorCol)
{
    SDL_SetRenderDrawColor(
        renderer,
        35,
        35,
        35,
        255);

    SDL_RenderClear(
        renderer);

    drawSidePanel(game);

    drawMoveHistory(game);

    drawBoard();

    drawSelectedSquare(game);

    drawLegalMoves(game);

    drawCheckIndicator(game);

    drawPieces(game);

    drawCoordinates();

    if (
        !game.isGameOver() &&
        !game.isPromotionPending())
    {
        drawCursor(
            cursorRow,
            cursorCol);
    }

    drawPromotionMenu(game);

    SDL_RenderPresent(
        renderer);
}

// ============================================================
// SIMPLE MENU FONT
// ============================================================

void Renderer::drawSmallText(
    const std::string& text,
    int x,
    int y,
    int scale)
{
    int currentX =
        x;

    for (char character : text)
    {
        char upper =
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(
                        character)));

        drawCharacter(
            upper,
            currentX,
            y,
            scale);

        currentX +=
            6 * scale;
    }
}

void Renderer::drawCharacter(
    char character,
    int x,
    int y,
    int scale)
{
    const int segmentWidth =
        4 * scale;

    const int segmentHeight =
        6 * scale;

    SDL_Rect rect;

    auto horizontal =
        [&](int offsetY)
    {
        rect = {
            x,
            y + offsetY,
            segmentWidth,
            scale
        };

        SDL_RenderFillRect(
            renderer,
            &rect);
    };

    auto verticalLeft =
        [&](int offsetY)
    {
        rect = {
            x,
            y + offsetY,
            scale,
            segmentHeight / 2
        };

        SDL_RenderFillRect(
            renderer,
            &rect);
    };

    auto verticalRight =
        [&](int offsetY)
    {
        rect = {
            x +
                segmentWidth -
                scale,

            y + offsetY,

            scale,

            segmentHeight / 2
        };

        SDL_RenderFillRect(
            renderer,
            &rect);
    };

    switch (character)
    {
        case 'A':
            horizontal(0);
            horizontal(
                segmentHeight / 2);

            verticalLeft(0);
            verticalRight(0);

            verticalLeft(
                segmentHeight / 2);

            verticalRight(
                segmentHeight / 2);
            break;

        case 'B':
            horizontal(0);
            horizontal(
                segmentHeight / 2);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);
            break;

        case 'C':
            horizontal(0);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);
            break;

        case 'D':
            horizontal(0);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);
            break;

        case 'E':
            horizontal(0);
            horizontal(
                segmentHeight / 2);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);
            break;

        case 'F':
            horizontal(0);
            horizontal(
                segmentHeight / 2);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);
            break;

        case 'G':
            horizontal(0);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            horizontal(
                segmentHeight / 2);

            verticalRight(
                segmentHeight / 2);
            break;

        case 'H':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);

            horizontal(
                segmentHeight / 2);
            break;

        case 'I':
            horizontal(0);
            horizontal(
                segmentHeight);

            rect = {
                x +
                    segmentWidth / 2 -
                    scale / 2,

                y,

                scale,

                segmentHeight
            };

            SDL_RenderFillRect(
                renderer,
                &rect);
            break;

        case 'L':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            horizontal(
                segmentHeight);
            break;

        case 'M':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);

            rect = {
                x +
                    segmentWidth / 2 -
                    scale / 2,

                y,

                scale,

                segmentHeight / 2
            };

            SDL_RenderFillRect(
                renderer,
                &rect);
            break;

        case 'N':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);

            SDL_RenderDrawLine(
                renderer,
                x,
                y,
                x +
                    segmentWidth,
                y +
                    segmentHeight);
            break;

        case 'O':
            horizontal(0);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);
            break;

        case 'P':
            horizontal(0);
            horizontal(
                segmentHeight / 2);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            break;

        case 'R':
            horizontal(0);
            horizontal(
                segmentHeight / 2);

            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);

            SDL_RenderDrawLine(
                renderer,
                x +
                    segmentWidth / 2,
                y +
                    segmentHeight / 2,

                x +
                    segmentWidth,
                y +
                    segmentHeight);
            break;

        case 'S':
            horizontal(0);
            horizontal(
                segmentHeight / 2);
            horizontal(
                segmentHeight);

            verticalLeft(0);
            verticalRight(
                segmentHeight / 2);
            break;

        case 'T':
            horizontal(0);

            rect = {
                x +
                    segmentWidth / 2 -
                    scale / 2,

                y,

                scale,

                segmentHeight
            };

            SDL_RenderFillRect(
                renderer,
                &rect);
            break;

        case 'U':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);

            horizontal(
                segmentHeight);
            break;

        case 'V':
            verticalLeft(0);
            verticalRight(0);

            SDL_RenderDrawLine(
                renderer,
                x,
                y +
                    segmentHeight / 2,

                x +
                    segmentWidth / 2,
                y +
                    segmentHeight);

            SDL_RenderDrawLine(
                renderer,
                x +
                    segmentWidth,
                y +
                    segmentHeight / 2,

                x +
                    segmentWidth / 2,
                y +
                    segmentHeight);
            break;

        case 'W':
            verticalLeft(0);
            verticalLeft(
                segmentHeight / 2);

            verticalRight(0);
            verticalRight(
                segmentHeight / 2);

            horizontal(
                segmentHeight);
            break;

        case 'X':
            SDL_RenderDrawLine(
                renderer,
                x,
                y,
                x +
                    segmentWidth,
                y +
                    segmentHeight);

            SDL_RenderDrawLine(
                renderer,
                x +
                    segmentWidth,
                y,
                x,
                y +
                    segmentHeight);
            break;

        case 'Y':
            SDL_RenderDrawLine(
                renderer,
                x,
                y,
                x +
                    segmentWidth / 2,
                y +
                    segmentHeight / 2);

            SDL_RenderDrawLine(
                renderer,
                x +
                    segmentWidth,
                y,
                x +
                    segmentWidth / 2,
                y +
                    segmentHeight / 2);

            rect = {
                x +
                    segmentWidth / 2 -
                    scale / 2,

                y +
                    segmentHeight / 2,

                scale,

                segmentHeight / 2
            };

            SDL_RenderFillRect(
                renderer,
                &rect);
            break;

        case ' ':
            break;

        case '0':
            horizontal(0);
            horizontal(segmentHeight);
            verticalLeft(0);
            verticalLeft(segmentHeight / 2);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '1':
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '2':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalRight(0);
            verticalLeft(segmentHeight / 2);
            break;

        case '3':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '4':
            horizontal(segmentHeight / 2);
            verticalLeft(0);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '5':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalLeft(0);
            verticalRight(segmentHeight / 2);
            break;

        case '6':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalLeft(0);
            verticalLeft(segmentHeight / 2);
            verticalRight(segmentHeight / 2);
            break;

        case '7':
            horizontal(0);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '8':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalLeft(0);
            verticalLeft(segmentHeight / 2);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case '9':
            horizontal(0);
            horizontal(segmentHeight / 2);
            horizontal(segmentHeight);
            verticalLeft(0);
            verticalRight(0);
            verticalRight(segmentHeight / 2);
            break;

        case ':':
        {
            SDL_Rect topDot = {
                x + scale,
                y +
                    segmentHeight / 3,

                scale,
                scale
            };

            SDL_Rect bottomDot = {
                x + scale,
                y +
                    2 *
                    segmentHeight /
                    3,

                scale,
                scale
            };

            SDL_RenderFillRect(
                renderer,
                &topDot);

            SDL_RenderFillRect(
                renderer,
                &bottomDot);

            break;
        }

        case '>':
            SDL_RenderDrawLine(
                renderer,
                x,
                y,
                x +
                    segmentWidth,
                y +
                    segmentHeight / 2);

            SDL_RenderDrawLine(
                renderer,
                x +
                    segmentWidth,
                y +
                    segmentHeight / 2,

                x,
                y +
                    segmentHeight);
            break;

        case '.':
        {
            SDL_Rect dot = {
                x + scale,
                y + segmentHeight,
                scale,
                scale
            };

            SDL_RenderFillRect(
                renderer,
                &dot);

            break;
        }

        case '+':
        {
            SDL_Rect horizontalBar = {
                x,
                y + segmentHeight / 2,
                segmentWidth,
                scale
            };

            SDL_Rect verticalBar = {
                x + segmentWidth / 2,
                y + segmentHeight / 4,
                scale,
                segmentHeight / 2
            };

            SDL_RenderFillRect(
                renderer,
                &horizontalBar);

            SDL_RenderFillRect(
                renderer,
                &verticalBar);

            break;
        }

        case '#':
        {
            SDL_RenderDrawLine(
                renderer,
                x + scale,
                y,
                x + scale,
                y + segmentHeight);

            SDL_RenderDrawLine(
                renderer,
                x + 3 * scale,
                y,
                x + 3 * scale,
                y + segmentHeight);

            SDL_RenderDrawLine(
                renderer,
                x,
                y + 2 * scale,
                x + segmentWidth,
                y + 2 * scale);

            SDL_RenderDrawLine(
                renderer,
                x,
                y + 4 * scale,
                x + segmentWidth,
                y + 4 * scale);

            break;
        }

        case '=':
        {
            SDL_Rect top = {
                x,
                y + 2 * scale,
                segmentWidth,
                scale
            };

            SDL_Rect bottom = {
                x,
                y + 4 * scale,
                segmentWidth,
                scale
            };

            SDL_RenderFillRect(
                renderer,
                &top);

            SDL_RenderFillRect(
                renderer,
                &bottom);

            break;
        }



        default:
            break;
    }
}

void Renderer::drawMenuText(
    const std::string& text,
    int x,
    int y,
    int scale)
{
    int cursorX =
        x;

    for (char character : text)
    {
        drawCharacter(
            character,
            cursorX,
            y,
            scale);

        cursorX +=
            6 * scale;
    }
}

void Renderer::drawMenuItem(
    const std::string& text,
    int x,
    int y,
    bool selected)
{
    if (selected)
    {
        SDL_SetRenderDrawColor(
            renderer,
            255,
            215,
            0,
            255);

        drawMenuText(
            ">",
            x - 28,
            y,
            3);
    }

    SDL_SetRenderDrawColor(
        renderer,
        selected
            ? 255
            : 220,
        selected
            ? 215
            : 220,
        selected
            ? 0
            : 220,
        255);

    drawMenuText(
        text,
        x,
        y,
        3);
}

// ============================================================
// MENU RENDERER
// ============================================================

void Renderer::renderMenu(
    const Menu& menu)
{
    SDL_SetRenderDrawColor(
        renderer,
        24,
        24,
        24,
        255);

    SDL_RenderClear(
        renderer);

    // --------------------------------------------------------
    // TITLE
    // --------------------------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        238,
        238,
        210,
        255);

    drawMenuText(
        "CHESS R36S",
        190,
        55,
        5);

    // Decorative line

    SDL_SetRenderDrawColor(
        renderer,
        118,
        150,
        86,
        255);

    SDL_Rect lineRect = {
        140,
        115,
        360,
        4
    };

    SDL_RenderFillRect(
        renderer,
        &lineRect);

    // ========================================================
    // MAIN MENU
    // ========================================================

    if (
        menu.getScreen() ==
        MenuScreen::Main)
    {
        int selected =
            menu.getSelectedOption();

        drawMenuItem(
            "PLAY VS COMPUTER",
            180,
            170,
            selected == 0);

        drawMenuItem(
            "TWO PLAYERS",
            180,
            225,
            selected == 1);

        drawMenuItem(
            "SETTINGS",
            180,
            280,
            selected == 2);

        drawMenuItem(
            "EXIT",
            180,
            335,
            selected == 3);
    }

    // ========================================================
    // SETTINGS
    // ========================================================

    else
    {
        int selected =
            menu.getSelectedOption();

        const GameConfig& config =
            menu.getConfig();

        std::string colorText =
            config.humanIsWhite
                ? "COLOR: WHITE"
                : "COLOR: BLACK";

        std::string difficultyText =
            "DIFFICULTY: ";

        switch (config.difficulty)
        {
            case Difficulty::Easy:
                difficultyText +=
                    "EASY";
                break;

            case Difficulty::Medium:
                difficultyText +=
                    "MEDIUM";
                break;

            case Difficulty::Hard:
                difficultyText +=
                    "HARD";
                break;

            case Difficulty::Expert:
                difficultyText +=
                    "EXPERT";
                break;

            case Difficulty::Master:
                difficultyText +=
                    "MASTER";
                break;
        }

        drawMenuItem(
            colorText,
            145,
            180,
            selected == 0);

        drawMenuItem(
            difficultyText,
            145,
            245,
            selected == 1);

        drawMenuItem(
            "BACK",
            145,
            310,
            selected == 2);
    }

    SDL_RenderPresent(
        renderer);
}