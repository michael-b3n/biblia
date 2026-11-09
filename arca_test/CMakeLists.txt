cmake_minimum_required(VERSION 4.3)

project(arca_test LANGUAGES CXX)

find_package(Catch2 3 REQUIRED)
include(Catch)

#
# The test helpers, shared with the tests of the libraries built on arca.
# Includable by their path below the test root, e.g. "test_utils/temp_folder.hpp".
#
file(GLOB ARCA_TEST_UTILS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/*.hpp")
if(WIN32)
  file(GLOB ARCA_TEST_UTILS_WINDOWS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/windows/*.cpp")
  list(APPEND ARCA_TEST_UTILS_FILES ${ARCA_TEST_UTILS_WINDOWS_FILES})
endif()

add_library(arca_test_utils STATIC)
target_sources(arca_test_utils
  PRIVATE ${ARCA_TEST_UTILS_FILES}
)
target_include_directories(arca_test_utils
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(arca_test_utils
  PUBLIC Catch2::Catch2
  PUBLIC arca
)

#
# Define target sources
#
file(GLOB ARCA_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")

# Include OS specific sources
if(WIN32)
  file(GLOB ARCA_TEST_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  list(APPEND ARCA_TEST_SRC_FILES ${ARCA_TEST_WINDOWS_SRC_FILES})
endif()
list(FILTER ARCA_TEST_SRC_FILES EXCLUDE REGEX "/test_utils/")

#
# Set executable.
#
add_executable(arca_test)

#
# Set target sources.
#
target_sources(arca_test
  PRIVATE ${ARCA_TEST_SRC_FILES}
)

#
# Link libaries.
#
target_link_libraries(
  arca_test
  PRIVATE
  Catch2::Catch2WithMain
  arca_test_utils
  arca)

#
# Register the Catch2 test cases with CTest, so they can be run via `ctest` in the build directory.
#
catch_discover_tests(arca_test)
