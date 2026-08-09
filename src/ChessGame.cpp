#include "ChessGame.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>

// ============================================================
// CONSTRUCTOR
// ============================================================

ChessGame::ChessGame()
{
    newGame();
}

// ============================================================
// NEW GAME
// ============================================================

void ChessGame::newGame()
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            board[row][col] =
                emptyPiece();
        }
    }

    // Black pieces
    board[0][0] = {'r', false, false};
    board[0][1] = {'n', false, false};
    board[0][2] = {'b', false, false};
    board[0][3] = {'q', false, false};
    board[0][4] = {'k', false, false};
    board[0][5] = {'b', false, false};
    board[0][6] = {'n', false, false};
    board[0][7] = {'r', false, false};

    for (int col = 0; col < 8; col++)
    {
        board[1][col] =
            {'p', false, false};
    }

    // White pieces
    board[7][0] = {'r', true, false};
    board[7][1] = {'n', true, false};
    board[7][2] = {'b', true, false};
    board[7][3] = {'q', true, false};
    board[7][4] = {'k', true, false};
    board[7][5] = {'b', true, false};
    board[7][6] = {'n', true, false};
    board[7][7] = {'r', true, false};

    for (int col = 0; col < 8; col++)
    {
        board[6][col] =
            {'p', true, false};
    }

    whiteTurn = true;

    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();

    enPassantRow = -1;
    enPassantCol = -1;

    halfMoveClock = 0;

    fullMoveNumber = 1;

    gameOver = false;
    checkMate = false;
    staleMate = false;
    drawGame = false;

    promotionPending = false;

    promotionRow = -1;
    promotionCol = -1;

    promotionChoice = 0;

    positionHistory.clear();
    moveHistory.clear();

    recordCurrentPosition();
}

// ============================================================
// PUBLIC GETTERS
// ============================================================

const Piece& ChessGame::getPiece(
    int row,
    int col) const
{
    return board[row][col];
}

bool ChessGame::isWhiteTurn() const
{
    return whiteTurn;
}

bool ChessGame::isGameOver() const
{
    return gameOver;
}

bool ChessGame::isCheckmate() const
{
    return checkMate;
}

bool ChessGame::isDraw() const
{
    return drawGame;
}

bool ChessGame::isStalemate() const
{
    return staleMate;
}

bool ChessGame::isPromotionPending() const
{
    return promotionPending;
}

int ChessGame::getPromotionChoice() const
{
    return promotionChoice;
}

int ChessGame::getPromotionRow() const
{
    return promotionRow;
}

int ChessGame::getPromotionCol() const
{
    return promotionCol;
}

const std::vector<Move>&
ChessGame::getLegalMoves() const
{
    return legalMoves;
}

bool ChessGame::hasSelectedPiece() const
{
    return pieceSelected;
}

int ChessGame::getSelectedRow() const
{
    return selectedRow;
}

int ChessGame::getSelectedCol() const
{
    return selectedCol;
}

const std::vector<std::string>&
ChessGame::getMoveHistory() const
{
    return moveHistory;
}

int ChessGame::getMoveCount() const
{
    return static_cast<int>(
        moveHistory.size());
}

// ============================================================
// BASIC UTILITIES
// ============================================================

bool ChessGame::insideBoard(
    int row,
    int col) const
{
    return
        row >= 0 &&
        row < 8 &&
        col >= 0 &&
        col < 8;
}

Piece ChessGame::emptyPiece() const
{
    return {' ', false, false};
}

bool ChessGame::isEmpty(
    int row,
    int col) const
{
    return
        board[row][col].type == ' ';
}

bool ChessGame::isEnemyPiece(
    int row,
    int col,
    bool white) const
{
    if (!insideBoard(row, col))
    {
        return false;
    }

    if (isEmpty(row, col))
    {
        return false;
    }

    return
        board[row][col].white != white;
}

bool ChessGame::isCapturableEnemy(
    int row,
    int col,
    bool white) const
{
    if (!isEnemyPiece(
            row,
            col,
            white))
    {
        return false;
    }

    return
        board[row][col].type != 'k';
}

// ============================================================
// KING SEARCH
// ============================================================

