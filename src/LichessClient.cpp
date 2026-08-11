#include "LichessClient.h"

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cctype>

LichessClient::LichessClient()
{
}

LichessClient::~LichessClient()
{
    stopGameStream();

    stopEventStream();

    shutdown();
}

bool LichessClient::initialize()
{
    if (curlInitialized)
    {
        return true;
    }

    CURLcode result =
        curl_global_init(
            CURL_GLOBAL_DEFAULT);

    if (result != CURLE_OK)
    {
        lastError =
            curl_easy_strerror(
                result);

        connectionState =
            LichessConnectionState::Error;

        return false;
    }

    curlInitialized = true;

    return true;
}

void LichessClient::shutdown()
{
    if (!curlInitialized)
    {
        return;
    }

    curl_global_cleanup();

    curlInitialized = false;
}

const std::string&
LichessClient::getLastError() const
{
    return lastError;
}

size_t LichessClient::writeCallback(
    char* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    size_t totalSize =
        size * nmemb;

    std::string* response =
        static_cast<std::string*>(
            userData);

    response->append(
        contents,
        totalSize);

    return totalSize;
}

bool LichessClient::performAuthenticatedGet(
    const std::string& url,
    std::string& response)
{
    if (!curlInitialized)
    {
        lastError =
            "libcurl is not initialized.";

        return false;
    }

    if (token.empty())
    {
        lastError =
            "No Lichess token configured.";

        return false;
    }

    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        lastError =
            "Could not create CURL handle.";

        return false;
    }

    char errorBuffer[CURL_ERROR_SIZE] = {0};

    curl_easy_setopt(
        curl,
        CURLOPT_ERRORBUFFER,
        errorBuffer);

    response.clear();

    std::string authorization =
        "Authorization: Bearer " +
        token;

    struct curl_slist* headers =
        nullptr;

    headers =
        curl_slist_append(
            headers,
            authorization.c_str());

    headers =
        curl_slist_append(
            headers,
            "Accept: application/json");

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str());

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response);

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "ChessR36S/0.1");

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L);

    // ========================================================
    // WINDOWS TLS / CERTIFICATES
    // ========================================================

    // Use Windows native certificate store.
    curl_easy_setopt(
        curl,
        CURLOPT_SSL_OPTIONS,
        CURLSSLOPT_NATIVE_CA);

    // Also provide curl's CA bundle as fallback.
    curl_easy_setopt(
        curl,
        CURLOPT_CAINFO,
        "external/curl/bin/curl-ca-bundle.crt");

    // TLS verification remains ENABLED.
    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L);

    CURLcode result =
        curl_easy_perform(
            curl);

    if (result != CURLE_OK)
    {
        if (errorBuffer[0] != '\0')
        {
            lastError =
                errorBuffer;
        }
        else
        {
            lastError =
                curl_easy_strerror(
                    result);
        }

        curl_slist_free_all(
            headers);

        curl_easy_cleanup(
            curl);

        return false;
    }

    long httpCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpCode);

    curl_slist_free_all(
        headers);

    curl_easy_cleanup(
        curl);

    if (httpCode < 200 ||
        httpCode >= 300)
    {
        lastError =
            "HTTP error " +
            std::to_string(
                httpCode);

        return false;
    }

    return true;
}

