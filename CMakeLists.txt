cmake_minimum_required(VERSION 4.3)

#
# Set global CMake variables.
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")

#
# Limit CMake to only provide "Release" and "Debug" others are not needed.
# see https://cmake.org/cmake/help/latest/variable/CMAKE_CONFIGURATION_TYPES.html
#
set(CMAKE_CONFIGURATION_TYPES Release Debug)

project(biblia)

# An empty build type builds without optimization and debug info.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
  message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set, configure with a preset, e.g. `cmake --preset gcc-release --fresh`")
endif()

#
# Install into the build folder unless another prefix is given, a release is packed from there.
#
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install directory" FORCE)
endif()

#
#
#
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Main Qt Versions
#
set(QT_VERSION_NAME Qt6)
set(QT_VERSION_NUMBER 6.11)

#
# Sets compile flag defaults.
#
add_compile_options(-Wall -Wpedantic)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Source paths in __FILE__ and std::source_location are kept relative.
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=>)

# GNU ld overflows REL32 relocations linking clang objects against the GCC-built mingw libraries.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_LINKER_TYPE LLD)
endif()

#
# Force a CMake reconfiguration when the .git/index changes.
# This is needed so the git-commit-hash is updated correctly.
#
set_property(GLOBAL APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/.git/index"
)

#
# Include all subdirectories.
#
enable_testing()
add_subdirectory(libs_external)
add_subdirectory(bibstd)
add_subdirectory(bibqml)
add_subdirectory(bibstd_test)
add_subdirectory(verselens)