bool ChessGame::findKing(
    bool white,
    int& kingRow,
    int& kingCol) const
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            const Piece& piece =
                board[row][col];

            if (
                piece.type == 'k' &&
                piece.white == white)
            {
                kingRow = row;
                kingCol = col;

                return true;
            }
        }
    }

    return false;
}

// ============================================================
// ATTACK DETECTION
// ============================================================

bool ChessGame::squareAttacked(
    int targetRow,
    int targetCol,
    bool byWhite) const
{
    // Pawns
    int pawnDirection =
        byWhite ? -1 : 1;

    int pawnRow =
        targetRow -
        pawnDirection;

    for (int offset : {-1, 1})
    {
        int pawnCol =
            targetCol + offset;

        if (
            insideBoard(
                pawnRow,
                pawnCol) &&
            board[pawnRow][pawnCol].type ==
                'p' &&
            board[pawnRow][pawnCol].white ==
                byWhite)
        {
            return true;
        }
    }

    // Knights
    const int knightOffsets[8][2] = {
        {-2, -1},
        {-2, 1},
        {-1, -2},
        {-1, 2},
        {1, -2},
        {1, 2},
        {2, -1},
        {2, 1}
    };

    for (const auto& offset :
         knightOffsets)
    {
        int row =
            targetRow +
            offset[0];

        int col =
            targetCol +
            offset[1];

        if (
            insideBoard(row, col) &&
            board[row][col].type == 'n' &&
            board[row][col].white ==
                byWhite)
        {
            return true;
        }
    }

    // King
    for (int rowOffset = -1;
         rowOffset <= 1;
         rowOffset++)
    {
        for (int colOffset = -1;
             colOffset <= 1;
             colOffset++)
        {
            if (
                rowOffset == 0 &&
                colOffset == 0)
            {
                continue;
            }

            int row =
                targetRow +
                rowOffset;

            int col =
                targetCol +
                colOffset;

            if (
                insideBoard(row, col) &&
                board[row][col].type == 'k' &&
                board[row][col].white ==
                    byWhite)
            {
                return true;
            }
        }
    }

    // Bishop / Queen
    const int diagonals[4][2] = {
        {-1, -1},
        {-1, 1},
        {1, -1},
        {1, 1}
    };

    for (const auto& direction :
         diagonals)
    {
        int row =
            targetRow +
            direction[0];

        int col =
            targetCol +
            direction[1];

        while (insideBoard(row, col))
        {
            if (!isEmpty(row, col))
            {
                const Piece& piece =
                    board[row][col];

                if (
                    piece.white == byWhite &&
                    (
                        piece.type == 'b' ||
                        piece.type == 'q'
                    ))
                {
                    return true;
                }

                break;
            }

            row += direction[0];
            col += direction[1];
        }
    }

    // Rook / Queen
    const int straights[4][2] = {
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1}
    };

    for (const auto& direction :
         straights)
    {
        int row =
            targetRow +
            direction[0];

        int col =
            targetCol +
            direction[1];

        while (insideBoard(row, col))
        {
            if (!isEmpty(row, col))
            {
                const Piece& piece =
                    board[row][col];

                if (
                    piece.white == byWhite &&
                    (
                        piece.type == 'r' ||
                        piece.type == 'q'
                    ))
                {
                    return true;
                }

                break;
            }

            row += direction[0];
            col += direction[1];
        }
    }

    return false;
}

bool ChessGame::kingInCheck(
    bool white) const
{
    int kingRow = -1;
    int kingCol = -1;

    if (!findKing(
            white,
            kingRow,
            kingCol))
    {
        return false;
    }

    return squareAttacked(
        kingRow,
        kingCol,
        !white);
}

// ============================================================
// MOVE GENERATION
// ============================================================

void ChessGame::addPseudoMove(
    std::vector<Move>& moves,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    if (!insideBoard(toRow, toCol))
    {
        return;
    }

    const Piece& movingPiece =
        board[fromRow][fromCol];

    if (!isEmpty(toRow, toCol))
    {
        const Piece& target =
            board[toRow][toCol];

        if (
            target.white ==
            movingPiece.white)
        {
            return;
        }

        if (target.type == 'k')
        {
            return;
        }
    }

    Move move;

    move.fromRow = fromRow;
    move.fromCol = fromCol;

    move.toRow = toRow;
    move.toCol = toCol;

    moves.push_back(move);
}

