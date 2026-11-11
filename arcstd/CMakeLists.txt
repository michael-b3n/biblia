cmake_minimum_required(VERSION 4.3)

project(arcstd)
add_library(${PROJECT_NAME} STATIC)

#
# Define target sources
#
file(GLOB ARCSTD_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
file(GLOB ARCSTD_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.hpp")

# Include OS specific sources and headers
if(WIN32)
  file(GLOB ARCSTD_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  file(GLOB ARCSTD_WINDOWS_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.hpp")
  list(APPEND ARCSTD_SRC_FILES ${ARCSTD_WINDOWS_SRC_FILES})
  list(APPEND ARCSTD_INC_FILES ${ARCSTD_WINDOWS_INC_FILES})
endif()

target_sources(${PROJECT_NAME}
  PRIVATE ${ARCSTD_SRC_FILES} ${ARCSTD_INC_FILES}
)

#
# Compile the Lua scripts of lua/internal into the library, a new script needs no other change.
# The label is the namespace of the scripts, \see lua/embedded_scripts.hpp.
#
include(incbin)
file(GLOB ARCSTD_LUA_INTERNAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/internal/*.lua")
set(INCBIN_HEADER arcstd/lua/embedded_scripts.hpp)
configure_incbin_source(internal lua/embedded_scripts.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp ${ARCSTD_LUA_INTERNAL})
target_sources(${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp
)

find_package(Boost REQUIRED COMPONENTS filesystem url)
find_package(spdlog REQUIRED)
find_package(libzip REQUIRED)
find_package(sol2 3.5 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
# sol2 3.5 is made for Lua 5.1 to 5.4, the unversioned mingw package already ships 5.5.
pkg_search_module(LUA REQUIRED IMPORTED_TARGET lua5.4)
pkg_search_module(SQLITE REQUIRED IMPORTED_TARGET sqlite3)

#
# Link libraries.
#
target_link_libraries(${PROJECT_NAME}
  PUBLIC Boost::headers
  PUBLIC Boost::filesystem
  PUBLIC Boost::url
  PUBLIC spdlog::spdlog
  PUBLIC libzip::zip
  PUBLIC PkgConfig::LIBCURL
  PUBLIC PkgConfig::LUA
  PRIVATE PkgConfig::SQLITE
  PUBLIC sol2::sol2
  PUBLIC libs_external::all
  PUBLIC stdc++exp
)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE mswsock ole32 oleaut32)
endif()

#
# Target compile definitions.
#
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:BIBSTD_DEBUG>)

if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
endif()

#
# Add target include dirs.
#
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ARCSTD_INCLUDE_DIR)
target_include_directories(${PROJECT_NAME} PUBLIC ${ARCSTD_INCLUDE_DIR})