bool LichessClient::extractUsernameFromJson(
    const std::string& json,
    std::string& result) const
{
    const std::string key =
        "\"username\"";

    std::size_t keyPosition =
        json.find(key);

    if (
        keyPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t colonPosition =
        json.find(
            ':',
            keyPosition +
                key.length());

    if (
        colonPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t firstQuote =
        json.find(
            '"',
            colonPosition + 1);

    if (
        firstQuote ==
        std::string::npos)
    {
        return false;
    }

    std::size_t secondQuote =
        json.find(
            '"',
            firstQuote + 1);

    if (
        secondQuote ==
        std::string::npos)
    {
        return false;
    }

    result =
        json.substr(
            firstQuote + 1,
            secondQuote -
                firstQuote -
                1);

    return !result.empty();
}

bool LichessClient::extractJsonInteger(
    const std::string& json,
    const std::string& key,
    int& value) const
{
    std::string searchKey =
        "\"" + key + "\"";

    std::size_t keyPosition =
        json.find(searchKey);

    if (
        keyPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t colonPosition =
        json.find(
            ':',
            keyPosition +
                searchKey.length());

    if (
        colonPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t numberStart =
        colonPosition + 1;

    while (
        numberStart <
            json.length() &&
        std::isspace(
            static_cast<unsigned char>(
                json[numberStart])))
    {
        numberStart++;
    }

    std::size_t numberEnd =
        numberStart;

    while (
        numberEnd <
            json.length() &&
        std::isdigit(
            static_cast<unsigned char>(
                json[numberEnd])))
    {
        numberEnd++;
    }

    if (
        numberEnd ==
        numberStart)
    {
        return false;
    }

    value =
        std::stoi(
            json.substr(
                numberStart,
                numberEnd -
                    numberStart));

    return true;
}

size_t LichessClient::gameStreamCallback(
    char* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    size_t totalSize =
        size * nmemb;

    LichessClient* client =
        static_cast<LichessClient*>(
            userData);

    if (!client)
    {
        return 0;
    }

    client->processGameData(
        contents,
        totalSize);

    return totalSize;
}

int LichessClient::gameStreamProgressCallback(
    void* clientPointer,
    curl_off_t,
    curl_off_t,
    curl_off_t,
    curl_off_t)
{
    LichessClient* client =
        static_cast<LichessClient*>(
            clientPointer);

    if (!client)
    {
        return 1;
    }

    return
        client
            ->stopGameStreamRequested
        ? 1
        : 0;
}

void LichessClient::processGameData(
    const char* data,
    size_t length)
{
    gameBuffer.append(
        data,
        length);

    while (true)
    {
        std::size_t newline =
            gameBuffer.find('\n');

        if (
            newline ==
            std::string::npos)
        {
            break;
        }

        std::string line =
            gameBuffer.substr(
                0,
                newline);

        gameBuffer.erase(
            0,
            newline + 1);

        if (
            !line.empty() &&
            line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        processGameLine(
            line);
    }
}

void LichessClient::processGameLine(
    const std::string& line)
{
    // ========================================================
    // GAME FULL
    // ========================================================

    if (
        line.find(
            "\"type\":\"gameFull\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"gameFull\"") !=
            std::string::npos)
    {
        std::cout
        << "[LICHESS] gameFull received."
        << std::endl;

        LichessGameInfo updated;

        {
            std::lock_guard<std::mutex>
                lock(eventMutex);

            updated.gameId =
                currentGame.gameId;
        }


        // ----------------------------------------------------
        // WHITE
        // ----------------------------------------------------

        extractNestedJsonString(
            line,
            "white",
            "name",
            updated.whiteUsername);

        if (
            updated.whiteUsername.empty())
        {
            extractNestedJsonString(
                line,
                "white",
                "id",
                updated.whiteUsername);
        }

        // ----------------------------------------------------
        // BLACK
        // ----------------------------------------------------

        extractNestedJsonString(
            line,
            "black",
            "name",
            updated.blackUsername);

        if (
            updated.blackUsername.empty())
        {
            extractNestedJsonString(
                line,
                "black",
                "id",
                updated.blackUsername);
        }

        // ----------------------------------------------------
        // STATE OBJECT
        // ----------------------------------------------------

        std::size_t statePosition =
            line.find(
                "\"state\"");

        if (
            statePosition !=
            std::string::npos)
        {
            std::string stateJson =
                line.substr(
                    statePosition);

            extractJsonString(
                stateJson,
                "moves",
                updated.moves);

            extractJsonString(
                stateJson,
                "status",
                updated.status);

            extractJsonInteger(
                stateJson,
                "wtime",
                updated.whiteTimeMs);

            extractJsonInteger(
                stateJson,
                "btime",
                updated.blackTimeMs);
        }

        // ----------------------------------------------------
        // PLAYER COLOR
        // ----------------------------------------------------

        std::string ownLower =
            username;

        std::string whiteLower =
            updated.whiteUsername;

        std::transform(
            ownLower.begin(),
            ownLower.end(),
            ownLower.begin(),
            [](unsigned char c)
            {
                return
                    static_cast<char>(
                        std::tolower(c));
            });

        std::transform(
            whiteLower.begin(),
            whiteLower.end(),
            whiteLower.begin(),
            [](unsigned char c)
            {
                return
                    static_cast<char>(
                        std::tolower(c));
            });

        updated.playerIsWhite =
            ownLower ==
            whiteLower;

        std::cout
            << "[LICHESS] Playing as: "
            << (
                updated.playerIsWhite
                    ? "WHITE"
                    : "BLACK"
            )
            << std::endl;

        std::cout
            << "[LICHESS] White: "
            << updated.whiteUsername
            << std::endl;

        std::cout
            << "[LICHESS] Black: "
            << updated.blackUsername
            << std::endl;

        std::cout
            << "[LICHESS] Playing as: "
            << (
                updated.playerIsWhite
                    ? "WHITE"
                    : "BLACK"
            )
            << std::endl;

        updated.active = true;
        updated.initialized = true;

        {
            std::lock_guard<std::mutex>
                lock(eventMutex);

            currentGame =
                updated;
        }

        connectionState =
            LichessConnectionState::
                InGame;

        return;
    }

    // ========================================================
    // GAME STATE
    // ========================================================

    if (
        line.find(
            "\"type\":\"gameState\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"gameState\"") !=
            std::string::npos)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        extractJsonString(
            line,
            "moves",
            currentGame.moves);

        extractJsonString(
            line,
            "status",
            currentGame.status);

        extractJsonInteger(
            line,
            "wtime",
            currentGame.whiteTimeMs);

        extractJsonInteger(
            line,
            "btime",
            currentGame.blackTimeMs);

        if (
            currentGame.status !=
                "started" &&
            currentGame.status !=
                "created")
        {
            currentGame.active =
                false;
        }

        return;
    }
}

void LichessClient::gameStreamLoop(
    std::string gameId)
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        gameStreamRunning =
            false;

        return;
    }

    char errorBuffer[CURL_ERROR_SIZE] = {0};

    curl_easy_setopt(
        curl,
        CURLOPT_ERRORBUFFER,
        errorBuffer);

    std::string authorization =
        "Authorization: Bearer " +
        token;

    struct curl_slist* headers =
        nullptr;

    headers =
        curl_slist_append(
            headers,
            authorization.c_str());

    headers =
        curl_slist_append(
            headers,
            "Accept: application/x-ndjson");

    std::string url =
        "https://lichess.org/api/board/game/stream/" +
        gameId;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str());

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        gameStreamCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        this);

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "ChessR36S/0.1");

    curl_easy_setopt(
        curl,
        CURLOPT_NOSIGNAL,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_OPTIONS,
        CURLSSLOPT_NATIVE_CA);

    curl_easy_setopt(
        curl,
        CURLOPT_CAINFO,
        "external/curl/bin/curl-ca-bundle.crt");

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L);

    curl_easy_setopt(
        curl,
        CURLOPT_TCP_KEEPALIVE,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_NOPROGRESS,
        0L);

    curl_easy_setopt(
        curl,
        CURLOPT_XFERINFOFUNCTION,
        gameStreamProgressCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_XFERINFODATA,
        this);

    std::cout
    << "[LICHESS] Connecting to game stream..."
    << std::endl;

    CURLcode result =
        curl_easy_perform(
            curl);

    long httpCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpCode);

    gameStreamRunning =
        false;

    if (
        result != CURLE_OK &&
        !stopGameStreamRequested)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        if (errorBuffer[0] != '\0')
        {
            lastError =
                errorBuffer;
        }
        else
        {
            lastError =
                curl_easy_strerror(
                    result);
        }

        std::cerr
            << "[LICHESS] Game stream error: "
            << lastError
            << std::endl;
    }
    else if (
        httpCode < 200 ||
        httpCode >= 300)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        lastError =
            "Game stream HTTP error " +
            std::to_string(
                httpCode);

        std::cerr
            << "[LICHESS] "
            << lastError
            << std::endl;
    }
    else if (
        !stopGameStreamRequested)
    {
        std::cout
            << "[LICHESS] Game stream ended."
            << std::endl;
    }

    curl_slist_free_all(
        headers);

    curl_easy_cleanup(
        curl);
}