void ChessGame::addSlidingMoves(
    std::vector<Move>& moves,
    int row,
    int col,
    int rowDirection,
    int colDirection) const
{
    const Piece& piece =
        board[row][col];

    int targetRow =
        row + rowDirection;

    int targetCol =
        col + colDirection;

    while (
        insideBoard(
            targetRow,
            targetCol))
    {
        if (isEmpty(
                targetRow,
                targetCol))
        {
            addPseudoMove(
                moves,
                row,
                col,
                targetRow,
                targetCol);
        }
        else
        {
            const Piece& target =
                board[targetRow][targetCol];

            if (
                target.white != piece.white &&
                target.type != 'k')
            {
                addPseudoMove(
                    moves,
                    row,
                    col,
                    targetRow,
                    targetCol);
            }

            break;
        }

        targetRow += rowDirection;
        targetCol += colDirection;
    }
}

std::vector<Move>
ChessGame::generatePseudoMoves(
    int row,
    int col) const
{
    std::vector<Move> moves;

    const Piece& piece =
        board[row][col];

    if (piece.type == ' ')
    {
        return moves;
    }

    // --------------------------------------------------------
    // PAWN
    // --------------------------------------------------------

    if (piece.type == 'p')
    {
        int direction =
            piece.white ? -1 : 1;

        int startRow =
            piece.white ? 6 : 1;

        int finalRow =
            piece.white ? 0 : 7;

        int oneRow =
            row + direction;

        if (
            insideBoard(oneRow, col) &&
            isEmpty(oneRow, col))
        {
            Move move;

            move.fromRow = row;
            move.fromCol = col;

            move.toRow = oneRow;
            move.toCol = col;

            move.promotion =
                oneRow == finalRow;

            moves.push_back(move);

            int twoRow =
                row +
                direction * 2;

            if (
                row == startRow &&
                insideBoard(twoRow, col) &&
                isEmpty(twoRow, col))
            {
                Move doubleMove;

                doubleMove.fromRow =
                    row;

                doubleMove.fromCol =
                    col;

                doubleMove.toRow =
                    twoRow;

                doubleMove.toCol =
                    col;

                moves.push_back(
                    doubleMove);
            }
        }

        for (int offset : {-1, 1})
        {
            int targetRow =
                row + direction;

            int targetCol =
                col + offset;

            if (!insideBoard(
                    targetRow,
                    targetCol))
            {
                continue;
            }

            if (
                isCapturableEnemy(
                    targetRow,
                    targetCol,
                    piece.white))
            {
                Move move;

                move.fromRow =
                    row;

                move.fromCol =
                    col;

                move.toRow =
                    targetRow;

                move.toCol =
                    targetCol;

                move.promotion =
                    targetRow ==
                    finalRow;

                moves.push_back(move);
            }

            // En passant
            if (
                targetRow ==
                    enPassantRow &&
                targetCol ==
                    enPassantCol)
            {
                if (
                    board[row][targetCol].type ==
                        'p' &&
                    board[row][targetCol].white !=
                        piece.white)
                {
                    Move move;

                    move.fromRow =
                        row;

                    move.fromCol =
                        col;

                    move.toRow =
                        targetRow;

                    move.toCol =
                        targetCol;

                    move.enPassant =
                        true;

                    moves.push_back(
                        move);
                }
            }
        }

        return moves;
    }

    // --------------------------------------------------------
    // KNIGHT
    // --------------------------------------------------------

    if (piece.type == 'n')
    {
        const int offsets[8][2] = {
            {-2, -1},
            {-2, 1},
            {-1, -2},
            {-1, 2},
            {1, -2},
            {1, 2},
            {2, -1},
            {2, 1}
        };

        for (const auto& offset :
             offsets)
        {
            addPseudoMove(
                moves,
                row,
                col,
                row + offset[0],
                col + offset[1]);
        }

        return moves;
    }

    // Bishop / Queen
    if (
        piece.type == 'b' ||
        piece.type == 'q')
    {
        addSlidingMoves(
            moves,
            row,
            col,
            -1,
            -1);

        addSlidingMoves(
            moves,
            row,
            col,
            -1,
            1);

        addSlidingMoves(
            moves,
            row,
            col,
            1,
            -1);

        addSlidingMoves(
            moves,
            row,
            col,
            1,
            1);
    }

    // Rook / Queen
    if (
        piece.type == 'r' ||
        piece.type == 'q')
    {
        addSlidingMoves(
            moves,
            row,
            col,
            -1,
            0);

        addSlidingMoves(
            moves,
            row,
            col,
            1,
            0);

        addSlidingMoves(
            moves,
            row,
            col,
            0,
            -1);

        addSlidingMoves(
            moves,
            row,
            col,
            0,
            1);
    }

    if (
        piece.type == 'b' ||
        piece.type == 'r' ||
        piece.type == 'q')
    {
        return moves;
    }

    // --------------------------------------------------------
    // KING
    // --------------------------------------------------------

    if (piece.type == 'k')
    {
        for (int rowOffset = -1;
             rowOffset <= 1;
             rowOffset++)
        {
            for (int colOffset = -1;
                 colOffset <= 1;
                 colOffset++)
            {
                if (
                    rowOffset == 0 &&
                    colOffset == 0)
                {
                    continue;
                }

                addPseudoMove(
                    moves,
                    row,
                    col,
                    row + rowOffset,
                    col + colOffset);
            }
        }

        if (!piece.moved)
        {
            int homeRow =
                piece.white ? 7 : 0;

            bool enemyWhite =
                !piece.white;

            if (
                row == homeRow &&
                col == 4 &&
                !squareAttacked(
                    homeRow,
                    4,
                    enemyWhite))
            {
                // Short castle
                if (
                    board[homeRow][7].type ==
                        'r' &&
                    board[homeRow][7].white ==
                        piece.white &&
                    !board[homeRow][7].moved &&
                    isEmpty(homeRow, 5) &&
                    isEmpty(homeRow, 6) &&
                    !squareAttacked(
                        homeRow,
                        5,
                        enemyWhite) &&
                    !squareAttacked(
                        homeRow,
                        6,
                        enemyWhite))
                {
                    Move move;

                    move.fromRow =
                        row;

                    move.fromCol =
                        col;

                    move.toRow =
                        homeRow;

                    move.toCol =
                        6;

                    move.castleKingSide =
                        true;

                    moves.push_back(move);
                }

                // Long castle
                if (
                    board[homeRow][0].type ==
                        'r' &&
                    board[homeRow][0].white ==
                        piece.white &&
                    !board[homeRow][0].moved &&
                    isEmpty(homeRow, 1) &&
                    isEmpty(homeRow, 2) &&
                    isEmpty(homeRow, 3) &&
                    !squareAttacked(
                        homeRow,
                        3,
                        enemyWhite) &&
                    !squareAttacked(
                        homeRow,
                        2,
                        enemyWhite))
                {
                    Move move;

                    move.fromRow =
                        row;

                    move.fromCol =
                        col;

                    move.toRow =
                        homeRow;

                    move.toCol =
                        2;

                    move.castleQueenSide =
                        true;

                    moves.push_back(move);
                }
            }
        }
    }

    return moves;
}

