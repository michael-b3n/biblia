cmake_minimum_required(VERSION 4.3)

project(arcstd_test LANGUAGES CXX)

find_package(Catch2 3 REQUIRED)
include(Catch)

#
# The test helpers, shared with the tests of the libraries built on arcstd.
# Includable by their path below the test root, e.g. "test_utils/temp_folder.hpp".
#
file(GLOB ARCSTD_TEST_UTILS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/*.hpp")
if(WIN32)
  file(GLOB ARCSTD_TEST_UTILS_WINDOWS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/windows/*.cpp")
  list(APPEND ARCSTD_TEST_UTILS_FILES ${ARCSTD_TEST_UTILS_WINDOWS_FILES})
endif()

add_library(arcstd_test_utils STATIC)
target_sources(arcstd_test_utils
  PRIVATE ${ARCSTD_TEST_UTILS_FILES}
)
target_include_directories(arcstd_test_utils
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(arcstd_test_utils
  PUBLIC Catch2::Catch2
  PUBLIC arcstd
)

#
# Define target sources
#
file(GLOB ARCSTD_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")

# Include OS specific sources
if(WIN32)
  file(GLOB ARCSTD_TEST_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  list(APPEND ARCSTD_TEST_SRC_FILES ${ARCSTD_TEST_WINDOWS_SRC_FILES})
endif()
list(FILTER ARCSTD_TEST_SRC_FILES EXCLUDE REGEX "/test_utils/")

#
# Set executable.
#
add_executable(arcstd_test)

#
# Set target sources.
#
target_sources(arcstd_test
  PRIVATE ${ARCSTD_TEST_SRC_FILES}
)

#
# Link libaries.
#
target_link_libraries(
  arcstd_test
  PRIVATE
  Catch2::Catch2WithMain
  arcstd_test_utils
  arcstd)

#
# Register the Catch2 test cases with CTest, so they can be run via `ctest` in the build directory.
#
catch_discover_tests(arcstd_test)
