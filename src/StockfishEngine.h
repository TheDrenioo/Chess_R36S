#ifndef STOCKFISH_ENGINE_H
#define STOCKFISH_ENGINE_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class StockfishEngine
{
public:
    StockfishEngine();
    ~StockfishEngine();

    bool start(
        const std::string& enginePath);

    void stop();

    bool isRunning() const;

    bool setSkillLevel(
        int level);

    std::string getBestMove(
        const std::string& fen,
        int moveTimeMs);

private:
#ifdef _WIN32
    PROCESS_INFORMATION processInfo{};

    HANDLE engineInputWrite = nullptr;
    HANDLE engineOutputRead = nullptr;
#endif

    bool running = false;

    bool sendCommand(
        const std::string& command);

    std::string readLine();

    bool waitFor(
        const std::string& expected);
};

#endif