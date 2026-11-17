cmake_minimum_required(VERSION 4.3)

project(sfsm VERSION 0.1.0 LANGUAGES CXX)

#
# Vendored copy of https://github.com/michael-b3n/SFSM at 7e63fa0.
# Update by replacing include/sfsm with the headers of the wanted revision.
#
add_library(sfsm INTERFACE)
target_compile_features(sfsm INTERFACE cxx_std_20)
target_include_directories(sfsm SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(sfsm::sfsm ALIAS sfsm)
