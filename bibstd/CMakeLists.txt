cmake_minimum_required(VERSION 4.3)

project(bibstd)
add_library(${PROJECT_NAME} STATIC)

#
# Debugging setting: if ON exports debug data during runtime.
#
option(BIBSTD_DEBUG_DATA "Export debug data during runtime" OFF)

#
# Define target sources
#
file(GLOB BIBSTD_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
file(GLOB BIBSTD_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.hpp")

# Include OS specific sources and headers
if(WIN32)
  file(GLOB BIBSTD_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  file(GLOB BIBSTD_WINDOWS_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.hpp")
  list(APPEND BIBSTD_SRC_FILES ${BIBSTD_WINDOWS_SRC_FILES})
  list(APPEND BIBSTD_INC_FILES ${BIBSTD_WINDOWS_INC_FILES})
endif()

target_sources(${PROJECT_NAME}
  PRIVATE ${BIBSTD_SRC_FILES} ${BIBSTD_INC_FILES}
)

#
# Compile the Lua scripts into the library, a new script in lua/internal or lua/bundled needs no other change.
# The label is the namespace of the scripts, \see lua/embedded_scripts.hpp.
#
include(helper_functions)
file(GLOB BIBSTD_LUA_INTERNAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/internal/*.lua")
file(GLOB BIBSTD_LUA_BUNDLED CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/bundled/*.lua" "${CMAKE_CURRENT_LIST_DIR}/lua/private/*.lua")
configure_incbin_source(internal lua/embedded_scripts.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp ${BIBSTD_LUA_INTERNAL})
configure_incbin_source(bundled lua/embedded_scripts.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/lua/bundled.cpp ${BIBSTD_LUA_BUNDLED})
target_sources(${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp ${CMAKE_CURRENT_BINARY_DIR}/lua/bundled.cpp
)

find_package(Boost REQUIRED COMPONENTS filesystem url)
find_package(spdlog REQUIRED)
find_package(pugixml REQUIRED)
find_package(libzip REQUIRED)
find_package(sol2 3.5 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
pkg_search_module(LEPTONICA REQUIRED IMPORTED_TARGET lept)
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
  PUBLIC pugixml::pugixml
  PUBLIC libzip::zip
  PUBLIC PkgConfig::TESSERACT
  PUBLIC PkgConfig::LEPTONICA
  PUBLIC PkgConfig::LIBCURL
  PUBLIC PkgConfig::LUA
  PRIVATE PkgConfig::SQLITE
  PUBLIC sol2::sol2
  PUBLIC libs_external::all
  PUBLIC stdc++exp
)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE mswsock)

  # Windows OCR and the Windows.Graphics.Capture screen capture, both built on C++/WinRT
  target_link_libraries(${PROJECT_NAME} PRIVATE runtimeobject ole32 oleaut32 d3d11 dxgi)

  # The C++/WinRT headers hand objects across the ABI by reading them through void**.
  # GCC is allowed to reorder around that from -O2 on and does, leading to crashes.
  # Every source built on those headers carries winrt in its name and is compiled without
  # the strict aliasing assumption.
  file(GLOB BIBSTD_WINRT_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*winrt*.cpp")
  set_source_files_properties(${BIBSTD_WINRT_SRC_FILES} PROPERTIES COMPILE_OPTIONS "-fno-strict-aliasing")
endif()

#
# Target compile definitions.
#
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:BIBSTD_DEBUG>)

if(BIBSTD_DEBUG_DATA)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BIBSTD_DEBUG_DATA)
endif()

if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
endif()

#
# Add target include dirs.
#
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH BIBSTD_INCLUDE_DIR)
target_include_directories(${PROJECT_NAME} PUBLIC ${BIBSTD_INCLUDE_DIR})

#
# The script reference doc/lua_api.md, generated from the comments tagged \lua, \see tools/gen_lua_doc.lua.
# CI turns the update off, so the test fails for a doc not committed with its sources.
#
option(BIBSTD_LUA_DOC_UPDATE "Regenerate doc/lua_api.md while building" ON)
find_program(BIBSTD_LUA_EXECUTABLE NAMES lua5.4 lua REQUIRED)
file(GLOB BIBSTD_LUA_DOC_SOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/workflow/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/workflow/*.hpp"
)
list(APPEND BIBSTD_LUA_DOC_SOURCES bibstd/lua/names.hpp bibstd/lua/state.hpp bibstd/lua/internal/interface_util.lua)

if(BIBSTD_LUA_DOC_UPDATE)
  list(TRANSFORM BIBSTD_LUA_DOC_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/" OUTPUT_VARIABLE BIBSTD_LUA_DOC_DEPENDS)
  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/doc/lua_api.md"
    COMMAND ${BIBSTD_LUA_EXECUTABLE} tools/gen_lua_doc.lua doc/lua_api.md ${BIBSTD_LUA_DOC_SOURCES}
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/gen_lua_doc.lua" ${BIBSTD_LUA_DOC_DEPENDS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating doc/lua_api.md"
    VERBATIM
  )
  add_custom_target(lua_doc ALL DEPENDS "${CMAKE_SOURCE_DIR}/doc/lua_api.md")
endif()

add_test(
  NAME lua_doc
  COMMAND ${BIBSTD_LUA_EXECUTABLE} tools/gen_lua_doc.lua --check doc/lua_api.md ${BIBSTD_LUA_DOC_SOURCES}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
