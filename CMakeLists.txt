cmake_minimum_required(VERSION 3.20)

project(ChessR36S LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ChessR36S
    src/main.cpp
)

target_include_directories(ChessR36S PRIVATE
    C:/w64devkit/include
)

target_link_directories(ChessR36S PRIVATE
    C:/w64devkit/lib
)

target_link_libraries(ChessR36S PRIVATE
    SDL2
    SDL2_image
    winmm
    user32
    gdi32
    dxguid
)