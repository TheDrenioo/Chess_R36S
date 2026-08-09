#include "StockfishEngine.h"

#include <algorithm>
#include <iostream>
#include <string>

#ifdef _WIN32

StockfishEngine::StockfishEngine()
{
}

StockfishEngine::~StockfishEngine()
{
    stop();
}

bool StockfishEngine::start(
    const std::string& enginePath)
{
    if (running)
    {
        return true;
    }

    SECURITY_ATTRIBUTES securityAttributes{};

    securityAttributes.nLength =
        sizeof(SECURITY_ATTRIBUTES);

    securityAttributes.bInheritHandle =
        TRUE;

    securityAttributes.lpSecurityDescriptor =
        nullptr;

    HANDLE outputRead = nullptr;
    HANDLE outputWrite = nullptr;

    HANDLE inputRead = nullptr;
    HANDLE inputWrite = nullptr;

    // --------------------------------------------------------
    // Stockfish stdout pipe
    // --------------------------------------------------------

    if (!CreatePipe(
            &outputRead,
            &outputWrite,
            &securityAttributes,
            0))
    {
        std::cerr
            << "Could not create Stockfish output pipe."
            << std::endl;

        return false;
    }

    SetHandleInformation(
        outputRead,
        HANDLE_FLAG_INHERIT,
        0);

    // --------------------------------------------------------
    // Stockfish stdin pipe
    // --------------------------------------------------------

    if (!CreatePipe(
            &inputRead,
            &inputWrite,
            &securityAttributes,
            0))
    {
        CloseHandle(outputRead);
        CloseHandle(outputWrite);

        std::cerr
            << "Could not create Stockfish input pipe."
            << std::endl;

        return false;
    }

    SetHandleInformation(
        inputWrite,
        HANDLE_FLAG_INHERIT,
        0);

    // --------------------------------------------------------
    // Process configuration
    // --------------------------------------------------------

    STARTUPINFOA startupInfo{};

    startupInfo.cb =
        sizeof(STARTUPINFOA);

    startupInfo.dwFlags =
        STARTF_USESTDHANDLES |
        STARTF_USESHOWWINDOW;

    startupInfo.wShowWindow =
        SW_HIDE;

    startupInfo.hStdInput =
        inputRead;

    startupInfo.hStdOutput =
        outputWrite;

    startupInfo.hStdError =
        outputWrite;

    std::string command =
        "\"" + enginePath + "\"";

    bool success =
        CreateProcessA(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo);

    CloseHandle(inputRead);
    CloseHandle(outputWrite);

    if (!success)
    {
        CloseHandle(outputRead);
        CloseHandle(inputWrite);

        std::cerr
            << "Could not start Stockfish."
            << std::endl;

        return false;
    }

    engineOutputRead =
        outputRead;

    engineInputWrite =
        inputWrite;

    running = true;

    // --------------------------------------------------------
    // Initialize UCI
    // --------------------------------------------------------

    sendCommand("uci");

    if (!waitFor("uciok"))
    {
        std::cerr
            << "Stockfish did not respond to UCI."
            << std::endl;

        stop();

        return false;
    }

    // Low resource configuration for R36S-like behavior.
    sendCommand(
        "setoption name Threads value 1");

    sendCommand(
        "setoption name Hash value 32");

    sendCommand(
        "isready");

    if (!waitFor("readyok"))
    {
        std::cerr
            << "Stockfish is not ready."
            << std::endl;

        stop();

        return false;
    }

    sendCommand(
        "ucinewgame");

    sendCommand(
        "isready");

    waitFor(
        "readyok");

    std::cout
        << "Stockfish started successfully."
        << std::endl;

    return true;
}

void StockfishEngine::stop()
{
    if (!running)
    {
        return;
    }

    sendCommand(
        "quit");

    WaitForSingleObject(
        processInfo.hProcess,
        1000);

    if (engineInputWrite)
    {
        CloseHandle(
            engineInputWrite);

        engineInputWrite =
            nullptr;
    }

    if (engineOutputRead)
    {
        CloseHandle(
            engineOutputRead);

        engineOutputRead =
            nullptr;
    }

    if (processInfo.hProcess)
    {
        CloseHandle(
            processInfo.hProcess);

        processInfo.hProcess =
            nullptr;
    }

    if (processInfo.hThread)
    {
        CloseHandle(
            processInfo.hThread);

        processInfo.hThread =
            nullptr;
    }

    running = false;
}

bool StockfishEngine::isRunning() const
{
    return running;
}

bool StockfishEngine::sendCommand(
    const std::string& command)
{
    if (!running)
    {
        return false;
    }

    std::string commandLine =
        command + "\n";

    DWORD bytesWritten = 0;

    BOOL result =
        WriteFile(
            engineInputWrite,
            commandLine.c_str(),
            static_cast<DWORD>(
                commandLine.size()),
            &bytesWritten,
            nullptr);

    return result == TRUE;
}

std::string StockfishEngine::readLine()
{
    if (!running)
    {
        return "";
    }

    std::string line;

    char character = 0;

    DWORD bytesRead = 0;

    while (true)
    {
        BOOL result =
            ReadFile(
                engineOutputRead,
                &character,
                1,
                &bytesRead,
                nullptr);

        if (
            !result ||
            bytesRead == 0)
        {
            return line;
        }

        if (character == '\n')
        {
            break;
        }

        if (character != '\r')
        {
            line += character;
        }
    }

    return line;
}

bool StockfishEngine::waitFor(
    const std::string& expected)
{
    while (running)
    {
        std::string line =
            readLine();

        if (line.empty())
        {
            continue;
        }

        if (
            line.find(expected) !=
            std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool StockfishEngine::setSkillLevel(
    int level)
{
    if (!running)
    {
        return false;
    }

    level =
        std::clamp(
            level,
            0,
            20);

    sendCommand(
        "setoption name Skill Level value " +
        std::to_string(level));

    sendCommand(
        "isready");

    return waitFor(
        "readyok");
}

std::string StockfishEngine::getBestMove(
    const std::string& fen,
    int moveTimeMs)
{
    if (!running)
    {
        return "";
    }

    moveTimeMs =
        std::max(
            moveTimeMs,
            50);

    sendCommand(
        "position fen " +
        fen);

    sendCommand(
        "go movetime " +
        std::to_string(
            moveTimeMs));

    while (running)
    {
        std::string line =
            readLine();

        if (
            line.rfind(
                "bestmove ",
                0) == 0)
        {
            std::string move =
                line.substr(9);

            std::size_t space =
                move.find(' ');

            if (
                space !=
                std::string::npos)
            {
                move =
                    move.substr(
                        0,
                        space);
            }

            return move;
        }
    }

    return "";
}

#else

// Temporary fallback.
// The R36S Linux implementation will be added
// when we begin the ARM port.

StockfishEngine::StockfishEngine()
{
}

StockfishEngine::~StockfishEngine()
{
}

bool StockfishEngine::start(
    const std::string&)
{
    return false;
}

void StockfishEngine::stop()
{
}

bool StockfishEngine::isRunning() const
{
    return false;
}

bool StockfishEngine::setSkillLevel(
    int)
{
    return false;
}

bool StockfishEngine::sendCommand(
    const std::string&)
{
    return false;
}

std::string StockfishEngine::readLine()
{
    return "";
}

bool StockfishEngine::waitFor(
    const std::string&)
{
    return false;
}

std::string StockfishEngine::getBestMove(
    const std::string&,
    int)
{
    return "";
}

#endif