cmake_minimum_required(VERSION 4.3)

project(bibocr)
add_library(${PROJECT_NAME} STATIC)

#
# Define target sources
#
file(GLOB BIBOCR_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
file(GLOB BIBOCR_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.hpp")

# Include OS specific sources and headers
if(WIN32)
  file(GLOB BIBOCR_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  file(GLOB BIBOCR_WINDOWS_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.hpp")
  list(APPEND BIBOCR_SRC_FILES ${BIBOCR_WINDOWS_SRC_FILES})
  list(APPEND BIBOCR_INC_FILES ${BIBOCR_WINDOWS_INC_FILES})
endif()

target_sources(${PROJECT_NAME}
  PRIVATE ${BIBOCR_SRC_FILES} ${BIBOCR_INC_FILES}
)

find_package(PkgConfig REQUIRED)
pkg_search_module(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
pkg_search_module(LEPTONICA REQUIRED IMPORTED_TARGET lept)

#
# Link libraries.
#
target_link_libraries(${PROJECT_NAME}
  PUBLIC arca
  PUBLIC PkgConfig::TESSERACT
  PUBLIC PkgConfig::LEPTONICA
)

if(WIN32)
  # Windows OCR and the Windows.Graphics.Capture screen capture, both built on C++/WinRT
  target_link_libraries(${PROJECT_NAME} PRIVATE runtimeobject ole32 oleaut32 d3d11 dxgi)

  # The C++/WinRT headers hand objects across the ABI by reading them through void**.
  # GCC is allowed to reorder around that from -O2 on and does, leading to crashes.
  # Every source built on those headers carries winrt in its name and is compiled without
  # the strict aliasing assumption.
  file(GLOB BIBOCR_WINRT_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*winrt*.cpp")
  set_source_files_properties(${BIBOCR_WINRT_SRC_FILES} PROPERTIES COMPILE_OPTIONS "-fno-strict-aliasing")
endif()

#
# Add target include dirs.
#
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH BIBOCR_INCLUDE_DIR)
target_include_directories(${PROJECT_NAME} PUBLIC ${BIBOCR_INCLUDE_DIR})
