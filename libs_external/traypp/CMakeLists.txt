cmake_minimum_required(VERSION 4.3)
project(tray VERSION 0.2)

file(GLOB src
    "tray/src/*.cpp"
    "tray/src/*/*.cpp"
    "tray/src/*/*/*.cpp"
)

add_library(tray STATIC ${src})

target_include_directories(tray SYSTEM PUBLIC "tray/include")

set_target_properties(tray PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON)

add_library(tray::tray ALIAS tray)
