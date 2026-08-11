#ifndef LICHESS_CLIENT_H
#define LICHESS_CLIENT_H

#include <curl/curl.h>

#include <string>
#include <atomic>
#include <mutex>
#include <thread>

enum class LichessConnectionState
{
    Disconnected,
    Connecting,
    Connected,
    InGame,
    Error
};

struct LichessChallenge
{
    std::string id;
    std::string challengerUsername;

    std::string speed;
    std::string color;

    bool rated = false;
    bool boardCompatible = false;
    bool active = false;
};

struct LichessGameInfo
{
    std::string gameId;

    std::string whiteUsername;
    std::string blackUsername;

    bool playerIsWhite = true;

    int whiteTimeMs = 0;
    int blackTimeMs = 0;

    std::string moves;
    std::string status;

    bool active = false;
    bool initialized = false;
};

class LichessClient
{
public:
    LichessClient();

    // ========================================================
    // CURL / CONNECTION
    // ========================================================

    bool initialize();

    void shutdown();

    bool authenticate();

    const std::string&
    getLastError() const;

    bool startEventStream();

    void stopEventStream();

    bool isEventStreamRunning() const;

    bool hasIncomingChallenge() const;

    ~LichessClient();

    bool startGameStream(
        const std::string& gameId);

    void stopGameStream();

    bool isGameStreamRunning() const;

    LichessGameInfo
    getCurrentGameSnapshot() const;

    LichessChallenge
    getIncomingChallenge() const;

    bool acceptIncomingChallenge();

    bool declineIncomingChallenge();

    // ========================================================
    // TOKEN
    // ========================================================

    void setToken(
        const std::string& newToken);

    void clearToken();

    bool hasToken() const;

    const std::string&
    getToken() const;

    bool sendMove(
        const std::string& gameId,
        const std::string& move);

    bool resignGame(
        const std::string& gameId);

    // ========================================================
    // USER
    // ========================================================

    void setUsername(
        const std::string& name);

    const std::string&
    getUsername() const;

    // ========================================================
    // CONNECTION STATE
    // ========================================================

    LichessConnectionState
    getConnectionState() const;

    bool isConnected() const;

    void setConnectionState(
        LichessConnectionState state);

    // ========================================================
    // GAME STATE
    // ========================================================

    const LichessGameInfo&
    getCurrentGame() const;

    LichessGameInfo&
    getCurrentGame();

    void clearCurrentGame();

    void reset();

private:
    // ========================================================
    // AUTH DATA
    // ========================================================

    std::string token;

    std::string username;

    std::string lastError;

    // ========================================================
    // CURL
    // ========================================================

    bool curlInitialized = false;

    static size_t writeCallback(
        char* contents,
        size_t size,
        size_t nmemb,
        void* userData);

    bool performAuthenticatedGet(
        const std::string& url,
        std::string& response);

    bool extractUsernameFromJson(
        const std::string& json,
        std::string& result) const;

    // ========================================================
    // STATE
    // ========================================================

    LichessConnectionState connectionState =
        LichessConnectionState::Disconnected;

    LichessGameInfo currentGame;

    // ========================================================
    // GAME STREAM
    // ========================================================

    std::thread gameThread;

    std::atomic<bool> gameStreamRunning{
        false
    };

    std::atomic<bool> stopGameStreamRequested{
        false
    };

    std::string gameBuffer;

    static size_t gameStreamCallback(
        char* contents,
        size_t size,
        size_t nmemb,
        void* userData);

    static int gameStreamProgressCallback(
        void* clientPointer,
        curl_off_t downloadTotal,
        curl_off_t downloadNow,
        curl_off_t uploadTotal,
        curl_off_t uploadNow);

    void gameStreamLoop(
        std::string gameId);

    void processGameData(
        const char* data,
        size_t length);

    void processGameLine(
        const std::string& line);

    bool extractJsonInteger(
        const std::string& json,
        const std::string& key,
        int& value) const;

    // ========================================================
    // EVENT STREAM
    // ========================================================

    std::thread eventThread;

    std::atomic<bool> eventStreamRunning{
        false
    };

    std::atomic<bool> stopEventStreamRequested{
        false
    };

    mutable std::mutex eventMutex;

    std::string eventBuffer;

    LichessChallenge incomingChallenge;

    // ========================================================
    // STREAM CALLBACKS
    // ========================================================

    static size_t eventStreamCallback(
        char* contents,
        size_t size,
        size_t nmemb,
        void* userData);

    void eventStreamLoop();

    void processEventData(
        const char* data,
        size_t length);

    void processEventLine(
        const std::string& line);

    // ========================================================
    // HTTP
    // ========================================================

    bool performAuthenticatedPost(
        const std::string& url);

    bool extractJsonString(
        const std::string& json,
        const std::string& key,
        std::string& result) const;

    bool extractNestedJsonString(
        const std::string& json,
        const std::string& objectKey,
        const std::string& key,
        std::string& result) const;

    bool jsonContainsTrue(
        const std::string& json,
        const std::string& key) const;


    static int streamProgressCallback(
        void* clientPointer,
        curl_off_t downloadTotal,
        curl_off_t downloadNow,
        curl_off_t uploadTotal,
        curl_off_t uploadNow);

};

#endif