#
# Set the mingw root path variable named by \p mingw_root_dir, the parent of the compiler's bin folder.
#
function(set_mingw_path mingw_root_dir)
  cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH MINGW_BIN_DIRECTORY)
  cmake_path(GET MINGW_BIN_DIRECTORY PARENT_PATH MINGW_ROOT_DIRECTORY)
  set(${mingw_root_dir} "${MINGW_ROOT_DIRECTORY}" PARENT_SCOPE)
endfunction(set_mingw_path)
