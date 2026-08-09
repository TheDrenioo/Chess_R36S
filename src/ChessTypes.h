#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

struct Piece
{
    char type = ' ';
    bool white = false;
    bool moved = false;
};

struct Move
{
    int fromRow = 0;
    int fromCol = 0;

    int toRow = 0;
    int toCol = 0;

    bool enPassant = false;
    bool castleKingSide = false;
    bool castleQueenSide = false;
    bool promotion = false;
};

enum class MoveSound
{
    None,
    MoveWhite,
    MoveBlack,
    Capture,
    Castle,
    Check,
    Illegal,
    Promotion,
    Checkmate,
    Draw
};

#endif