// ============================================================
// BOARD SIMULATION
// ============================================================

void ChessGame::copyBoard(
    Piece destination[8][8],
    const Piece source[8][8]) const
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            destination[row][col] =
                source[row][col];
        }
    }
}

void ChessGame::applyMoveToBoard(
    const Move& move,
    bool simulation)
{
    Piece movingPiece =
        board[move.fromRow][move.fromCol];

    board[move.toRow][move.toCol] =
        movingPiece;

    board[move.toRow][move.toCol].moved =
        true;

    board[move.fromRow][move.fromCol] =
        emptyPiece();

    if (move.enPassant)
    {
        board[move.fromRow][move.toCol] =
            emptyPiece();
    }

    if (move.castleKingSide)
    {
        int row =
            move.fromRow;

        board[row][5] =
            board[row][7];

        board[row][5].moved =
            true;

        board[row][7] =
            emptyPiece();
    }

    if (move.castleQueenSide)
    {
        int row =
            move.fromRow;

        board[row][3] =
            board[row][0];

        board[row][3].moved =
            true;

        board[row][0] =
            emptyPiece();
    }

    if (
        simulation &&
        move.promotion)
    {
        board[move.toRow][move.toCol].type =
            'q';
    }
}

// ============================================================
// LEGAL MOVES
// ============================================================