bool LichessClient::startGameStream(
    const std::string& gameId)
{
    if (gameId.empty())
    {
        lastError =
            "Cannot start game stream: empty game ID.";

        return false;
    }

    if (!curlInitialized)
    {
        lastError =
            "Cannot start game stream: libcurl is not initialized.";

        return false;
    }

    if (token.empty())
    {
        lastError =
            "Cannot start game stream: no Lichess token.";

        return false;
    }

    // Already running.
    if (gameStreamRunning)
    {
        return true;
    }

    // Clean up a PREVIOUS thread only if it has already finished.
    if (gameThread.joinable())
    {
        gameThread.join();
    }

    stopGameStreamRequested =
        false;

    gameBuffer.clear();

    // ========================================================
    // IMPORTANT
    //
    // Mark it as running BEFORE starting the thread.
    // Otherwise main() can attempt to start the stream twice
    // before the new thread has had time to execute.
    // ========================================================

    gameStreamRunning =
        true;

    std::cout
        << "[LICHESS] Starting game stream: "
        << gameId
        << std::endl;

    gameThread =
        std::thread(
            &LichessClient::gameStreamLoop,
            this,
            gameId);

    return true;
}

void LichessClient::stopGameStream()
{
    stopGameStreamRequested =
        true;

    if (gameThread.joinable())
    {
        gameThread.join();
    }

    gameStreamRunning =
        false;
}

