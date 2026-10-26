#
# Generate the script reference \p output with tools/gen_lua_doc.lua from the paths given after \p update, relative to
# the source root: folders searched for C++ and Lua sources, and single files. With \p update the build regenerates it,
# the test lua_doc checks that it is up to date.
#
function(add_lua_doc output update)
  find_program(LUA_EXECUTABLE NAMES lua5.4 lua REQUIRED)
  unset(sources)
  foreach(path ${ARGN})
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${path}")
      file(GLOB_RECURSE found CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/${path}/*.cpp" "${CMAKE_SOURCE_DIR}/${path}/*.hpp" "${CMAKE_SOURCE_DIR}/${path}/*.lua"
      )
      # Not in the repository, the doc would differ between machines
      list(FILTER found EXCLUDE REGEX "/private/")
      list(APPEND sources ${found})
    else()
      list(APPEND sources ${path})
    endif()
  endforeach()

  # A file of the sources, as arguments they would be longer than a Windows command line may be
  list(JOIN sources "\n" source_lines)
  set(source_file "${CMAKE_CURRENT_BINARY_DIR}/lua_doc_sources.txt")
  file(CONFIGURE OUTPUT "${source_file}" CONTENT "${source_lines}\n" @ONLY)
  set(command ${LUA_EXECUTABLE} tools/gen_lua_doc.lua)

  if(update)
    list(TRANSFORM sources PREPEND "${CMAKE_SOURCE_DIR}/" OUTPUT_VARIABLE depends)
    add_custom_command(
      OUTPUT "${CMAKE_SOURCE_DIR}/${output}"
      COMMAND ${command} ${output} "@${source_file}"
      DEPENDS "${CMAKE_SOURCE_DIR}/tools/gen_lua_doc.lua" ${depends}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Generating ${output}"
      VERBATIM
    )
    add_custom_target(lua_doc ALL DEPENDS "${CMAKE_SOURCE_DIR}/${output}")
  endif()
  add_test(NAME lua_doc COMMAND ${command} --check ${output} "@${source_file}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction(add_lua_doc)