std::vector<Move>
ChessGame::calculateLegalMoves(
    int row,
    int col)
{
    std::vector<Move> pseudo =
        generatePseudoMoves(
            row,
            col);

    std::vector<Move> result;

    bool movingWhite =
        board[row][col].white;

    for (const Move& move : pseudo)
    {
        Piece backup[8][8];

        copyBoard(
            backup,
            board);

        applyMoveToBoard(
            move,
            true);

        bool illegal =
            kingInCheck(
                movingWhite);

        copyBoard(
            board,
            backup);

        if (!illegal)
        {
            result.push_back(
                move);
        }
    }

    return result;
}

bool ChessGame::hasAnyLegalMove(
    bool white)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            const Piece& piece =
                board[row][col];

            if (
                piece.type != ' ' &&
                piece.white == white)
            {
                if (
                    !calculateLegalMoves(
                         row,
                         col)
                         .empty())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

// ============================================================
// SELECTION
// ============================================================

bool ChessGame::selectPiece(
    int row,
    int col)
{
    if (
        gameOver ||
        promotionPending)
    {
        return false;
    }

    if (!insideBoard(row, col))
    {
        return false;
    }

    const Piece& piece =
        board[row][col];

    if (piece.type == ' ')
    {
        return false;
    }

    if (piece.white != whiteTurn)
    {
        return false;
    }

    legalMoves =
        calculateLegalMoves(
            row,
            col);

    pieceSelected = true;

    selectedRow = row;
    selectedCol = col;

    return true;
}

void ChessGame::cancelSelection()
{
    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();
}

bool ChessGame::findSelectedMove(
    int row,
    int col,
    Move& result) const
{
    for (const Move& move :
         legalMoves)
    {
        if (
            move.toRow == row &&
            move.toCol == col)
        {
            result = move;

            return true;
        }
    }

    return false;
}

std::string ChessGame::squareName(
    int row,
    int col) const
{
    char file =
        static_cast<char>(
            'a' + col);

    int rank =
        8 - row;

    std::string result;

    result += file;
    result += std::to_string(rank);

    return result;
}

char ChessGame::pieceLetter(
    char type) const
{
    switch (type)
    {
        case 'k':
            return 'K';

        case 'q':
            return 'Q';

        case 'r':
            return 'R';

        case 'b':
            return 'B';

        case 'n':
            return 'N';

        default:
            return '\0';
    }
}

std::string ChessGame::moveToSAN(
    const Move& move,
    bool wasCapture,
    bool givesCheck,
    bool givesMate,
    char promotionType) const
{
    const Piece& piece =
        board[move.toRow][move.toCol];

    std::string san;

    // Castling
    if (move.castleKingSide)
    {
        san = "O-O";
    }
    else if (move.castleQueenSide)
    {
        san = "O-O-O";
    }
    else
    {
        // Pawn
        if (piece.type == 'p')
        {
            if (wasCapture)
            {
                char sourceFile =
                    static_cast<char>(
                        'a' + move.fromCol);

                san += sourceFile;
                san += 'x';
            }

            san +=
                squareName(
                    move.toRow,
                    move.toCol);
        }

        // Other pieces
        else
        {
            char letter =
                pieceLetter(
                    piece.type);

            if (letter != '\0')
            {
                san += letter;
            }

            if (wasCapture)
            {
                san += 'x';
            }

            san +=
                squareName(
                    move.toRow,
                    move.toCol);
        }

        if (promotionType != ' ')
        {
            san += '=';

            san +=
                pieceLetter(
                    promotionType);
        }
    }

    if (givesMate)
    {
        san += '#';
    }
    else if (givesCheck)
    {
        san += '+';
    }

    return san;
}

// ============================================================
// MOVE EXECUTION
// ============================================================

MoveSound ChessGame::moveSelectedPiece(
    int row,
    int col)
{
    if (
        gameOver ||
        promotionPending)
    {
        return MoveSound::Illegal;
    }

    if (!pieceSelected)
    {
        if (selectPiece(row, col))
        {
            return MoveSound::None;
        }

        return MoveSound::Illegal;
    }

    if (
        row == selectedRow &&
        col == selectedCol)
    {
        cancelSelection();

        return MoveSound::None;
    }

    if (
        board[row][col].type != ' ' &&
        board[row][col].white ==
            whiteTurn)
    {
        selectPiece(
            row,
            col);

        return MoveSound::None;
    }

    Move move;

    if (!findSelectedMove(
            row,
            col,
            move))
    {
        return MoveSound::Illegal;
    }

    return executeMove(move);
}

MoveSound ChessGame::executeMove(
    const Move& move)
{
    Piece movingPiece =
        board[move.fromRow][move.fromCol];

    int originalFromRow =
        move.fromRow;

    int originalFromCol =
        move.fromCol;

    bool movingWhite =
        movingPiece.white;

    bool capture =
        !isEmpty(
            move.toRow,
            move.toCol) ||
        move.enPassant;

    bool pawnMove =
        movingPiece.type == 'p';

    enPassantRow = -1;
    enPassantCol = -1;

    if (
        pawnMove &&
        std::abs(
            move.toRow -
            move.fromRow) == 2)
    {
        enPassantRow =
            (
                move.fromRow +
                move.toRow
            ) / 2;

        enPassantCol =
            move.fromCol;
    }

    if (
        pawnMove ||
        capture)
    {
        halfMoveClock = 0;
    }
    else
    {
        halfMoveClock++;
    }

    applyMoveToBoard(
        move,
        false);

    cancelSelection();

    if (move.promotion)
    {
        promotionPending = true;

        promotionRow =
            move.toRow;

        promotionCol =
            move.toCol;

        promotionChoice = 0;

        pendingPromotionMove =
            move;

        pendingPromotionCapture =
            capture;

        cancelSelection();

        return MoveSound::Promotion;
    }

    whiteTurn =
        !whiteTurn;

    if (!movingWhite)
    {
        fullMoveNumber++;
    }

    recordCurrentPosition();

    MoveSound stateSound =
        evaluateGameState();

    bool givesCheck =
        kingInCheck(
            whiteTurn);

    bool givesMate =
        checkMate;

    std::string san =
        moveToSAN(
            move,
            capture,
            givesCheck,
            givesMate);

    moveHistory.push_back(
        san);

    if (stateSound != MoveSound::None)
    {
        return stateSound;
    }

    if (
        move.castleKingSide ||
        move.castleQueenSide)
    {
        return MoveSound::Castle;
    }

    if (capture)
    {
        return MoveSound::Capture;
    }

    return movingWhite
        ? MoveSound::MoveWhite
        : MoveSound::MoveBlack;
}

// ============================================================
// PROMOTION
// ============================================================

void ChessGame::changePromotionChoice(
    int direction)
{
    if (!promotionPending)
    {
        return;
    }

    promotionChoice += direction;

    if (promotionChoice < 0)
    {
        promotionChoice = 3;
    }

    if (promotionChoice > 3)
    {
        promotionChoice = 0;
    }
}

MoveSound ChessGame::confirmPromotion()
{
    if (!promotionPending)
    {
        return MoveSound::None;
    }

    char promotedType =
        promotionPieces[
            promotionChoice];

    board[promotionRow][promotionCol].type =
        promotionPieces[promotionChoice];

    board[promotionRow][promotionCol].moved =
        true;

    promotionPending = false;

    bool promotedWhite =
        board[promotionRow]
            [promotionCol]
            .white;

    whiteTurn =
        !whiteTurn;

    if (!promotedWhite)
    {
        fullMoveNumber++;
    }

    recordCurrentPosition();

    MoveSound stateSound =
        evaluateGameState();

    bool givesCheck =
        kingInCheck(
            whiteTurn);

    bool givesMate =
        checkMate;

    std::string san =
        moveToSAN(
            pendingPromotionMove,
            pendingPromotionCapture,
            givesCheck,
            givesMate,
            promotedType);

    moveHistory.push_back(
        san);

    if (stateSound != MoveSound::None)
    {
        return stateSound;
    }

    return MoveSound::Promotion;
}

// ============================================================
// MATERIAL
// ============================================================

bool ChessGame::insufficientMaterial() const
{
    int queens = 0;
    int rooks = 0;
    int pawns = 0;

    int bishops = 0;
    int knights = 0;

    std::vector<int> bishopColors;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            const Piece& piece =
                board[row][col];

            switch (piece.type)
            {
                case 'q':
                    queens++;
                    break;

                case 'r':
                    rooks++;
                    break;

                case 'p':
                    pawns++;
                    break;

                case 'b':
                    bishops++;

                    bishopColors.push_back(
                        (row + col) % 2);
                    break;

                case 'n':
                    knights++;
                    break;
            }
        }
    }

    if (
        queens > 0 ||
        rooks > 0 ||
        pawns > 0)
    {
        return false;
    }

    if (
        bishops == 0 &&
        knights == 0)
    {
        return true;
    }

    if (
        bishops == 1 &&
        knights == 0)
    {
        return true;
    }

    if (
        bishops == 0 &&
        knights == 1)
    {
        return true;
    }

    if (
        knights == 0 &&
        bishops > 0)
    {
        bool sameColor = true;

        for (size_t i = 1;
             i < bishopColors.size();
             i++)
        {
            if (
                bishopColors[i] !=
                bishopColors[0])
            {
                sameColor = false;
                break;
            }
        }

        if (sameColor)
        {
            return true;
        }
    }

    return false;
}

