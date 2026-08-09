#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// CONFIGURATION
// ============================================================

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

constexpr int BOARD_SIZE = 480;
constexpr int TILE_SIZE = 60;

constexpr int BOARD_X = 80;
constexpr int BOARD_Y = 0;

// ============================================================
// STRUCTURES
// ============================================================

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

// ============================================================
// GLOBAL GAME STATE
// ============================================================

Piece board[8][8];

bool whiteTurn = true;

int cursorRow = 7;
int cursorCol = 4;

bool pieceSelected = false;

int selectedRow = -1;
int selectedCol = -1;

std::vector<Move> legalMoves;

// En passant target
int enPassantRow = -1;
int enPassantCol = -1;

// 50-move rule
int halfMoveClock = 0;

// Game state
bool gameOver = false;
bool checkMate = false;
bool staleMate = false;
bool drawGame = false;

// Promotion
bool promotionPending = false;

int promotionRow = -1;
int promotionCol = -1;

int promotionChoice = 0;

const char promotionPieces[4] = {
    'q',
    'r',
    'b',
    'n'
};

// Repetition
std::unordered_map<std::string, int> positionHistory;

// Graphics
std::map<std::string, SDL_Texture*> textures;

// Sounds
std::map<std::string, Mix_Chunk*> sounds;

// ============================================================
// BASIC UTILITIES
// ============================================================

bool insideBoard(int row, int col)
{
    return
        row >= 0 &&
        row < 8 &&
        col >= 0 &&
        col < 8;
}

Piece emptyPiece()
{
    return {' ', false, false};
}

bool isEmpty(int row, int col)
{
    return board[row][col].type == ' ';
}

bool isEnemyPiece(
    int row,
    int col,
    bool white)
{
    if (!insideBoard(row, col))
    {
        return false;
    }

    if (isEmpty(row, col))
    {
        return false;
    }

    return board[row][col].white != white;
}

bool isCapturableEnemy(
    int row,
    int col,
    bool white)
{
    if (!isEnemyPiece(row, col, white))
    {
        return false;
    }

    // A king is never captured directly.
    return board[row][col].type != 'k';
}

// ============================================================
// AUDIO
// ============================================================

Mix_Chunk* loadSound(const std::string& path)
{
    Mix_Chunk* sound =
        Mix_LoadWAV(path.c_str());

    if (!sound)
    {
        std::cerr
            << "Could not load sound: "
            << path
            << "\n"
            << Mix_GetError()
            << std::endl;
    }

    return sound;
}

void loadSounds()
{
    sounds["move-self"] =
        loadSound(
            "assets/sounds/standard/move-self.mp3");

    sounds["move-opponent"] =
        loadSound(
            "assets/sounds/standard/move-opponent.mp3");

    sounds["capture"] =
        loadSound(
            "assets/sounds/standard/capture.mp3");

    sounds["castle"] =
        loadSound(
            "assets/sounds/standard/castle.mp3");

    sounds["check"] =
        loadSound(
            "assets/sounds/standard/move-check.mp3");

    sounds["illegal"] =
        loadSound(
            "assets/sounds/standard/illegal.mp3");

    sounds["promote"] =
        loadSound(
            "assets/sounds/standard/promote.mp3");

    sounds["game-start"] =
        loadSound(
            "assets/sounds/standard/game-start.mp3");

    sounds["game-win"] =
        loadSound(
            "assets/sounds/standard/game-win-long.mp3");

    sounds["game-draw"] =
        loadSound(
            "assets/sounds/standard/game-draw.mp3");

    sounds["game-end"] =
        loadSound(
            "assets/sounds/standard/game-end.mp3");

    sounds["click"] =
        loadSound(
            "assets/sounds/standard/click.mp3");

    sounds["notify"] =
        loadSound(
            "assets/sounds/standard/notify.mp3");
}

void playSound(const std::string& name)
{
    auto it =
        sounds.find(name);

    if (it == sounds.end())
    {
        return;
    }

    if (!it->second)
    {
        return;
    }

    Mix_PlayChannel(
        -1,
        it->second,
        0);
}

void destroySounds()
{
    for (auto& item : sounds)
    {
        if (item.second)
        {
            Mix_FreeChunk(
                item.second);
        }
    }

    sounds.clear();
}

// ============================================================
// TEXTURES
// ============================================================

