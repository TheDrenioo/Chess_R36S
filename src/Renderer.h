#ifndef RENDERER_H
#define RENDERER_H

#include "ChessGame.h"

#include <SDL.h>

#include <map>
#include <string>

class Renderer
{
public:
    static constexpr int WINDOW_WIDTH = 640;
    static constexpr int WINDOW_HEIGHT = 480;

    static constexpr int BOARD_SIZE = 480;
    static constexpr int TILE_SIZE = 60;

    static constexpr int BOARD_X = 80;
    static constexpr int BOARD_Y = 0;

    Renderer();

    ~Renderer();

    bool initialize();

    void render(
        const ChessGame& game,
        int cursorRow,
        int cursorCol);

    SDL_Renderer*
    getSDLRenderer();

private:
    SDL_Window* window =
        nullptr;

    SDL_Renderer* renderer =
        nullptr;

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

    void drawBoard();

    void drawPieces(
        const ChessGame& game);

    void drawSelectedSquare(
        const ChessGame& game);

    void drawLegalMoves(
        const ChessGame& game);

    void drawCursor(
        int row,
        int col);

    void drawCheckIndicator(
        const ChessGame& game);

    void drawCoordinates();

    void drawSidePanel(
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