std::string ChessGame::getFEN() const
{
    std::ostringstream fen;

    // --------------------------------------------------------
    // BOARD
    // --------------------------------------------------------

    for (int row = 0;
         row < 8;
         row++)
    {
        int emptyCount = 0;

        for (int col = 0;
             col < 8;
             col++)
        {
            const Piece& piece =
                board[row][col];

            if (piece.type == ' ')
            {
                emptyCount++;

                continue;
            }

            if (emptyCount > 0)
            {
                fen << emptyCount;

                emptyCount = 0;
            }

            char symbol =
                piece.type;

            if (piece.white)
            {
                symbol =
                    static_cast<char>(
                        std::toupper(
                            static_cast<unsigned char>(
                                symbol)));
            }

            fen << symbol;
        }

        if (emptyCount > 0)
        {
            fen << emptyCount;
        }

        if (row != 7)
        {
            fen << '/';
        }
    }

    // --------------------------------------------------------
    // SIDE TO MOVE
    // --------------------------------------------------------

    fen
        << ' '
        << (whiteTurn
                ? 'w'
                : 'b');

    // --------------------------------------------------------
    // CASTLING
    // --------------------------------------------------------

    fen
        << ' '
        << getCastlingRights();

    // --------------------------------------------------------
    // EN PASSANT
    // --------------------------------------------------------

    fen << ' ';

    if (
        enPassantRow >= 0 &&
        enPassantCol >= 0)
    {
        char file =
            static_cast<char>(
                'a' +
                enPassantCol);

        int rank =
            8 -
            enPassantRow;

        fen
            << file
            << rank;
    }
    else
    {
        fen << '-';
    }

    // --------------------------------------------------------
    // CLOCKS
    // --------------------------------------------------------

    fen
        << ' '
        << halfMoveClock
        << ' '
        << fullMoveNumber;

    return fen.str();
}