SDL_Texture* loadTexture(
    SDL_Renderer* renderer,
    const std::string& path)
{
    SDL_Surface* surface =
        IMG_Load(path.c_str());

    if (!surface)
    {
        std::cerr
            << "Could not load image: "
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

void loadPieceTextures(
    SDL_Renderer* renderer)
{
    textures["wk"] =
        loadTexture(
            renderer,
            "assets/pieces/wk.png");

    textures["wq"] =
        loadTexture(
            renderer,
            "assets/pieces/wq.png");

    textures["wr"] =
        loadTexture(
            renderer,
            "assets/pieces/wr.png");

    textures["wb"] =
        loadTexture(
            renderer,
            "assets/pieces/wb.png");

    textures["wn"] =
        loadTexture(
            renderer,
            "assets/pieces/wn.png");

    textures["wp"] =
        loadTexture(
            renderer,
            "assets/pieces/wp.png");

    textures["bk"] =
        loadTexture(
            renderer,
            "assets/pieces/bk.png");

    textures["bq"] =
        loadTexture(
            renderer,
            "assets/pieces/bq.png");

    textures["br"] =
        loadTexture(
            renderer,
            "assets/pieces/br.png");

    textures["bb"] =
        loadTexture(
            renderer,
            "assets/pieces/bb.png");

    textures["bn"] =
        loadTexture(
            renderer,
            "assets/pieces/bn.png");

    textures["bp"] =
        loadTexture(
            renderer,
            "assets/pieces/bp.png");
}

void destroyTextures()
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

std::string getPieceKey(
    const Piece& piece)
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

// ============================================================
// INITIAL POSITION
// ============================================================

void setupBoard()
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            board[row][col] =
                emptyPiece();
        }
    }

    // Black
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

    // White
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

    cursorRow = 7;
    cursorCol = 4;

    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();

    enPassantRow = -1;
    enPassantCol = -1;

    halfMoveClock = 0;

    gameOver = false;
    checkMate = false;
    staleMate = false;
    drawGame = false;

    promotionPending = false;

    promotionRow = -1;
    promotionCol = -1;

    promotionChoice = 0;

    positionHistory.clear();
}

// ============================================================
// KING SEARCH
// ============================================================

bool findKing(
    bool white,
    int& kingRow,
    int& kingCol)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (
                board[row][col].type == 'k' &&
                board[row][col].white == white)
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