bool LichessClient::isGameStreamRunning() const
{
    return gameStreamRunning;
}

LichessGameInfo
LichessClient::getCurrentGameSnapshot() const
{
    std::lock_guard<std::mutex>
        lock(eventMutex);

    return currentGame;
}

bool LichessClient::extractJsonString(
    const std::string& json,
    const std::string& key,
    std::string& result) const
{
    std::string searchKey =
        "\"" + key + "\"";

    std::size_t keyPosition =
        json.find(searchKey);

    if (
        keyPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t colon =
        json.find(
            ':',
            keyPosition +
                searchKey.length());

    if (
        colon ==
        std::string::npos)
    {
        return false;
    }

    std::size_t firstQuote =
        json.find(
            '"',
            colon + 1);

    if (
        firstQuote ==
        std::string::npos)
    {
        return false;
    }

    std::size_t secondQuote =
        json.find(
            '"',
            firstQuote + 1);

    if (
        secondQuote ==
        std::string::npos)
    {
        return false;
    }

    result =
        json.substr(
            firstQuote + 1,
            secondQuote -
                firstQuote -
                1);

    return true;
}

bool LichessClient::extractNestedJsonString(
    const std::string& json,
    const std::string& objectKey,
    const std::string& key,
    std::string& result) const
{
    std::string objectSearch =
        "\"" +
        objectKey +
        "\"";

    std::size_t objectPosition =
        json.find(
            objectSearch);

    if (
        objectPosition ==
        std::string::npos)
    {
        return false;
    }

    std::size_t objectStart =
        json.find(
            '{',
            objectPosition);

    if (
        objectStart ==
        std::string::npos)
    {
        return false;
    }

    int depth = 0;

    std::size_t objectEnd =
        std::string::npos;

    for (
        std::size_t i =
            objectStart;
        i < json.length();
        i++)
    {
        if (json[i] == '{')
        {
            depth++;
        }
        else if (
            json[i] == '}')
        {
            depth--;

            if (depth == 0)
            {
                objectEnd = i;

                break;
            }
        }
    }

    if (
        objectEnd ==
        std::string::npos)
    {
        return false;
    }

    std::string objectJson =
        json.substr(
            objectStart,
            objectEnd -
                objectStart +
                1);

    return extractJsonString(
        objectJson,
        key,
        result);
}

bool LichessClient::jsonContainsTrue(
    const std::string& json,
    const std::string& key) const
{
    std::string search =
        "\"" +
        key +
        "\":true";

    if (
        json.find(search) !=
        std::string::npos)
    {
        return true;
    }

    // Account for whitespace.
    search =
        "\"" +
        key +
        "\": true";

    return
        json.find(search) !=
        std::string::npos;
}

size_t LichessClient::eventStreamCallback(
    char* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    size_t totalSize =
        size * nmemb;

    LichessClient* client =
        static_cast<LichessClient*>(
            userData);

    if (!client)
    {
        return 0;
    }

    client->processEventData(
        contents,
        totalSize);

    return totalSize;
}

bool LichessClient::authenticate()
{
    connectionState =
        LichessConnectionState::Connecting;

    lastError.clear();

    // --------------------------------------------------------
    // Load token from environment if none was configured.
    // --------------------------------------------------------

    if (token.empty())
    {
        const char* environmentToken =
            std::getenv(
                "CHESSR36S_LICHESS_TOKEN");

        if (environmentToken)
        {
            token =
                environmentToken;
        }
    }

    if (token.empty())
    {
        lastError =
            "CHESSR36S_LICHESS_TOKEN is not configured.";

        connectionState =
            LichessConnectionState::Error;

        return false;
    }

    std::string response;

    bool success =
        performAuthenticatedGet(
            "https://lichess.org/api/account",
            response);

    if (!success)
    {
        connectionState =
            LichessConnectionState::Error;

        return false;
    }

    // ========================================================
    // EXTRACT ACCOUNT USERNAME
    // ========================================================

    std::string accountUsername;

    if (!extractUsernameFromJson(
            response,
            accountUsername))
    {
        lastError =
            "Could not read username from Lichess account response.";

        connectionState =
            LichessConnectionState::Error;

        return false;
    }

    username =
        accountUsername;

    // Authentication and account data are valid.
    connectionState =
        LichessConnectionState::Connected;

    return true;
}

void LichessClient::setToken(
    const std::string& newToken)
{
    token =
        newToken;
}

void LichessClient::clearToken()
{
    token.clear();
}

bool LichessClient::hasToken() const
{
    return
        !token.empty();
}

const std::string&
LichessClient::getToken() const
{
    return token;
}

void LichessClient::setUsername(
    const std::string& name)
{
    username =
        name;
}

const std::string&
LichessClient::getUsername() const
{
    return username;
}

LichessConnectionState
LichessClient::getConnectionState() const
{
    return connectionState;
}

bool LichessClient::isConnected() const
{
    return
        connectionState ==
            LichessConnectionState::Connected ||
        connectionState ==
            LichessConnectionState::InGame;
}

void LichessClient::setConnectionState(
    LichessConnectionState state)
{
    connectionState =
        state;
}

const LichessGameInfo&
LichessClient::getCurrentGame() const
{
    return currentGame;
}

LichessGameInfo&
LichessClient::getCurrentGame()
{
    return currentGame;
}

void LichessClient::clearCurrentGame()
{
    std::lock_guard<std::mutex>
        lock(eventMutex);

    currentGame =
        LichessGameInfo{};
}

void LichessClient::reset()
{
    username.clear();

    connectionState =
        LichessConnectionState::Disconnected;

    clearCurrentGame();
}

void LichessClient::processEventData(
    const char* data,
    size_t length)
{
    eventBuffer.append(
        data,
        length);

    while (true)
    {
        std::size_t newline =
            eventBuffer.find('\n');

        if (
            newline ==
            std::string::npos)
        {
            break;
        }

        std::string line =
            eventBuffer.substr(
                0,
                newline);

        eventBuffer.erase(
            0,
            newline + 1);

        if (
            !line.empty() &&
            line.back() == '\r')
        {
            line.pop_back();
        }

        // Lichess sends an empty line
        // periodically as keep-alive.
        if (line.empty())
        {
            continue;
        }

        processEventLine(
            line);
    }
}

void LichessClient::processEventLine(
    const std::string& line)
{
    // ========================================================
    // CHALLENGE
    // ========================================================

    if (
        line.find(
            "\"type\":\"challenge\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"challenge\"") !=
            std::string::npos)
    {
        LichessChallenge challenge;

        if (!extractNestedJsonString(
                line,
                "challenge",
                "id",
                challenge.id))
        {
            return;
        }

        extractNestedJsonString(
            line,
            "challenger",
            "name",
            challenge
                .challengerUsername);

        extractNestedJsonString(
            line,
            "challenge",
            "speed",
            challenge.speed);

        extractNestedJsonString(
            line,
            "challenge",
            "color",
            challenge.color);

        challenge.rated =
            jsonContainsTrue(
                line,
                "rated");

        // Board API compatibility is supplied
        // by the event's compat object.
        std::size_t compatPosition =
            line.find(
                "\"compat\"");

        if (
            compatPosition !=
            std::string::npos)
        {
            std::string compatPart =
                line.substr(
                    compatPosition);

            challenge.boardCompatible =
                jsonContainsTrue(
                    compatPart,
                    "board");
        }

        // ----------------------------------------------------
        // We only want challenges SENT TO US.
        // ----------------------------------------------------

        std::string destinationUsername;

        extractNestedJsonString(
            line,
            "destUser",
            "name",
            destinationUsername);

        if (
            !username.empty() &&
            !destinationUsername.empty())
        {
            std::string ownLower =
                username;

            std::string destinationLower =
                destinationUsername;

            std::transform(
                ownLower.begin(),
                ownLower.end(),
                ownLower.begin(),
                [](unsigned char c)
                {
                    return
                        static_cast<char>(
                            std::tolower(c));
                });

            std::transform(
                destinationLower.begin(),
                destinationLower.end(),
                destinationLower.begin(),
                [](unsigned char c)
                {
                    return
                        static_cast<char>(
                            std::tolower(c));
                });

            if (
                ownLower !=
                destinationLower)
            {
                // This is an outgoing challenge.
                return;
            }
        }

        challenge.active = true;

        {
            std::lock_guard<std::mutex>
                lock(eventMutex);

            incomingChallenge =
                challenge;
        }

        return;
    }

    // ========================================================
    // CHALLENGE CANCELED
    // ========================================================

    if (
        line.find(
            "\"type\":\"challengeCanceled\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"challengeCanceled\"") !=
            std::string::npos)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        incomingChallenge =
            LichessChallenge{};

        return;
    }

    // ========================================================
    // GAME START
    // ========================================================

    if (
        line.find(
            "\"type\":\"gameStart\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"gameStart\"") !=
            std::string::npos)
    {
        std::string gameId;

        // ----------------------------------------------------
        // Lichess gameStart uses:
        //
        // "game": {
        //     "gameId": "xxxxxxxx",
        //     ...
        // }
        // ----------------------------------------------------

        bool foundGameId =
            extractNestedJsonString(
                line,
                "game",
                "gameId",
                gameId);

        if (
            !foundGameId ||
            gameId.empty())
        {
            std::cerr
                << "[LICHESS] gameStart received "
                << "without a valid gameId."
                << std::endl;

            return;
        }

        std::cout
            << "[LICHESS] gameStart received. ID: "
            << gameId
            << std::endl;

        {
            std::lock_guard<std::mutex>
                lock(eventMutex);

            currentGame =
                LichessGameInfo{};

            currentGame.gameId =
                gameId;

            currentGame.active =
                true;

            currentGame.initialized =
                false;

            incomingChallenge =
                LichessChallenge{};
        }

        connectionState =
            LichessConnectionState::InGame;

        return;
    }

    // ========================================================
    // GAME FINISH
    // ========================================================

    if (
        line.find(
            "\"type\":\"gameFinish\"") !=
            std::string::npos ||
        line.find(
            "\"type\": \"gameFinish\"") !=
            std::string::npos)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        currentGame.active =
            false;

        connectionState =
            LichessConnectionState::
                Connected;

        return;
    }
}