// ============================================================
// REPETITION
// ============================================================

std::string
ChessGame::getCastlingRights() const
{
    std::string rights;

    if (
        board[7][4].type == 'k' &&
        board[7][4].white &&
        !board[7][4].moved)
    {
        if (
            board[7][7].type == 'r' &&
            board[7][7].white &&
            !board[7][7].moved)
        {
            rights += 'K';
        }

        if (
            board[7][0].type == 'r' &&
            board[7][0].white &&
            !board[7][0].moved)
        {
            rights += 'Q';
        }
    }

    if (
        board[0][4].type == 'k' &&
        !board[0][4].white &&
        !board[0][4].moved)
    {
        if (
            board[0][7].type == 'r' &&
            !board[0][7].white &&
            !board[0][7].moved)
        {
            rights += 'k';
        }

        if (
            board[0][0].type == 'r' &&
            !board[0][0].white &&
            !board[0][0].moved)
        {
            rights += 'q';
        }
    }

    if (rights.empty())
    {
        return "-";
    }

    return rights;
}

std::string
ChessGame::getPositionKey() const
{
    std::ostringstream stream;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            const Piece& piece =
                board[row][col];

            if (piece.type == ' ')
            {
                stream << '.';
            }
            else
            {
                char symbol =
                    piece.type;

                if (piece.white)
                {
                    symbol =
                        static_cast<char>(
                            std::toupper(
                                static_cast<unsigned char>(
                                    symbol)));
                }

                stream << symbol;
            }
        }
    }

    stream
        << '|'
        << (whiteTurn ? 'w' : 'b')
        << '|'
        << getCastlingRights()
        << '|';

    if (
        enPassantRow >= 0 &&
        enPassantCol >= 0)
    {
        stream
            << enPassantRow
            << ','
            << enPassantCol;
    }
    else
    {
        stream << '-';
    }

    return stream.str();
}

