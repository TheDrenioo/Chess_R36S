#ifndef RENDERER_H
#define RENDERER_H

#include "ChessGame.h"
#include "Menu.h"
#include "ChessClock.h"

#include <SDL.h>

#include <map>
#include <string>

class Renderer
{
public:
    void renderMenu(
    const Menu& menu);

    static constexpr int WINDOW_WIDTH = 640;
    static constexpr int WINDOW_HEIGHT = 480;

    static constexpr int BOARD_SIZE = 480;
    static constexpr int TILE_SIZE = 60;

    static constexpr int BOARD_X = 80;
    static constexpr int BOARD_Y = 0;

    static constexpr int LEFT_PANEL_WIDTH = 80;

    static constexpr int RIGHT_PANEL_X =
        BOARD_X + BOARD_SIZE;

    static constexpr int RIGHT_PANEL_WIDTH =
        WINDOW_WIDTH - RIGHT_PANEL_X;

    Renderer();

    ~Renderer();

    bool initialize();

    void render(
        const ChessGame& game,
        const ChessClock& chessClock,
        int cursorRow,
        int cursorCol,
        bool boardFlipped,
        const std::string& opponentName,
        bool opponentIsWhite);

    SDL_Renderer*
    getSDLRenderer();

private:
    SDL_Window* window =
        nullptr;

    SDL_Renderer* renderer =
        nullptr;

    int screenRow(
        int logicalRow,
        bool flipped) const;

    int screenCol(
        int logicalCol,
        bool flipped) const;

    void drawOpponentName(
        const std::string& opponentName,
        bool opponentIsWhite);

    void drawMenuText(
        const std::string& text,
        int x,
        int y,
        int scale);

    void drawCharacter(
        char character,
        int x,
        int y,
        int scale);

    void drawMenuItem(
        const std::string& text,
        int x,
        int y,
        bool selected);

    void drawClocks(
        const ChessGame& game,
        const ChessClock& chessClock);

    void drawCapturedPieces(
        const ChessGame& game);

    std::string formatClock(
        int seconds) const;

    void drawMoveHistory(
        const ChessGame& game);

    void drawSmallText(
        const std::string& text,
        int x,
        int y,
        int scale);

    std::map<
        std::string,
        SDL_Texture*>
        textures;

    bool loadTextures();

    SDL_Texture* loadTexture(
        const std::string& path);

    void destroyTextures();

    std::string getPieceKey(
        const Piece& piece) const;

    void drawBoard(
        bool flipped);

    void drawPieces(
        const ChessGame& game,
        bool flipped);

    void drawSelectedSquare(
        const ChessGame& game,
        bool flipped);

    void drawLegalMoves(
        const ChessGame& game,
        bool flipped);

    void drawCursor(
        int row,
        int col,
        bool flipped);

    void drawCheckIndicator(
        const ChessGame& game,
        bool flipped);

    void drawCoordinates(
        bool flipped);

    void drawLastMoveHighlight(
        const ChessGame& game,
        bool flipped);

    void drawPanels(
        const ChessGame& game);

    void drawPromotionMenu(
        const ChessGame& game);

    void drawCircle(
        int centerX,
        int centerY,
        int radius);

    void drawLine(
        int x1,
        int y1,
        int x2,
        int y2);

    void drawLetter(
        char letter,
        int x,
        int y);

    void drawNumber(
        int number,
        int x,
        int y);
};

#endif