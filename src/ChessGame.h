#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "ChessTypes.h"

#include <string>
#include <unordered_map>
#include <vector>


class ChessGame
{
public:
    ChessGame();

    void newGame();

    bool hasLastMove() const;

    int getLastMoveFromRow() const;
    int getLastMoveFromCol() const;

    int getLastMoveToRow() const;
    int getLastMoveToCol() const;
        
    std::string getFEN() const;

    MoveSound makeUCIMove(
        const std::string& uciMove);

    const std::vector<std::string>&
    getMoveHistory() const;

    int getMoveCount() const;

    const Piece& getPiece(
        int row,
        int col) const;

    bool isWhiteTurn() const;

    bool isGameOver() const;

    bool isCheckmate() const;

    bool isDraw() const;

    bool isStalemate() const;

    bool isPromotionPending() const;

    int getPromotionChoice() const;

    int getPromotionRow() const;

    int getPromotionCol() const;

    const std::vector<Move>&
    getLegalMoves() const;

    const std::vector<Piece>&
    getCapturedByWhite() const;

    const std::vector<Piece>&
    getCapturedByBlack() const;

    int getCapturedPointsByWhite() const;

    int getCapturedPointsByBlack() const;

    void declareTimeout(
        bool whiteLost);

    bool hasSelectedPiece() const;

    int getSelectedRow() const;

    const std::string&
    getLastMoveUCI() const;

    int getSelectedCol() const;

    bool kingInCheck(
        bool white) const;

    bool selectPiece(
        int row,
        int col);

    MoveSound moveSelectedPiece(
        int row,
        int col);

    void cancelSelection();

    void changePromotionChoice(
        int direction);

    MoveSound confirmPromotion();

private:
    Piece board[8][8];

    bool whiteTurn = true;

    bool pieceSelected = false;

    std::string lastMoveUCI;

    int selectedRow = -1;
    int selectedCol = -1;

    int lastMoveFromRow = -1;
    int lastMoveFromCol = -1;

    int lastMoveToRow = -1;
    int lastMoveToCol = -1;

    std::vector<Move> legalMoves;

    std::vector<std::string> moveHistory;

    std::vector<Piece>
    capturedByWhite;

    std::vector<Piece>
        capturedByBlack;

    bool timeoutGame = false;

    bool whiteLostOnTime = false;

    int pieceValue(
        char type) const;

    Move pendingPromotionMove;

    bool pendingPromotionCapture =
        false;

    int enPassantRow = -1;
    int enPassantCol = -1;

    int halfMoveClock = 0;

    int fullMoveNumber = 1;

    bool gameOver = false;
    bool checkMate = false;
    bool staleMate = false;
    bool drawGame = false;

    bool promotionPending = false;

    int promotionRow = -1;
    int promotionCol = -1;

    int promotionChoice = 0;

    std::string moveToSAN(
        const Move& move,
        bool wasCapture,
        bool givesCheck,
        bool givesMate,
        char promotionType = ' ') const;

    std::string squareName(
        int row,
        int col) const;

    char pieceLetter(
        char type) const;

    const char promotionPieces[4] = {
        'q',
        'r',
        'b',
        'n'
    };

    std::unordered_map<
        std::string,
        int>
        positionHistory;

    bool insideBoard(
        int row,
        int col) const;

    bool isEmpty(
        int row,
        int col) const;

    bool isEnemyPiece(
        int row,
        int col,
        bool white) const;

    bool isCapturableEnemy(
        int row,
        int col,
        bool white) const;

    Piece emptyPiece() const;

    bool findKing(
        bool white,
        int& kingRow,
        int& kingCol) const;

    bool squareAttacked(
        int targetRow,
        int targetCol,
        bool byWhite) const;

    void addPseudoMove(
        std::vector<Move>& moves,
        int fromRow,
        int fromCol,
        int toRow,
        int toCol) const;

    void addSlidingMoves(
        std::vector<Move>& moves,
        int row,
        int col,
        int rowDirection,
        int colDirection) const;

    std::vector<Move>
    generatePseudoMoves(
        int row,
        int col) const;

    std::vector<Move>
    calculateLegalMoves(
        int row,
        int col);

    bool hasAnyLegalMove(
        bool white);

    void copyBoard(
        Piece destination[8][8],
        const Piece source[8][8]) const;

    void applyMoveToBoard(
        const Move& move,
        bool simulation);

    bool findSelectedMove(
        int row,
        int col,
        Move& result) const;

    MoveSound executeMove(
        const Move& move);

    bool insufficientMaterial() const;

    std::string getCastlingRights() const;

    std::string getPositionKey() const;

    void recordCurrentPosition();

    bool threefoldRepetition() const;

    MoveSound evaluateGameState();
};

#endif