void LichessClient::eventStreamLoop()
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        eventStreamRunning =
            false;

        return;
    }

    std::string authorization =
        "Authorization: Bearer " +
        token;

    struct curl_slist* headers =
        nullptr;

    headers =
        curl_slist_append(
            headers,
            authorization.c_str());

    headers =
        curl_slist_append(
            headers,
            "Accept: application/x-ndjson");

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "https://lichess.org/api/stream/event");

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        eventStreamCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        this);

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "ChessR36S/0.1");

    curl_easy_setopt(
        curl,
        CURLOPT_FOLLOWLOCATION,
        1L);

    // Prevent libcurl from using signals.
    // Important when using threads.
    curl_easy_setopt(
        curl,
        CURLOPT_NOSIGNAL,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_OPTIONS,
        CURLSSLOPT_NATIVE_CA);

    curl_easy_setopt(
        curl,
        CURLOPT_CAINFO,
        "external/curl/bin/curl-ca-bundle.crt");

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L);

    eventStreamRunning =
        true;

    curl_easy_setopt(
        curl,
        CURLOPT_NOPROGRESS,
        0L);

    curl_easy_setopt(
        curl,
        CURLOPT_XFERINFOFUNCTION,
        streamProgressCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_XFERINFODATA,
        this);

    CURLcode result =
        curl_easy_perform(
            curl);

    eventStreamRunning =
        false;

    if (
        result != CURLE_OK &&
        !stopEventStreamRequested)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        lastError =
            curl_easy_strerror(
                result);
    }

    curl_slist_free_all(
        headers);

    curl_easy_cleanup(
        curl);
}