void ChessGame::recordCurrentPosition()
{
    positionHistory[
        getPositionKey()]++;
}

bool ChessGame::threefoldRepetition() const
{
    auto it =
        positionHistory.find(
            getPositionKey());

    if (
        it ==
        positionHistory.end())
    {
        return false;
    }

    return it->second >= 3;
}

// ============================================================
// GAME STATE
// ============================================================

MoveSound ChessGame::evaluateGameState()
{
    bool check =
        kingInCheck(
            whiteTurn);

    bool anyMove =
        hasAnyLegalMove(
            whiteTurn);

    if (!anyMove)
    {
        gameOver = true;

        if (check)
        {
            checkMate = true;

            return
                MoveSound::Checkmate;
        }

        staleMate = true;
        drawGame = true;

        return MoveSound::Draw;
    }

    if (halfMoveClock >= 100)
    {
        gameOver = true;
        drawGame = true;

        return MoveSound::Draw;
    }

    if (insufficientMaterial())
    {
        gameOver = true;
        drawGame = true;

        return MoveSound::Draw;
    }

    if (threefoldRepetition())
    {
        gameOver = true;
        drawGame = true;

        return MoveSound::Draw;
    }

    if (check)
    {
        return MoveSound::Check;
    }

    return MoveSound::None;
}

MoveSound ChessGame::makeUCIMove(
    const std::string& uciMove)
{
    if (
        gameOver ||
        promotionPending)
    {
        return MoveSound::Illegal;
    }

    if (
        uciMove.length() < 4)
    {
        return MoveSound::Illegal;
    }

    // --------------------------------------------------------
    // CONVERT UCI COORDINATES
    //
    // e2e4:
    // from = e2
    // to   = e4
    // --------------------------------------------------------

    int fromCol =
        uciMove[0] - 'a';

    int fromRank =
        uciMove[1] - '0';

    int toCol =
        uciMove[2] - 'a';

    int toRank =
        uciMove[3] - '0';

    int fromRow =
        8 - fromRank;

    int toRow =
        8 - toRank;

    if (
        !insideBoard(
            fromRow,
            fromCol) ||
        !insideBoard(
            toRow,
            toCol))
    {
        return MoveSound::Illegal;
    }

    const Piece& piece =
        board[fromRow][fromCol];

    if (
        piece.type == ' ' ||
        piece.white != whiteTurn)
    {
        return MoveSound::Illegal;
    }

    std::vector<Move> moves =
        calculateLegalMoves(
            fromRow,
            fromCol);

    Move selectedMove;

    bool found = false;

    for (const Move& move :
         moves)
    {
        if (
            move.toRow == toRow &&
            move.toCol == toCol)
        {
            selectedMove = move;

            found = true;

            break;
        }
    }

    if (!found)
    {
        return MoveSound::Illegal;
    }

    MoveSound originalSound =
        executeMove(
            selectedMove);

    // --------------------------------------------------------
    // STOCKFISH PROMOTION
    //
    // Example:
    // e7e8q
    // --------------------------------------------------------

    if (
        selectedMove.promotion &&
        promotionPending)
    {
        char promotionType = 'q';

        if (
            uciMove.length() >= 5)
        {
            promotionType =
                static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(
                            uciMove[4])));
        }

        switch (promotionType)
        {
            case 'q':
                promotionChoice = 0;
                break;

            case 'r':
                promotionChoice = 1;
                break;

            case 'b':
                promotionChoice = 2;
                break;

            case 'n':
                promotionChoice = 3;
                break;

            default:
                promotionChoice = 0;
                break;
        }

        MoveSound promotionSound =
            confirmPromotion();

        if (
            promotionSound ==
                MoveSound::Check ||
            promotionSound ==
                MoveSound::Checkmate ||
            promotionSound ==
                MoveSound::Draw)
        {
            return promotionSound;
        }

        return MoveSound::Promotion;
    }

    return originalSound;
}