cmake_minimum_required(VERSION 4.3)

project(beman_execution VERSION 0.3.0)

#
# Vendored copy of https://github.com/bemanproject/execution at 8e358c0, the headers only.
# Update by replacing include/beman with the headers of the wanted revision.
#
# Local changes, to be kept or dropped with care on an update:
# - detail/stop_when.hpp no longer prints "stop_when: stop requested" to stdout.
# - detail/inplace_stop_source.hpp registers a callback under the lock. Without it a callback registered
#   while the stop is requested is lost, bibstd_test "stop_callback_registered_while_the_stop_is_requested_is_called".
#
add_library(beman_execution INTERFACE)

target_compile_features(beman_execution INTERFACE cxx_std_23)
target_include_directories(beman_execution SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# The thread pool of the library deletes with size and alignment, which clang leaves off for MinGW.
target_compile_options(beman_execution
  INTERFACE $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
)

find_package(Threads REQUIRED)
target_link_libraries(beman_execution INTERFACE Threads::Threads)

add_library(beman::execution ALIAS beman_execution)