int LichessClient::streamProgressCallback(
    void* clientPointer,
    curl_off_t,
    curl_off_t,
    curl_off_t,
    curl_off_t)
{
    LichessClient* client =
        static_cast<LichessClient*>(
            clientPointer);

    if (!client)
    {
        return 1;
    }

    return
        client
            ->stopEventStreamRequested
        ? 1
        : 0;
}

bool LichessClient::startEventStream()
{
    if (!isConnected())
    {
        lastError =
            "Lichess account is not connected.";

        return false;
    }

    if (eventStreamRunning)
    {
        return true;
    }

    if (eventThread.joinable())
    {
        eventThread.join();
    }

    stopEventStreamRequested =
        false;

    eventBuffer.clear();

    eventThread =
        std::thread(
            &LichessClient::
                eventStreamLoop,
            this);

    return true;
}

void LichessClient::stopEventStream()
{
    stopEventStreamRequested =
        true;

    if (eventThread.joinable())
    {
        eventThread.join();
    }

    eventStreamRunning =
        false;
}

bool LichessClient::isEventStreamRunning() const
{
    return eventStreamRunning;
}

bool LichessClient::hasIncomingChallenge() const
{
    std::lock_guard<std::mutex>
        lock(eventMutex);

    return
        incomingChallenge.active;
}

