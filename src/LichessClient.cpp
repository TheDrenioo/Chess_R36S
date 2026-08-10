#include "LichessClient.h"

#include <cstdlib>
#include <iostream>

LichessClient::LichessClient()
{
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

    CURLcode result =
        curl_easy_perform(
            curl);

    if (result != CURLE_OK)
    {
        lastError =
            curl_easy_strerror(
                result);

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