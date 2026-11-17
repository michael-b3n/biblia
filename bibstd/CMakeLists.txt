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
# Compile the Lua scripts of lua/bundled and lua/private into the library, a new script needs no other change.
#
include(embedded_scripts)
file(GLOB BIBSTD_LUA_BUNDLED CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/bundled/*.lua" "${CMAKE_CURRENT_LIST_DIR}/lua/private/*.lua")
configure_embedded_scripts(${CMAKE_CURRENT_BINARY_DIR}/lua/bundled_scripts.cpp
  HEADER bibstd/lua/bundled_scripts.hpp
  NAMESPACE bibstd::lua::bundled
  SCRIPTS ${BIBSTD_LUA_BUNDLED}
)
target_sources(${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lua/bundled_scripts.cpp
)

find_package(pugixml REQUIRED)

#
# Link libraries.
#
target_link_libraries(${PROJECT_NAME}
  PUBLIC arcstd
  PUBLIC bibocr
  PUBLIC tray::tray
  PUBLIC sfsm::sfsm
  PRIVATE pugixml::pugixml
)

if(WIN32)
  # open_browser starts the browser through COM
  target_link_libraries(${PROJECT_NAME} PRIVATE ole32 oleaut32)
endif()

#
# Target compile definitions.
#
if(BIBSTD_DEBUG_DATA)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BIBSTD_DEBUG_DATA)
endif()

#
# Add target include dirs.
#
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH BIBSTD_INCLUDE_DIR)
target_include_directories(${PROJECT_NAME} PUBLIC ${BIBSTD_INCLUDE_DIR})

#
# The script reference doc/lua_api.md, from the comments tagged \lua in all app sources.
# CI turns the update off, so the test fails for a doc not committed with its sources.
#
include(lua_doc)
option(BIBSTD_LUA_DOC_UPDATE "Regenerate doc/lua_api.md while building" ON)
add_lua_doc(doc/lua_api.md ${BIBSTD_LUA_DOC_UPDATE} arca/arcstd arca/arcqml bibocr bibstd bibqml verselens)