bool squareAttacked(
    int targetRow,
    int targetCol,
    bool byWhite)
{
    // Pawns
    int pawnDirection =
        byWhite ? -1 : 1;

    int pawnRow =
        targetRow - pawnDirection;

    for (int offset : {-1, 1})
    {
        int pawnCol =
            targetCol + offset;

        if (
            insideBoard(
                pawnRow,
                pawnCol) &&
            board[pawnRow][pawnCol].type == 'p' &&
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
            targetRow + offset[0];

        int col =
            targetCol + offset[1];

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
                targetRow + rowOffset;

            int col =
                targetCol + colOffset;

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

    // Bishop / Queen diagonals
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
            targetRow + direction[0];

        int col =
            targetCol + direction[1];

        while (insideBoard(row, col))
        {
            if (!isEmpty(row, col))
            {
                if (
                    board[row][col].white ==
                        byWhite &&
                    (
                        board[row][col].type == 'b' ||
                        board[row][col].type == 'q'
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

    // Rook / Queen straight lines
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
            targetRow + direction[0];

        int col =
            targetCol + direction[1];

        while (insideBoard(row, col))
        {
            if (!isEmpty(row, col))
            {
                if (
                    board[row][col].white ==
                        byWhite &&
                    (
                        board[row][col].type == 'r' ||
                        board[row][col].type == 'q'
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

bool kingInCheck(bool white)
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

void addPseudoMove(
    std::vector<Move>& moves,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol)
{
    if (!insideBoard(toRow, toCol))
    {
        return;
    }

    Piece movingPiece =
        board[fromRow][fromCol];

    if (!isEmpty(toRow, toCol))
    {
        if (
            board[toRow][toCol].white ==
            movingPiece.white)
        {
            return;
        }

        if (board[toRow][toCol].type == 'k')
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

void addSlidingMoves(
    std::vector<Move>& moves,
    int row,
    int col,
    int rowDirection,
    int colDirection)
{
    Piece piece =
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
            if (
                board[targetRow][targetCol].white !=
                    piece.white &&
                board[targetRow][targetCol].type !=
                    'k')
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

std::vector<Move> generatePseudoMoves(
    int row,
    int col)
{
    std::vector<Move> moves;

    Piece piece =
        board[row][col];

    if (piece.type == ' ')
    {
        return moves;
    }

    // ========================================================
    // PAWN
    // ========================================================

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
                row + direction * 2;

            if (
                row == startRow &&
                insideBoard(twoRow, col) &&
                isEmpty(twoRow, col))
            {
                Move doubleMove;

                doubleMove.fromRow = row;
                doubleMove.fromCol = col;

                doubleMove.toRow = twoRow;
                doubleMove.toCol = col;

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

            // Normal capture
            if (
                isCapturableEnemy(
                    targetRow,
                    targetCol,
                    piece.white))
            {
                Move move;

                move.fromRow = row;
                move.fromCol = col;

                move.toRow = targetRow;
                move.toCol = targetCol;

                move.promotion =
                    targetRow == finalRow;

                moves.push_back(move);
            }

            // En passant
            if (
                targetRow == enPassantRow &&
                targetCol == enPassantCol)
            {
                int capturedPawnRow =
                    row;

                if (
                    insideBoard(
                        capturedPawnRow,
                        targetCol) &&
                    board[capturedPawnRow][targetCol].type ==
                        'p' &&
                    board[capturedPawnRow][targetCol].white !=
                        piece.white)
                {
                    Move move;

                    move.fromRow = row;
                    move.fromCol = col;

                    move.toRow = targetRow;
                    move.toCol = targetCol;

                    move.enPassant = true;

                    moves.push_back(move);
                }
            }
        }

        return moves;
    }

    // ========================================================
    // KNIGHT
    // ========================================================

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

    // ========================================================
    // BISHOP / QUEEN
    // ========================================================

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

    // ========================================================
    // ROOK / QUEEN
    // ========================================================

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
        piece.type == 'q' ||
        piece.type == 'r' ||
        piece.type == 'b')
    {
        return moves;
    }

    // ========================================================
    // KING
    // ========================================================

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

        // Castling
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
                // King side
                if (
                    board[homeRow][7].type == 'r' &&
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

                    move.fromRow = row;
                    move.fromCol = col;

                    move.toRow = homeRow;
                    move.toCol = 6;

                    move.castleKingSide =
                        true;

                    moves.push_back(move);
                }

                // Queen side
                if (
                    board[homeRow][0].type == 'r' &&
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

                    move.fromRow = row;
                    move.fromCol = col;

                    move.toRow = homeRow;
                    move.toCol = 2;

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
// BOARD COPYING / SIMULATION
// ============================================================

void copyBoard(
    Piece destination[8][8],
    Piece source[8][8])
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

void applyMoveToBoard(
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

    // En passant capture
    if (move.enPassant)
    {
        board[move.fromRow][move.toCol] =
            emptyPiece();
    }

    // King-side castle
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

    // Queen-side castle
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

    // During simulation use queen promotion
    if (
        simulation &&
        move.promotion)
    {
        board[move.toRow][move.toCol].type =
            'q';
    }
}

// ============================================================
// TRUE LEGAL MOVES
// ============================================================

std::vector<Move> calculateLegalMoves(
    int row,
    int col)
{
    std::vector<Move> pseudoMoves =
        generatePseudoMoves(
            row,
            col);

    std::vector<Move> result;

    bool movingWhite =
        board[row][col].white;

    for (const Move& move :
         pseudoMoves)
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

bool hasAnyLegalMove(bool white)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            if (
                board[row][col].type != ' ' &&
                board[row][col].white ==
                    white)
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
// CASTLING RIGHTS FOR REPETITION
// ============================================================

std::string getCastlingRights()
{
    std::string rights;

    // White
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

    // Black
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
        rights = "-";
    }

    return rights;
}

// ============================================================
// POSITION KEY FOR THREEFOLD REPETITION
// ============================================================

std::string getPositionKey()
{
    std::ostringstream key;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Piece piece =
                board[row][col];

            if (piece.type == ' ')
            {
                key << '.';
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

                key << symbol;
            }
        }
    }

    // Side to move
    key << '|'
        << (whiteTurn ? 'w' : 'b');

    // Castling rights
    key << '|'
        << getCastlingRights();

    // En passant target
    key << '|';

    if (
        enPassantRow >= 0 &&
        enPassantCol >= 0)
    {
        key
            << enPassantRow
            << ','
            << enPassantCol;
    }
    else
    {
        key << '-';
    }

    return key.str();
}

void recordCurrentPosition()
{
    std::string key =
        getPositionKey();

    positionHistory[key]++;
}

bool threefoldRepetition()
{
    std::string key =
        getPositionKey();

    auto it =
        positionHistory.find(key);

    if (it == positionHistory.end())
    {
        return false;
    }

    return it->second >= 3;
}

// ============================================================
// INSUFFICIENT MATERIAL
// ============================================================

bool insufficientMaterial()
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
            Piece piece =
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

    // King vs King
    if (
        bishops == 0 &&
        knights == 0)
    {
        return true;
    }

    // King + Bishop vs King
    if (
        bishops == 1 &&
        knights == 0)
    {
        return true;
    }

    // King + Knight vs King
    if (
        bishops == 0 &&
        knights == 1)
    {
        return true;
    }

    // Only bishops, all on same square color
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

// ============================================================
// GAME STATE EVALUATION
// ============================================================

void evaluateGameState()
{
    if (promotionPending)
    {
        return;
    }

    bool player =
        whiteTurn;

    bool check =
        kingInCheck(player);

    bool hasMoves =
        hasAnyLegalMove(player);

    if (!hasMoves)
    {
        gameOver = true;

        if (check)
        {
            checkMate = true;

            std::cout
                << "CHECKMATE - "
                << (player
                        ? "Black wins"
                        : "White wins")
                << std::endl;

            playSound(
                "game-win");
        }
        else
        {
            staleMate = true;
            drawGame = true;

            std::cout
                << "DRAW - Stalemate"
                << std::endl;

            playSound(
                "game-draw");
        }

        return;
    }

    if (halfMoveClock >= 100)
    {
        gameOver = true;
        drawGame = true;

        std::cout
            << "DRAW - 50 move rule"
            << std::endl;

        playSound(
            "game-draw");

        return;
    }

    if (insufficientMaterial())
    {
        gameOver = true;
        drawGame = true;

        std::cout
            << "DRAW - Insufficient material"
            << std::endl;

        playSound(
            "game-draw");

        return;
    }

    if (threefoldRepetition())
    {
        gameOver = true;
        drawGame = true;

        std::cout
            << "DRAW - Threefold repetition"
            << std::endl;

        playSound(
            "game-draw");

        return;
    }

    if (check)
    {
        std::cout
            << "CHECK - "
            << (player
                    ? "White king"
                    : "Black king")
            << std::endl;

        playSound(
            "check");
    }
}

// ============================================================
// SELECTION
// ============================================================

void cancelSelection()
{
    pieceSelected = false;

    selectedRow = -1;
    selectedCol = -1;

    legalMoves.clear();
}

bool findSelectedMove(
    int row,
    int col,
    Move& result)
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

// ============================================================
// EXECUTE MOVE
// ============================================================

void executeMove(
    const Move& move)
{
    Piece movingPiece =
        board[move.fromRow][move.fromCol];

    bool wasCapture =
        !isEmpty(
            move.toRow,
            move.toCol) ||
        move.enPassant;

    bool pawnMove =
        movingPiece.type == 'p';

    // Reset en passant
    enPassantRow = -1;
    enPassantCol = -1;

    // Set new en passant target
    if (
        movingPiece.type == 'p' &&
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

    // 50 move counter
    if (
        pawnMove ||
        wasCapture)
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

    // Move sound
    if (
        move.castleKingSide ||
        move.castleQueenSide)
    {
        playSound(
            "castle");
    }
    else if (wasCapture)
    {
        playSound(
            "capture");
    }
    else
    {
        playSound(
            whiteTurn
                ? "move-self"
                : "move-opponent");
    }

    // Promotion must be chosen first
    if (move.promotion)
    {
        promotionPending = true;

        promotionRow =
            move.toRow;

        promotionCol =
            move.toCol;

        promotionChoice = 0;

        cancelSelection();

        return;
    }

    whiteTurn =
        !whiteTurn;

    cancelSelection();

    recordCurrentPosition();

    evaluateGameState();
}

// ============================================================
// PROMOTION
// ============================================================

void confirmPromotion()
{
    if (!promotionPending)
    {
        return;
    }

    board[promotionRow][promotionCol].type =
        promotionPieces[promotionChoice];

    board[promotionRow][promotionCol].moved =
        true;

    promotionPending = false;

    playSound(
        "promote");

    whiteTurn =
        !whiteTurn;

    recordCurrentPosition();

    evaluateGameState();
}

// ============================================================
// PLAYER INPUT
// ============================================================

void moveCursor(
    int rowOffset,
    int colOffset)
{
    if (
        promotionPending ||
        gameOver)
    {
        return;
    }

    cursorRow =
        std::clamp(
            cursorRow + rowOffset,
            0,
            7);

    cursorCol =
        std::clamp(
            cursorCol + colOffset,
            0,
            7);
}

void selectOrMovePiece()
{
    if (
        gameOver ||
        promotionPending)
    {
        return;
    }

    Piece currentPiece =
        board[cursorRow][cursorCol];

    // No selected piece
    if (!pieceSelected)
    {
        if (currentPiece.type == ' ')
        {
            playSound("illegal");

            return;
        }

        if (
            currentPiece.white !=
            whiteTurn)
        {
            playSound("illegal");

            return;
        }

        legalMoves =
            calculateLegalMoves(
                cursorRow,
                cursorCol);

        pieceSelected = true;

        selectedRow =
            cursorRow;

        selectedCol =
            cursorCol;

        playSound("click");

        return;
    }

    // Same square -> deselect
    if (
        selectedRow == cursorRow &&
        selectedCol == cursorCol)
    {
        cancelSelection();

        return;
    }

    // Select another friendly piece
    if (
        currentPiece.type != ' ' &&
        currentPiece.white ==
            whiteTurn)
    {
        legalMoves =
            calculateLegalMoves(
                cursorRow,
                cursorCol);

        selectedRow =
            cursorRow;

        selectedCol =
            cursorCol;

        playSound("click");

        return;
    }

    Move move;

    if (!findSelectedMove(
            cursorRow,
            cursorCol,
            move))
    {
        playSound("illegal");

        return;
    }

    executeMove(move);
}

// ============================================================
// BOARD RENDERING
// ============================================================

void drawBoard(
    SDL_Renderer* renderer)
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
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
// PIECE RENDERING
// ============================================================

void drawPieces(
    SDL_Renderer* renderer)
{
    constexpr int padding = 3;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            Piece piece =
                board[row][col];

            if (piece.type == ' ')
            {
                continue;
            }

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
// SELECTED SQUARE
// ============================================================

void drawSelectedSquare(
    SDL_Renderer* renderer)
{
    if (!pieceSelected)
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
            selectedCol * TILE_SIZE,

        BOARD_Y +
            selectedRow * TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    for (int i = 0; i < 4; i++)
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
// CIRCLE
// ============================================================

void drawCircle(
    SDL_Renderer* renderer,
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

// ============================================================
// LEGAL MOVE MARKERS
// ============================================================

void drawLegalMoves(
    SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    for (const Move& move :
         legalMoves)
    {
        int centerX =
            BOARD_X +
            move.toCol * TILE_SIZE +
            TILE_SIZE / 2;

        int centerY =
            BOARD_Y +
            move.toRow * TILE_SIZE +
            TILE_SIZE / 2;

        bool capture =
            !isEmpty(
                move.toRow,
                move.toCol) ||
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
                renderer,
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

            SDL_Rect border = {
                BOARD_X +
                    move.toCol * TILE_SIZE +
                    4,

                BOARD_Y +
                    move.toRow * TILE_SIZE +
                    4,

                TILE_SIZE - 8,
                TILE_SIZE - 8
            };

            for (int i = 0; i < 4; i++)
            {
                SDL_RenderDrawRect(
                    renderer,
                    &border);

                border.x++;
                border.y++;

                border.w -= 2;
                border.h -= 2;
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

void drawCursor(
    SDL_Renderer* renderer)
{
    SDL_Rect rect = {
        BOARD_X +
            cursorCol * TILE_SIZE,

        BOARD_Y +
            cursorRow * TILE_SIZE,

        TILE_SIZE,
        TILE_SIZE
    };

    SDL_SetRenderDrawColor(
        renderer,
        255,
        215,
        0,
        255);

    for (int i = 0; i < 3; i++)
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
// CHECK INDICATOR
// ============================================================

void drawCheckIndicator(
    SDL_Renderer* renderer)
{
    bool checkedWhite = false;

    if (kingInCheck(true))
    {
        checkedWhite = true;
    }
    else if (!kingInCheck(false))
    {
        return;
    }

    int kingRow = -1;
    int kingCol = -1;

    if (!findKing(
            checkedWhite,
            kingRow,
            kingCol))
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
        120);

    SDL_Rect rect = {
        BOARD_X +
            kingCol * TILE_SIZE,

        BOARD_Y +
            kingRow * TILE_SIZE,

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
// SIMPLE COORDINATE FONT
// ============================================================

void line(
    SDL_Renderer* renderer,
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

void drawLetter(
    SDL_Renderer* renderer,
    char letter,
    int x,
    int y)
{
    constexpr int w = 6;
    constexpr int h = 8;

    switch (letter)
    {
        case 'a':
            line(renderer, x, y + h, x + 3, y);
            line(renderer, x + 3, y, x + w, y + h);
            line(renderer, x + 1, y + 4, x + 5, y + 4);
            break;

        case 'b':
            line(renderer, x, y, x, y + h);
            line(renderer, x, y, x + 5, y);
            line(renderer, x + 5, y, x + 5, y + 4);
            line(renderer, x + 5, y + 4, x, y + 4);
            line(renderer, x + 5, y + 4, x + 5, y + h);
            line(renderer, x + 5, y + h, x, y + h);
            break;

        case 'c':
            line(renderer, x + w, y, x, y);
            line(renderer, x, y, x, y + h);
            line(renderer, x, y + h, x + w, y + h);
            break;

        case 'd':
            line(renderer, x, y, x, y + h);
            line(renderer, x, y, x + 5, y);
            line(renderer, x + 5, y, x + w, y + 4);
            line(renderer, x + w, y + 4, x + 5, y + h);
            line(renderer, x + 5, y + h, x, y + h);
            break;

        case 'e':
            line(renderer, x, y, x, y + h);
            line(renderer, x, y, x + w, y);
            line(renderer, x, y + 4, x + 5, y + 4);
            line(renderer, x, y + h, x + w, y + h);
            break;

        case 'f':
            line(renderer, x, y, x, y + h);
            line(renderer, x, y, x + w, y);
            line(renderer, x, y + 4, x + 5, y + 4);
            break;

        case 'g':
            line(renderer, x + w, y, x, y);
            line(renderer, x, y, x, y + h);
            line(renderer, x, y + h, x + w, y + h);
            line(renderer, x + w, y + h, x + w, y + 4);
            line(renderer, x + w, y + 4, x + 3, y + 4);
            break;

        case 'h':
            line(renderer, x, y, x, y + h);
            line(renderer, x + w, y, x + w, y + h);
            line(renderer, x, y + 4, x + w, y + 4);
            break;
    }
}

void drawNumber(
    SDL_Renderer* renderer,
    int number,
    int x,
    int y)
{
    constexpr int w = 6;
    constexpr int h = 8;

    switch (number)
    {
        case 1:
            line(renderer, x + 3, y, x + 3, y + h);
            break;

        case 2:
            line(renderer, x, y, x + w, y);
            line(renderer, x + w, y, x + w, y + 4);
            line(renderer, x + w, y + 4, x, y + h);
            line(renderer, x, y + h, x + w, y + h);
            break;

        case 3:
            line(renderer, x, y, x + w, y);
            line(renderer, x + w, y, x + w, y + h);
            line(renderer, x, y + 4, x + w, y + 4);
            line(renderer, x, y + h, x + w, y + h);
            break;

        case 4:
            line(renderer, x, y, x, y + 4);
            line(renderer, x, y + 4, x + w, y + 4);
            line(renderer, x + w, y, x + w, y + h);
            break;

        case 5:
            line(renderer, x + w, y, x, y);
            line(renderer, x, y, x, y + 4);
            line(renderer, x, y + 4, x + w, y + 4);
            line(renderer, x + w, y + 4, x + w, y + h);
            line(renderer, x + w, y + h, x, y + h);
            break;

        case 6:
            line(renderer, x + w, y, x, y);
            line(renderer, x, y, x, y + h);
            line(renderer, x, y + h, x + w, y + h);
            line(renderer, x + w, y + h, x + w, y + 4);
            line(renderer, x + w, y + 4, x, y + 4);
            break;

        case 7:
            line(renderer, x, y, x + w, y);
            line(renderer, x + w, y, x, y + h);
            break;

        case 8:
            line(renderer, x, y, x + w, y);
            line(renderer, x, y + h, x + w, y + h);
            line(renderer, x, y, x, y + h);
            line(renderer, x + w, y, x + w, y + h);
            line(renderer, x, y + 4, x + w, y + 4);
            break;
    }
}

void drawCoordinates(
    SDL_Renderer* renderer)
{
    // Rank numbers
    for (int row = 0; row < 8; row++)
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
            renderer,
            rank,
            BOARD_X + 4,
            BOARD_Y +
                row * TILE_SIZE +
                4);
    }

    // File letters
    for (int col = 0; col < 8; col++)
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
            renderer,
            file,

            BOARD_X +
                col * TILE_SIZE +
                TILE_SIZE - 11,

            BOARD_Y +
                BOARD_SIZE - 12);
    }
}

// ============================================================
// SIDE PANEL
// ============================================================

void drawSidePanel(
    SDL_Renderer* renderer)
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

    // Turn indicator
    SDL_SetRenderDrawColor(
        renderer,
        whiteTurn
            ? 235
            : 70,
        whiteTurn
            ? 235
            : 70,
        whiteTurn
            ? 235
            : 70,
        255);

    SDL_Rect turnBox = {
        18,
        18,
        44,
        44
    };

    SDL_RenderFillRect(
        renderer,
        &turnBox);

    // Border
    SDL_SetRenderDrawColor(
        renderer,
        120,
        120,
        120,
        255);

    SDL_RenderDrawRect(
        renderer,
        &turnBox);

    // Check warning
    if (kingInCheck(whiteTurn))
    {
        SDL_SetRenderDrawColor(
            renderer,
            220,
            40,
            40,
            255);

        SDL_Rect checkBox = {
            18,
            75,
            44,
            14
        };

        SDL_RenderFillRect(
            renderer,
            &checkBox);
    }

    // Game over indicator
    if (gameOver)
    {
        SDL_SetRenderDrawColor(
            renderer,
            drawGame
                ? 180
                : 210,
            drawGame
                ? 180
                : 150,
            drawGame
                ? 180
                : 20,
            255);

        SDL_Rect endBox = {
            18,
            100,
            44,
            44
        };

        SDL_RenderFillRect(
            renderer,
            &endBox);
    }
}

// ============================================================
// PROMOTION MENU
// ============================================================

void drawPromotionMenu(
    SDL_Renderer* renderer)
{
    if (!promotionPending)
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

    bool white =
        board[promotionRow]
             [promotionCol]
             .white;

    for (int i = 0; i < 4; i++)
    {
        SDL_Rect box = {
            8,
            88 + i * 76,
            64,
            64
        };

        if (i == promotionChoice)
        {
            SDL_SetRenderDrawColor(
                renderer,
                255,
                215,
                0,
                255);

            for (int border = 0;
                 border < 3;
                 border++)
            {
                SDL_Rect highlight = {
                    box.x + border,
                    box.y + border,
                    box.w - border * 2,
                    box.h - border * 2
                };

                SDL_RenderDrawRect(
                    renderer,
                    &highlight);
            }
        }

        Piece preview = {
            promotionPieces[i],
            white,
            true
        };

        std::string key =
            getPieceKey(
                preview);

        auto it =
            textures.find(key);

        if (
            it != textures.end() &&
            it->second)
        {
            SDL_Rect pieceRect = {
                box.x + 4,
                box.y + 4,
                56,
                56
            };

            SDL_RenderCopy(
                renderer,
                it->second,
                nullptr,
                &pieceRect);
        }
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_NONE);
}

// ============================================================
// MAIN
// ============================================================

int main(
    int argc,
    char* argv[])
{
    (void)argc;
    (void)argv;

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

    bool audioAvailable = true;

    if (
        Mix_OpenAudio(
            44100,
            MIX_DEFAULT_FORMAT,
            2,
            2048) < 0)
    {
        std::cerr
            << "SDL_mixer error: "
            << Mix_GetError()
            << std::endl;

        audioAvailable = false;
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

        if (audioAvailable)
        {
            Mix_CloseAudio();
        }

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

        if (audioAvailable)
        {
            Mix_CloseAudio();
        }

        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    loadPieceTextures(
        renderer);

    if (audioAvailable)
    {
        loadSounds();
    }

    setupBoard();

    // Initial position counts as first occurrence.
    recordCurrentPosition();

    playSound(
        "game-start");

    // ========================================================
    // CONTROLLER DETECTION
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
                    << "Controller: "
                    << SDL_GameControllerName(
                           controller)
                    << std::endl;

                break;
            }
        }
    }

    bool running = true;

    SDL_Event event;

    // ========================================================
    // MAIN LOOP
    // ========================================================

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            // =================================================
            // KEYBOARD
            // =================================================

            if (event.type == SDL_KEYDOWN)
            {
                // Promotion menu
                if (promotionPending)
                {
                    switch (
                        event.key.keysym.sym)
                    {
                        case SDLK_UP:
                            promotionChoice--;

                            if (promotionChoice < 0)
                            {
                                promotionChoice = 3;
                            }

                            playSound("click");
                            break;

                        case SDLK_DOWN:
                            promotionChoice++;

                            if (promotionChoice > 3)
                            {
                                promotionChoice = 0;
                            }

                            playSound("click");
                            break;

                        case SDLK_RETURN:
                        case SDLK_SPACE:
                            confirmPromotion();
                            break;

                        case SDLK_ESCAPE:
                            running = false;
                            break;
                    }

                    continue;
                }

                switch (
                    event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        running = false;
                        break;

                    case SDLK_UP:
                        moveCursor(
                            -1,
                            0);
                        break;

                    case SDLK_DOWN:
                        moveCursor(
                            1,
                            0);
                        break;

                    case SDLK_LEFT:
                        moveCursor(
                            0,
                            -1);
                        break;

                    case SDLK_RIGHT:
                        moveCursor(
                            0,
                            1);
                        break;

                    case SDLK_RETURN:
                    case SDLK_SPACE:
                        selectOrMovePiece();
                        break;

                    case SDLK_BACKSPACE:
                        cancelSelection();
                        break;

                    case SDLK_r:
                        setupBoard();

                        recordCurrentPosition();

                        playSound(
                            "game-start");

                        std::cout
                            << "New game"
                            << std::endl;

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
                // Promotion
                if (promotionPending)
                {
                    switch (
                        event.cbutton.button)
                    {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:
                            promotionChoice--;

                            if (promotionChoice < 0)
                            {
                                promotionChoice = 3;
                            }

                            playSound("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                            promotionChoice++;

                            if (promotionChoice > 3)
                            {
                                promotionChoice = 0;
                            }

                            playSound("click");
                            break;

                        case SDL_CONTROLLER_BUTTON_A:
                            confirmPromotion();
                            break;

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
                        moveCursor(
                            -1,
                            0);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        moveCursor(
                            1,
                            0);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        moveCursor(
                            0,
                            -1);
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        moveCursor(
                            0,
                            1);
                        break;

                    case SDL_CONTROLLER_BUTTON_A:
                        selectOrMovePiece();
                        break;

                    case SDL_CONTROLLER_BUTTON_B:
                        cancelSelection();
                        break;

                    case SDL_CONTROLLER_BUTTON_BACK:
                        setupBoard();

                        recordCurrentPosition();

                        playSound(
                            "game-start");

                        break;

                    case SDL_CONTROLLER_BUTTON_START:
                        running = false;
                        break;
                }
            }
        }

        // =====================================================
        // RENDER
        // =====================================================

        SDL_SetRenderDrawColor(
            renderer,
            35,
            35,
            35,
            255);

        SDL_RenderClear(
            renderer);

        drawSidePanel(
            renderer);

        drawBoard(
            renderer);

        drawSelectedSquare(
            renderer);

        drawLegalMoves(
            renderer);

        drawCheckIndicator(
            renderer);

        drawPieces(
            renderer);

        drawCoordinates(
            renderer);

        if (!gameOver)
        {
            drawCursor(
                renderer);
        }

        drawPromotionMenu(
            renderer);

        SDL_RenderPresent(
            renderer);
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    if (controller)
    {
        SDL_GameControllerClose(
            controller);
    }

    if (audioAvailable)
    {
        destroySounds();

        Mix_CloseAudio();
    }

    destroyTextures();

    SDL_DestroyRenderer(
        renderer);

    SDL_DestroyWindow(
        window);

    IMG_Quit();
    SDL_Quit();

    return 0;
}