cmake_minimum_required(VERSION 3.30)
project(LionUI C)

set(CMAKE_C_STANDARD 11)

add_executable(LionUI main.c)

target_include_directories(LionUI PRIVATE "${CMAKE_SOURCE_DIR}/Raylib/include")
target_link_directories(LionUI PRIVATE "${CMAKE_SOURCE_DIR}/Raylib/lib")
target_link_libraries(LionUI PRIVATE raylib winmm)
