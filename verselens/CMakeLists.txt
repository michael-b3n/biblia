cmake_minimum_required(VERSION 4.3)

#
# Include cmake functions
#
include(helper_functions)

#
# Define app information
# The release workflow requires the tag v<major>.<minor> to match this version.
#
set(APP_VERSION_MAJOR 2)
set(APP_VERSION_MINOR 5)
set(APP_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}")
set(APP_EXE_NAME "verselens")
set(APP_NAME "VerseLens")
set(APP_REPOSITORY_URL "https://github.com/michael-b3n/biblia")
set(APP_DATA_FOLDER_NAME "verselens")
set(APP_FILE_DESCRIPTION "${APP_NAME}")
message(STATUS "project name: " ${APP_EXE_NAME})
message(STATUS "project version: " ${APP_VERSION})

#
# The MSIX package gets its updates from the store, it is built without the Velopack updater.
# Velopack is set up for windows only, elsewhere the option is off.
# Only this app uses Velopack, so it adds the library itself and skips its download without it.
#
include(CMakeDependentOption)
cmake_dependent_option(VERSELENS_VELOPACK "Build the Velopack installer hooks and updater into VerseLens" ON "WIN32" OFF)
message(STATUS "velopack: " ${VERSELENS_VELOPACK})

if(VERSELENS_VELOPACK)
  add_subdirectory(${CMAKE_SOURCE_DIR}/libs_external/velopack ${CMAKE_BINARY_DIR}/libs_external/velopack)
endif()

#
# Define resources
#
set(TESSDATA_FOLDER_NAME "tessdata")
set(TESSDATA_FOLDER_PATH "${CMAKE_CURRENT_LIST_DIR}/res/${TESSDATA_FOLDER_NAME}")
message(STATUS "tessdata folder: " ${TESSDATA_FOLDER_PATH})

#
# Generate git information
#
generate_git_info(GIT_SHA1 GIT_DATE)
message(STATUS "git sha1: " ${GIT_SHA1})
message(STATUS "git date: " ${GIT_DATE})

if(GIT_SHA1 STREQUAL "")
  message(FATAL_ERROR "git sha1 not found")
endif()

#
# Define project
#
project(
  verselens
  VERSION ${APP_VERSION}
)

#
# Configure version files
#
configure_file(${CMAKE_CURRENT_LIST_DIR}/res/resource.rc.in ${CMAKE_CURRENT_BINARY_DIR}/res/resource.rc @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/res/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/res/version.hpp @ONLY)