LichessChallenge
LichessClient::getIncomingChallenge() const
{
    std::lock_guard<std::mutex>
        lock(eventMutex);

    return incomingChallenge;
}

bool LichessClient::performAuthenticatedPost(
    const std::string& url)
{
    if (
        !curlInitialized ||
        token.empty())
    {
        return false;
    }

    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return false;
    }

    std::string response;

    std::string authorization =
        "Authorization: Bearer " +
        token;

    struct curl_slist* headers =
        nullptr;

    headers =
        curl_slist_append(
            headers,
            authorization.c_str());

    headers =
        curl_slist_append(
            headers,
            "Accept: application/json");

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str());

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        "");

    curl_easy_setopt(
        curl,
        CURLOPT_USERAGENT,
        "ChessR36S/0.1");

    curl_easy_setopt(
        curl,
        CURLOPT_NOSIGNAL,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback);

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_OPTIONS,
        CURLSSLOPT_NATIVE_CA);

    curl_easy_setopt(
        curl,
        CURLOPT_CAINFO,
        "external/curl/bin/curl-ca-bundle.crt");

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYPEER,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_SSL_VERIFYHOST,
        2L);

    CURLcode result =
        curl_easy_perform(
            curl);

    long httpCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpCode);

    curl_slist_free_all(
        headers);

    curl_easy_cleanup(
        curl);

    if (
        result != CURLE_OK)
    {
        lastError =
            curl_easy_strerror(
                result);

        return false;
    }

    if (
        httpCode < 200 ||
        httpCode >= 300)
    {
        lastError =
            "Lichess HTTP error " +
            std::to_string(
                httpCode);

        return false;
    }

    return true;
}

