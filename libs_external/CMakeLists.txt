cmake_minimum_required(VERSION 4.3)

project(libs_external)

add_subdirectory(beman_execution)
add_subdirectory(incbin)
add_subdirectory(magic_enum)
add_subdirectory(rapidcsv)
add_subdirectory(sfsm)
add_subdirectory(traypp)

add_library(libs_external_all INTERFACE)
target_link_libraries(libs_external_all
  INTERFACE
  beman::execution
  incbin::incbin
  magic_enum::magic_enum
  rapidcsv::rapidcsv
  sfsm::sfsm
  tray::tray
)

add_library(libs_external::all ALIAS libs_external_all)