#
# Define target files
#
file(GLOB ${PROJECT_NAME}_CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB ${PROJECT_NAME}_HPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.hpp")

# Include OS specific sources, the ones built on Velopack carry velopack in their name.
if(WIN32)
  file(GLOB ${PROJECT_NAME}_WINDOWS_CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/windows/*.cpp")
  file(GLOB ${PROJECT_NAME}_WINDOWS_HPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/windows/*.hpp")
  if(NOT VERSELENS_VELOPACK)
    list(FILTER ${PROJECT_NAME}_WINDOWS_CPP_FILES EXCLUDE REGEX "velopack[^/]*$")
    list(FILTER ${PROJECT_NAME}_WINDOWS_HPP_FILES EXCLUDE REGEX "velopack[^/]*$")
  endif()
  list(APPEND ${PROJECT_NAME}_CPP_FILES ${${PROJECT_NAME}_WINDOWS_CPP_FILES})
  list(APPEND ${PROJECT_NAME}_HPP_FILES ${${PROJECT_NAME}_WINDOWS_HPP_FILES})
endif()
file(GLOB_RECURSE ${PROJECT_NAME}_QML_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qml/*.qml")
file(GLOB_RECURSE ${PROJECT_NAME}_SVG_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qml/*.svg")
file(GLOB_RECURSE ${PROJECT_NAME}_QML_CPP_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qml/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_QML_HPP_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qml/*.hpp")

#
# Setup Qt6 and executable
#
set(QT_FORCE_CMP0156_TO_VALUE NEW)

find_package(${QT_VERSION_NAME} ${QT_VERSION_NUMBER} REQUIRED COMPONENTS Quick QuickControls2 QuickDialogs2 QuickShapes Svg)
qt_standard_project_setup(REQUIRES ${QT_VERSION_NUMBER})
qt_add_executable(${PROJECT_NAME} WIN32)

qt_add_qml_module(
  ${PROJECT_NAME}
  URI "ui"
  VERSION ${APP_VERSION}
  QML_FILES
  ${${PROJECT_NAME}_QML_FILES_RELATIVE}
  SOURCES
  ${${PROJECT_NAME}_QML_CPP_FILES_RELATIVE}
  ${${PROJECT_NAME}_QML_HPP_FILES_RELATIVE}
  IMPORTS
  BibQml
  DEPENDENCIES TARGET BibQml
  RESOURCES
  ${${PROJECT_NAME}_SVG_FILES_RELATIVE}
)
qt_generate_deploy_qml_app_script(
  TARGET ${PROJECT_NAME}
  OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
  NO_UNSUPPORTED_PLATFORM_ERROR
  NO_TRANSLATIONS
  NO_COMPILER_RUNTIME
)

#
# Set custom executable name.
#
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${APP_EXE_NAME})

#
# Include directories (automatically include all qml subdirectories).
#
file(GLOB ${PROJECT_NAME}_QML_SUBDIR_CANDIDATES LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/qml/*")

foreach(entry ${${PROJECT_NAME}_QML_SUBDIR_CANDIDATES})
  if(IS_DIRECTORY ${entry})
    list(APPEND ${PROJECT_NAME}_QML_SUBDIRS ${entry})
  endif()
endforeach()

target_include_directories(${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/qml
  PRIVATE ${${PROJECT_NAME}_QML_SUBDIRS}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

#
# Setup resource compiler and file.
#
set(CMAKE_RC_COMPILER_INIT windres)
enable_language(RC)
set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

#
# Set target sources.
#
target_sources(${PROJECT_NAME}
  PRIVATE ${${PROJECT_NAME}_CPP_FILES}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/res/resource.rc
)

#
# Add all resources that are compiled into the executable using incbin.
#
add_incbin_resources(src/construct_tray.cpp res/icon.ico)
add_incbin_resources(src/construct_translations.cpp res/display_names.csv)

#
# Link against external dependencies
#
target_link_libraries(${PROJECT_NAME}
  PRIVATE Qt6::Quick
  PRIVATE Qt6::QuickControls2
  PRIVATE Qt6::QuickDialogs2
  PRIVATE Qt6::QuickShapes
  PRIVATE Qt6::Svg
  PRIVATE bibstd
  PRIVATE BibQmlplugin
)

if(VERSELENS_VELOPACK)
  target_link_libraries(${PROJECT_NAME} PRIVATE velopack::velopack)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VERSELENS_VELOPACK)

  #
  # Copy the velopack dll next to the executable, so it starts from the build folder.
  # The install resolves the runtime dependency there as well, without this copy it fails.
  #
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:velopack::velopack> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endif()

#
# Configure installation
#
set_mingw_path(MINGW_ROOT_DIRECTORY)
message(STATUS "found mingw64 root directory: " ${MINGW_ROOT_DIRECTORY})

include(GNUInstallDirs)

# Start from an empty folder, only for the default prefix so a given prefix is never deleted.
install(CODE "
  if(CMAKE_INSTALL_PREFIX STREQUAL [[${CMAKE_BINARY_DIR}/install]])
    file(REMOVE_RECURSE [[${CMAKE_BINARY_DIR}/install]])
  endif()
")
install(
  TARGETS ${PROJECT_NAME}
  RUNTIME_DEPENDENCIES
  DIRECTORIES
  ${MINGW_ROOT_DIRECTORY}/bin
  PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
  POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)
install(DIRECTORY ${TESSDATA_FOLDER_PATH} DESTINATION share)
install(SCRIPT ${QT_DEPLOY_SCRIPT})
