cmake_minimum_required(VERSION 3.16)

project(ChessR36S LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    ChessR36S

    src/main.cpp
    src/ChessGame.cpp
    src/Renderer.cpp
    src/AudioManager.cpp
    src/StockfishEngine.cpp
    src/Menu.cpp
    src/ChessClock.cpp
    src/LichessClient.cpp
)

target_include_directories(
    ChessR36S
    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/src

    C:/w64devkit/include/SDL2

    ${CMAKE_CURRENT_SOURCE_DIR}/external/curl/include
)

target_link_directories(
    ChessR36S
    PRIVATE

    C:/w64devkit/lib

    ${CMAKE_CURRENT_SOURCE_DIR}/external/curl/lib
)

target_link_libraries(
    ChessR36S
    PRIVATE

    mingw32
    SDL2main
    SDL2
    SDL2_image
    SDL2_mixer
    curl
)

add_custom_command(
    TARGET ChessR36S
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different

    "${CMAKE_CURRENT_SOURCE_DIR}/external/curl/bin/libcurl-x64.dll"

    "$<TARGET_FILE_DIR:ChessR36S>/libcurl-x64.dll"
)

add_custom_command(
    TARGET ChessR36S
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_if_different

    "${CMAKE_CURRENT_SOURCE_DIR}/external/curl/bin/curl-ca-bundle.crt"

    "$<TARGET_FILE_DIR:ChessR36S>/curl-ca-bundle.crt"
)