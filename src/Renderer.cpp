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

    int Renderer::screenRow(
        int logicalRow,
        bool flipped) const
    {
        if (flipped)
        {
            return 7 - logicalRow;
        }

        return logicalRow;
    }

    int Renderer::screenCol(
        int logicalCol,
        bool flipped) const
    {
        if (flipped)
        {
            return 7 - logicalCol;
        }

        return logicalCol;
    }

// ============================================================
// BOARD
// ============================================================

void Renderer::drawBoard(
    bool flipped)
{
    for (int logicalRow = 0;
         logicalRow < 8;
         logicalRow++)
    {
        for (int logicalCol = 0;
             logicalCol < 8;
             logicalCol++)
        {
            bool light =
                (logicalRow +
                 logicalCol) %
                    2 ==
                0;

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

            int row =
                screenRow(
                    logicalRow,
                    flipped);

            int col =
                screenCol(
                    logicalCol,
                    flipped);

            SDL_Rect square = {
                BOARD_X +
                    col *
                        TILE_SIZE,

                BOARD_Y +
                    row *
                        TILE_SIZE,

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
    const ChessGame& game,
    bool flipped)
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

            int screenR =
                screenRow(
                    row,
                    flipped);

            int screenC =
                screenCol(
                    col,
                    flipped);

            SDL_Rect destination = {
                BOARD_X +
                    screenC *
                        TILE_SIZE +
                    padding,

                BOARD_Y +
                    screenR *
                        TILE_SIZE +
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

void Renderer::drawLastMoveHighlight(
    const ChessGame& game,
    bool flipped)
{
    if (!game.hasLastMove())
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    // Chess.com-like yellow overlay
    SDL_SetRenderDrawColor(
        renderer,
        246,
        246,
        105,
        135);


    int fromRow =
        screenRow(
            game.getLastMoveFromRow(),
            flipped);

    int fromCol =
        screenCol(
            game.getLastMoveFromCol(),
            flipped);

    int toRow =
        screenRow(
            game.getLastMoveToRow(),
            flipped);

    int toCol =
        screenCol(
            game.getLastMoveToCol(),
            flipped);

    // Origin square
    SDL_Rect fromSquare = {
        BOARD_X +
            fromCol * TILE_SIZE,

        BOARD_Y +
            fromRow * TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_RenderFillRect(
        renderer,
        &fromSquare);

    // Destination square
    SDL_Rect toSquare = {
        BOARD_X +
            toCol * TILE_SIZE,

        BOARD_Y +
            toRow * TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_RenderFillRect(
        renderer,
        &toSquare);

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

void Renderer::drawSelectedSquare(
    const ChessGame& game,
    bool flipped)
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

    int row =
        screenRow(
            game.getSelectedRow(),
            flipped);

    int col =
        screenCol(
            game.getSelectedCol(),
            flipped);

    SDL_Rect rect = {
        BOARD_X +
            col * TILE_SIZE,

        BOARD_Y +
            row * TILE_SIZE,

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

    // ========================================================
    // TITLE
    // ========================================================

    SDL_SetRenderDrawColor(
        renderer,
        190,
        190,
        190,
        255);

    drawSmallText(
        "MOVES",
        RIGHT_PANEL_X + 8,
        12,
        1);

    SDL_SetRenderDrawColor(
        renderer,
        75,
        75,
        75,
        255);

    SDL_Rect separator = {
        RIGHT_PANEL_X + 5,
        28,
        RIGHT_PANEL_WIDTH - 10,
        1
    };

    SDL_RenderFillRect(
        renderer,
        &separator);

    if (history.empty())
    {
        return;
    }

    // ========================================================
    // MOVE HISTORY
    // ========================================================

    constexpr int maxVisibleHalfMoves = 16;

    int total =
        static_cast<int>(
            history.size());

    int start =
        std::max(
            0,
            total -
                maxVisibleHalfMoves);

    // Keep the beginning on a white move.
    if (start % 2 != 0)
    {
        start--;
    }

    int y = 42;

    for (int i = start;
         i < total;
         i += 2)
    {
        int moveNumber =
            i / 2 + 1;

        // ----------------------------------------------------
        // MOVE NUMBER
        // ----------------------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            130,
            130,
            130,
            255);

        std::string numberText =
            std::to_string(
                moveNumber);

        numberText += ".";

        drawSmallText(
            numberText,
            RIGHT_PANEL_X + 4,
            y,
            1);

        // ----------------------------------------------------
        // WHITE MOVE
        // ----------------------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            225,
            225,
            225,
            255);

        drawSmallText(
            history[i],
            RIGHT_PANEL_X + 25,
            y,
            1);

        // ----------------------------------------------------
        // BLACK MOVE
        // ----------------------------------------------------

        if (i + 1 < total)
        {
            SDL_SetRenderDrawColor(
                renderer,
                175,
                175,
                175,
                255);

            drawSmallText(
                history[i + 1],
                RIGHT_PANEL_X + 25,
                y + 18,
                1);
        }

        y += 42;

        if (
            y >
            WINDOW_HEIGHT - 30)
        {
            break;
        }
    }
}

void Renderer::drawLegalMoves(
    const ChessGame& game,
    bool flipped)
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    for (const Move& move :
         game.getLegalMoves())
    {
        int screenR =
            screenRow(
                move.toRow,
                flipped);

        int screenC =
            screenCol(
                move.toCol,
                flipped);

        int centerX =
            BOARD_X +
            screenC * TILE_SIZE +
            TILE_SIZE / 2;

        int centerY =
            BOARD_Y +
            screenR * TILE_SIZE +
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
                    screenC * TILE_SIZE +
                    4,

                BOARD_Y +
                    screenR * TILE_SIZE +
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
    int col,
    bool flipped)
{
    int visualRow =
        screenRow(
            row,
            flipped);

    int visualCol =
        screenCol(
            col,
            flipped);

    SDL_Rect rect = {
        BOARD_X +
            visualCol *
                TILE_SIZE,

        BOARD_Y +
            visualRow *
                TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_SetRenderDrawColor(
        renderer,
        255,
        215,
        0,
        255);

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
    const ChessGame& game,
    bool flipped)
{
    bool checkedWhite;

    // Determine which king is in check.
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

    // --------------------------------------------------------
    // Find king using LOGICAL board coordinates
    // --------------------------------------------------------

    int logicalKingRow = -1;
    int logicalKingCol = -1;

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
                logicalKingRow = row;
                logicalKingCol = col;

                break;
            }
        }

        if (logicalKingRow >= 0)
        {
            break;
        }
    }

    if (
        logicalKingRow < 0 ||
        logicalKingCol < 0)
    {
        return;
    }

    // --------------------------------------------------------
    // Convert logical position to VISUAL position
    // --------------------------------------------------------

    int visualKingRow =
        screenRow(
            logicalKingRow,
            flipped);

    int visualKingCol =
        screenCol(
            logicalKingCol,
            flipped);

    // --------------------------------------------------------
    // Draw red check square
    // --------------------------------------------------------

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        renderer,
        220,
        40,
        40,
        120);

    SDL_Rect checkSquare = {
        BOARD_X +
            visualKingCol *
                TILE_SIZE,

        BOARD_Y +
            visualKingRow *
                TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_RenderFillRect(
        renderer,
        &checkSquare);

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

// ============================================================
// SIDE PANEL
// ============================================================

void Renderer::drawPanels(
    const ChessGame& game)
{
    // ========================================================
    // LEFT PANEL
    // ========================================================

    SDL_SetRenderDrawColor(
        renderer,
        28,
        28,
        28,
        255);

    SDL_Rect leftPanel = {
        0,
        0,
        LEFT_PANEL_WIDTH,
        WINDOW_HEIGHT
    };

    SDL_RenderFillRect(
        renderer,
        &leftPanel);

    // ========================================================
    // RIGHT PANEL
    // ========================================================

    SDL_SetRenderDrawColor(
        renderer,
        28,
        28,
        28,
        255);

    SDL_Rect rightPanel = {
        RIGHT_PANEL_X,
        0,
        RIGHT_PANEL_WIDTH,
        WINDOW_HEIGHT
    };

    SDL_RenderFillRect(
        renderer,
        &rightPanel);

    // ========================================================
    // GAME OVER INDICATOR
    // ========================================================

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
            205,
            44,
            20
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

void Renderer::drawCoordinates(
    bool flipped)
{
    for (int visualRow = 0;
         visualRow < 8;
         visualRow++)
    {
        int logicalRow =
            flipped
                ? 7 - visualRow
                : visualRow;

        int rank =
            8 -
            logicalRow;

        int logicalCol =
            flipped
                ? 7
                : 0;

        bool light =
            (
                logicalRow +
                logicalCol
            ) %
                2 ==
            0;

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
            BOARD_Y +
                visualRow *
                    TILE_SIZE +
                4);
    }

    for (int visualCol = 0;
         visualCol < 8;
         visualCol++)
    {
        int logicalCol =
            flipped
                ? 7 - visualCol
                : visualCol;

        char file =
            static_cast<char>(
                'a' +
                logicalCol);

        int logicalRow =
            flipped
                ? 0
                : 7;

        bool light =
            (
                logicalRow +
                logicalCol
            ) %
                2 ==
            0;

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
                visualCol *
                    TILE_SIZE +
                TILE_SIZE -
                11,

            BOARD_Y +
                BOARD_SIZE -
                12);
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

std::string Renderer::formatClock(
    int seconds) const
{
    int minutes =
        seconds / 60;

    int remainingSeconds =
        seconds % 60;

    std::string result;

    if (minutes < 10)
    {
        result += '0';
    }

    result +=
        std::to_string(
            minutes);

    result += ':';

    if (remainingSeconds < 10)
    {
        result += '0';
    }

    result +=
        std::to_string(
            remainingSeconds);

    return result;
}

void Renderer::drawPlayerNames(
    const std::string& playerName,
    const std::string& opponentName)
{
    constexpr std::size_t maxChars =
        11;

    std::string topName =
        opponentName;

    std::string bottomName =
        playerName;

    if (
        topName.length() >
        maxChars)
    {
        topName =
            topName.substr(
                0,
                maxChars);
    }

    if (
        bottomName.length() >
        maxChars)
    {
        bottomName =
            bottomName.substr(
                0,
                maxChars);
    }

    SDL_SetRenderDrawColor(
        renderer,
        215,
        215,
        215,
        255);

    // ========================================================
    // OPPONENT
    // ========================================================

    drawSmallText(
        topName,
        5,
        57,
        1);

    // ========================================================
    // LOCAL PLAYER
    // ========================================================

    drawSmallText(
        bottomName,
        5,
        419,
        1);
}


void Renderer::render(
    const ChessGame& game,
    const ChessClock& chessClock,
    int cursorRow,
    int cursorCol,
    bool boardFlipped,
    const std::string& playerName,
    const std::string& opponentName)
{
    SDL_SetRenderDrawColor(
        renderer,
        35,
        35,
        35,
        255);

    SDL_RenderClear(
        renderer);

    // Left + right background panels
    drawPanels(
        game);

    // LEFT PANEL
    drawClocks(
        chessClock,
        boardFlipped);

    drawPlayerNames(
        playerName,
        opponentName);

    drawCapturedPieces(
        game,
        boardFlipped);

    // RIGHT PANEL
    drawMoveHistory(
        game);

    // BOARD

    drawBoard(
        boardFlipped);

    drawLastMoveHighlight(
            game,
            boardFlipped);

    drawSelectedSquare(
            game,
            boardFlipped);

    drawLegalMoves(
            game,
            boardFlipped);

    drawCheckIndicator(
            game,
            boardFlipped);

    drawPieces(
            game,
            boardFlipped);

    drawCoordinates(
        boardFlipped);

    if (
        !game.isGameOver() &&
        !game.isPromotionPending())
    {
        drawCursor(
            cursorRow,
            cursorCol,
            boardFlipped);
    }

    drawPromotionMenu(
        game);

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
        drawCharacter(
            character,
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
    if (scale <= 0)
    {
        return;
    }

    character =
        static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(
                    character)));

    // 5x7 bitmap font.
    // Each string represents one row.
    const char* pattern[7] = {
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000"
    };

    switch (character)
    {
        // ====================================================
        // LETTERS
        // ====================================================

        case 'A':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10001",
                "11111",
                "10001",
                "10001",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'B':
        {
            static const char* p[7] = {
                "11110",
                "10001",
                "10001",
                "11110",
                "10001",
                "10001",
                "11110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'C':
        {
            static const char* p[7] = {
                "01111",
                "10000",
                "10000",
                "10000",
                "10000",
                "10000",
                "01111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'D':
        {
            static const char* p[7] = {
                "11110",
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "11110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'E':
        {
            static const char* p[7] = {
                "11111",
                "10000",
                "10000",
                "11110",
                "10000",
                "10000",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'F':
        {
            static const char* p[7] = {
                "11111",
                "10000",
                "10000",
                "11110",
                "10000",
                "10000",
                "10000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'G':
        {
            static const char* p[7] = {
                "01111",
                "10000",
                "10000",
                "10111",
                "10001",
                "10001",
                "01111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'H':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "10001",
                "11111",
                "10001",
                "10001",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'I':
        {
            static const char* p[7] = {
                "11111",
                "00100",
                "00100",
                "00100",
                "00100",
                "00100",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'J':
        {
            static const char* p[7] = {
                "00111",
                "00010",
                "00010",
                "00010",
                "10010",
                "10010",
                "01100"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'K':
        {
            static const char* p[7] = {
                "10001",
                "10010",
                "10100",
                "11000",
                "10100",
                "10010",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'L':
        {
            static const char* p[7] = {
                "10000",
                "10000",
                "10000",
                "10000",
                "10000",
                "10000",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'M':
        {
            static const char* p[7] = {
                "10001",
                "11011",
                "10101",
                "10101",
                "10001",
                "10001",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'N':
        {
            static const char* p[7] = {
                "10001",
                "11001",
                "10101",
                "10011",
                "10001",
                "10001",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'O':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'P':
        {
            static const char* p[7] = {
                "11110",
                "10001",
                "10001",
                "11110",
                "10000",
                "10000",
                "10000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'Q':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10001",
                "10001",
                "10101",
                "10010",
                "01101"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'R':
        {
            static const char* p[7] = {
                "11110",
                "10001",
                "10001",
                "11110",
                "10100",
                "10010",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'S':
        {
            static const char* p[7] = {
                "01111",
                "10000",
                "10000",
                "01110",
                "00001",
                "00001",
                "11110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'T':
        {
            static const char* p[7] = {
                "11111",
                "00100",
                "00100",
                "00100",
                "00100",
                "00100",
                "00100"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'U':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'V':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "01010",
                "00100"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'W':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "10001",
                "10101",
                "10101",
                "10101",
                "01010"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'X':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "01010",
                "00100",
                "01010",
                "10001",
                "10001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'Y':
        {
            static const char* p[7] = {
                "10001",
                "10001",
                "01010",
                "00100",
                "00100",
                "00100",
                "00100"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case 'Z':
        {
            static const char* p[7] = {
                "11111",
                "00001",
                "00010",
                "00100",
                "01000",
                "10000",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        // ====================================================
        // NUMBERS
        // ====================================================

        case '0':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10011",
                "10101",
                "11001",
                "10001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '1':
        {
            static const char* p[7] = {
                "00100",
                "01100",
                "00100",
                "00100",
                "00100",
                "00100",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '2':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "00001",
                "00010",
                "00100",
                "01000",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '3':
        {
            static const char* p[7] = {
                "11110",
                "00001",
                "00001",
                "01110",
                "00001",
                "00001",
                "11110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '4':
        {
            static const char* p[7] = {
                "00010",
                "00110",
                "01010",
                "10010",
                "11111",
                "00010",
                "00010"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '5':
        {
            static const char* p[7] = {
                "11111",
                "10000",
                "10000",
                "11110",
                "00001",
                "00001",
                "11110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '6':
        {
            static const char* p[7] = {
                "01110",
                "10000",
                "10000",
                "11110",
                "10001",
                "10001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '7':
        {
            static const char* p[7] = {
                "11111",
                "00001",
                "00010",
                "00100",
                "01000",
                "01000",
                "01000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '8':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10001",
                "01110",
                "10001",
                "10001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '9':
        {
            static const char* p[7] = {
                "01110",
                "10001",
                "10001",
                "01111",
                "00001",
                "00001",
                "01110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        // ====================================================
        // SYMBOLS
        // ====================================================

        case '.':
        {
            static const char* p[7] = {
                "00000",
                "00000",
                "00000",
                "00000",
                "00000",
                "00110",
                "00110"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case ':':
        {
            static const char* p[7] = {
                "00000",
                "00110",
                "00110",
                "00000",
                "00110",
                "00110",
                "00000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '+':
        {
            static const char* p[7] = {
                "00000",
                "00100",
                "00100",
                "11111",
                "00100",
                "00100",
                "00000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '-':
        {
            static const char* p[7] = {
                "00000",
                "00000",
                "00000",
                "11111",
                "00000",
                "00000",
                "00000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '_':
        {
            static const char* p[7] = {
                "00000",
                "00000",
                "00000",
                "00000",
                "00000",
                "00000",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '=':
        {
            static const char* p[7] = {
                "00000",
                "00000",
                "11111",
                "00000",
                "11111",
                "00000",
                "00000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '#':
        {
            static const char* p[7] = {
                "01010",
                "01010",
                "11111",
                "01010",
                "11111",
                "01010",
                "01010"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '/':
        {
            static const char* p[7] = {
                "00001",
                "00010",
                "00010",
                "00100",
                "01000",
                "01000",
                "10000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '>':
        {
            static const char* p[7] = {
                "10000",
                "01000",
                "00100",
                "00010",
                "00100",
                "01000",
                "10000"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case '<':
        {
            static const char* p[7] = {
                "00001",
                "00010",
                "00100",
                "01000",
                "00100",
                "00010",
                "00001"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }

        case ' ':
            return;

        default:
        {
            // Unknown character -> small box.
            static const char* p[7] = {
                "11111",
                "10001",
                "10001",
                "10001",
                "10001",
                "10001",
                "11111"
            };

            for (int i = 0; i < 7; i++) pattern[i] = p[i];
            break;
        }
    }

    // ========================================================
    // DRAW BITMAP
    // ========================================================

    for (int row = 0;
         row < 7;
         row++)
    {
        for (int col = 0;
             col < 5;
             col++)
        {
            if (
                pattern[row][col] !=
                '1')
            {
                continue;
            }

            SDL_Rect pixel = {
                x +
                    col * scale,

                y +
                    row * scale,

                scale,
                scale
            };

            SDL_RenderFillRect(
                renderer,
                &pixel);
        }
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
    const Menu& menu,
    const LichessClient& lichess)
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
            170,
            150,
            selected == 0);

        drawMenuItem(
            "PLAY ONLINE",
            170,
            200,
            selected == 1);

        drawMenuItem(
            "TWO PLAYERS",
            170,
            250,
            selected == 2);

        drawMenuItem(
            "SETTINGS",
            170,
            300,
            selected == 3);

        drawMenuItem(
            "EXIT",
            170,
            350,
            selected == 4);
    }

    // ========================================================
    // SETTINGS
    // ========================================================

    else if (
        menu.getScreen() ==
        MenuScreen::Settings)
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

        std::string timeText =
            "TIME: ";

        switch (config.timeControl)
        {
            case TimeControl::NoClock:
                timeText += "NONE";
                break;

            case TimeControl::Bullet1:
                timeText += "1 MIN";
                break;

            case TimeControl::Blitz3:
                timeText += "3 MIN";
                break;

            case TimeControl::Blitz3Plus2:
                timeText += "3+2";
                break;

            case TimeControl::Blitz5:
                timeText += "5 MIN";
                break;

            case TimeControl::Rapid10:
                timeText += "10 MIN";
                break;
        }

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
            160,
            selected == 0);

        drawMenuItem(
            difficultyText,
            145,
            215,
            selected == 1);

        drawMenuItem(
            timeText,
            145,
            270,
            selected == 2);

        drawMenuItem(
            "BACK",
            145,
            325,
            selected == 3);
    }

    else if (
        menu.getScreen() ==
        MenuScreen::Online)
    {
        // ========================================================
        // LICHESS ONLINE SCREEN
        // ========================================================

        SDL_SetRenderDrawColor(
            renderer,
            238,
            238,
            210,
            255);

        drawMenuText(
            "LICHESS",
            225,
            155,
            5);

        // ========================================================
        // CONNECTION STATUS
        // ========================================================

        if (lichess.isConnected())
        {
            // ====================================================
            // CONNECTED
            // ====================================================

            SDL_SetRenderDrawColor(
                renderer,
                118,
                150,
                86,
                255);

            SDL_Rect connectionLight = {
                185,
                219,
                12,
                12
            };

            SDL_RenderFillRect(
                renderer,
                &connectionLight);

            SDL_SetRenderDrawColor(
                renderer,
                210,
                230,
                210,
                255);

            drawMenuText(
                "CONNECTED",
                215,
                215,
                3);

            // ====================================================
            // ACCOUNT LABEL
            // ====================================================

            SDL_SetRenderDrawColor(
                renderer,
                150,
                150,
                150,
                255);

            drawMenuText(
                "USER",
                245,
                270,
                2);

            // ====================================================
            // USERNAME
            // ====================================================

            std::string username =
                lichess.getUsername();

            constexpr std::size_t maxUsernameLength =
                16;

            if (
                username.length() >
                maxUsernameLength)
            {
                username =
                    username.substr(
                        0,
                        maxUsernameLength);
            }

            SDL_SetRenderDrawColor(
                renderer,
                238,
                238,
                210,
                255);

            drawMenuText(
                username,
                180,
                305,
                3);
        }
        else
        {
            // Red connection indicator
            SDL_SetRenderDrawColor(
                renderer,
                190,
                65,
                65,
                255);

            SDL_Rect connectionLight = {
                170,
                239,
                12,
                12
            };

            SDL_RenderFillRect(
                renderer,
                &connectionLight);

            SDL_SetRenderDrawColor(
                renderer,
                200,
                170,
                170,
                255);

            drawMenuText(
                "NOT CONNECTED",
                200,
                235,
                3);
        }

        // ========================================================
        // BACK
        // ========================================================

        SDL_SetRenderDrawColor(
            renderer,
            118,
            150,
            86,
            255);

        drawMenuText(
            "B BACK",
            245,
            380,
            2);
    }

    SDL_RenderPresent(
        renderer);
}

void Renderer::drawClocks(
    const ChessClock& chessClock,
    bool boardFlipped)
{
    if (!chessClock.isEnabled())
    {
        return;
    }

    // ========================================================
    // BOARD ORIENTATION
    //
    // Normal:
    //   Top    = Black
    //   Bottom = White
    //
    // Flipped:
    //   Top    = White
    //   Bottom = Black
    // ========================================================

    bool topIsWhite =
        boardFlipped;

    bool bottomIsWhite =
        !boardFlipped;

    int topSeconds =
        topIsWhite
            ? chessClock.getWhiteSeconds()
            : chessClock.getBlackSeconds();

    int bottomSeconds =
        bottomIsWhite
            ? chessClock.getWhiteSeconds()
            : chessClock.getBlackSeconds();

    bool topActive =
        chessClock.isWhiteActive() ==
        topIsWhite;

    bool bottomActive =
        chessClock.isWhiteActive() ==
        bottomIsWhite;

    // ========================================================
    // TOP CLOCK - OPPONENT
    // ========================================================

    if (topActive)
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
            60,
            60,
            60,
            255);
    }

    SDL_Rect topClock = {
        5,
        8,
        70,
        36
    };

    SDL_RenderFillRect(
        renderer,
        &topClock);

    SDL_SetRenderDrawColor(
        renderer,
        240,
        240,
        240,
        255);

    drawSmallText(
        formatClock(topSeconds),
        12,
        20,
        2);

    // ========================================================
    // BOTTOM CLOCK - LOCAL PLAYER
    // ========================================================

    if (bottomActive)
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
            60,
            60,
            60,
            255);
    }

    SDL_Rect bottomClock = {
        5,
        440,
        70,
        34
    };

    SDL_RenderFillRect(
        renderer,
        &bottomClock);

    SDL_SetRenderDrawColor(
        renderer,
        240,
        240,
        240,
        255);

    drawSmallText(
        formatClock(bottomSeconds),
        12,
        451,
        2);
}

void Renderer::drawCapturedPieces(
    const ChessGame& game,
    bool boardFlipped)
{
    constexpr int iconSize = 14;
    constexpr int spacing = 16;

    // ========================================================
    // MATERIAL
    // ========================================================

    int whiteScore =
        game.getCapturedPointsByWhite();

    int blackScore =
        game.getCapturedPointsByBlack();

    int difference =
        whiteScore -
        blackScore;

    // ========================================================
    // DETERMINE WHICH COLOR IS TOP / BOTTOM
    // ========================================================

    bool topIsWhite =
        boardFlipped;

    bool bottomIsWhite =
        !boardFlipped;

    // ========================================================
    // CAPTURE LISTS
    //
    // getCapturedByWhite()
    // = black pieces captured by White
    //
    // getCapturedByBlack()
    // = white pieces captured by Black
    // ========================================================

    const std::vector<Piece>& topCaptures =
        topIsWhite
            ? game.getCapturedByWhite()
            : game.getCapturedByBlack();

    const std::vector<Piece>& bottomCaptures =
        bottomIsWhite
            ? game.getCapturedByWhite()
            : game.getCapturedByBlack();

    // ========================================================
    // TOP PLAYER CAPTURES
    // ========================================================

    int index = 0;

    for (const Piece& piece :
         topCaptures)
    {
        std::string key =
            getPieceKey(piece);

        auto it =
            textures.find(key);

        if (
            it == textures.end() ||
            !it->second)
        {
            continue;
        }

        int column =
            index % 4;

        int row =
            index / 4;

        SDL_Rect destination = {
            7 +
                column *
                    spacing,

            82 +
                row *
                    spacing,

            iconSize,
            iconSize
        };

        SDL_RenderCopy(
            renderer,
            it->second,
            nullptr,
            &destination);

        index++;
    }

    // ========================================================
    // BOTTOM PLAYER CAPTURES
    // ========================================================

    index = 0;

    for (const Piece& piece :
         bottomCaptures)
    {
        std::string key =
            getPieceKey(piece);

        auto it =
            textures.find(key);

        if (
            it == textures.end() ||
            !it->second)
        {
            continue;
        }

        int column =
            index % 4;

        int row =
            index / 4;

        SDL_Rect destination = {
            7 +
                column *
                    spacing,

            334 +
                row *
                    spacing,

            iconSize,
            iconSize
        };

        SDL_RenderCopy(
            renderer,
            it->second,
            nullptr,
            &destination);

        index++;
    }

    // ========================================================
    // MATERIAL ADVANTAGE
    // ========================================================

    int topAdvantage = 0;
    int bottomAdvantage = 0;

    if (difference > 0)
    {
        // White has advantage.

        if (topIsWhite)
        {
            topAdvantage =
                difference;
        }
        else
        {
            bottomAdvantage =
                difference;
        }
    }
    else if (difference < 0)
    {
        // Black has advantage.

        int blackAdvantage =
            -difference;

        if (!topIsWhite)
        {
            topAdvantage =
                blackAdvantage;
        }
        else
        {
            bottomAdvantage =
                blackAdvantage;
        }
    }

    SDL_SetRenderDrawColor(
        renderer,
        220,
        220,
        220,
        255);

    // Top advantage
    if (topAdvantage > 0)
    {
        std::string points =
            "+" +
            std::to_string(
                topAdvantage);

        drawSmallText(
            points,
            8,
            151,
            1);
    }

    // Bottom advantage
    if (bottomAdvantage > 0)
    {
        std::string points =
            "+" +
            std::to_string(
                bottomAdvantage);

        drawSmallText(
            points,
            8,
            402,
            1);
    }
}