bool LichessClient::acceptIncomingChallenge()
{
    LichessChallenge challenge =
        getIncomingChallenge();

    if (
        !challenge.active ||
        challenge.id.empty())
    {
        return false;
    }

    std::string url =
        "https://lichess.org/api/challenge/" +
        challenge.id +
        "/accept";

    return
        performAuthenticatedPost(
            url);
}

bool LichessClient::declineIncomingChallenge()
{
    LichessChallenge challenge =
        getIncomingChallenge();

    if (
        !challenge.active ||
        challenge.id.empty())
    {
        return false;
    }

    std::string url =
        "https://lichess.org/api/challenge/" +
        challenge.id +
        "/decline";

    bool result =
        performAuthenticatedPost(
            url);

    if (result)
    {
        std::lock_guard<std::mutex>
            lock(eventMutex);

        incomingChallenge =
            LichessChallenge{};
    }

    return result;
}

bool LichessClient::sendMove(
    const std::string& gameId,
    const std::string& move)
{
    if (
        gameId.empty() ||
        move.empty())
    {
        lastError =
            "Invalid game ID or move.";

        return false;
    }

    std::string url =
        "https://lichess.org/api/board/game/" +
        gameId +
        "/move/" +
        move;

    bool success =
        performAuthenticatedPost(
            url);

    if (!success)
    {
        std::cerr
            << "[LICHESS] Move rejected: "
            << move
            << " - "
            << lastError
            << std::endl;

        return false;
    }

    std::cout
        << "[LICHESS] Move sent: "
        << move
        << std::endl;

    return true;
}

bool LichessClient::resignGame(
    const std::string& gameId)
{
    if (gameId.empty())
    {
        lastError =
            "No active Lichess game.";

        return false;
    }

    std::string url =
        "https://lichess.org/api/board/game/" +
        gameId +
        "/resign";

    bool success =
        performAuthenticatedPost(
            url);

    if (success)
    {
        std::cout
            << "[LICHESS] Game resigned."
            << std::endl;
    }
    else
    {
        std::cerr
            << "[LICHESS] Could not resign: "
            << lastError
            << std::endl;
    }

    return success;
}