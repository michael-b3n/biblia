cmake_minimum_required(VERSION 4.3)

project(BibQml)

#
# Define target sources and headers, the qml files and icons are in ArcQml.
#
file(GLOB_RECURSE ${PROJECT_NAME}_CPP_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_HPP_FILES_RELATIVE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

#
# Setup Qt6 and library
#
set(QT_FORCE_CMP0156_TO_VALUE NEW)

find_package(${QT_VERSION_NAME} ${QT_VERSION_NUMBER} REQUIRED COMPONENTS Quick QuickControls2)
qt_standard_project_setup(REQUIRES ${QT_VERSION_NUMBER})

#
# Create QML module library with plugin
#
qt_add_library(${PROJECT_NAME} STATIC)

qt_add_qml_module(
  ${PROJECT_NAME}
  URI "BibQml"
  OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/BibQml"
  SOURCES
  ${${PROJECT_NAME}_CPP_FILES_RELATIVE}
  ${${PROJECT_NAME}_HPP_FILES_RELATIVE}
  DEPENDENCIES TARGET ArcQml
)

#
# Link dependencies
#
target_link_libraries(${PROJECT_NAME}
  PUBLIC bibstd
  PUBLIC ArcQml
  PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Gui
)

#
# Add target include dirs (automatically includes all immediate subdirectories).
#
file(GLOB ${PROJECT_NAME}_SUBDIR_CANDIDATES LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")

foreach(entry ${${PROJECT_NAME}_SUBDIR_CANDIDATES})
  if(IS_DIRECTORY ${entry})
    list(APPEND ${PROJECT_NAME}_SUBDIRS ${entry})
  endif()
endforeach()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${${PROJECT_NAME}_SUBDIRS})
