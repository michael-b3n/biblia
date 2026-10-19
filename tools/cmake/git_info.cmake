#
# Set the git information variables named by \p git_sha1 and \p git_date.
#
function(generate_git_info git_sha1 git_date)
  find_package(Git REQUIRED)

  # the commit's SHA1, and whether the building assistant was dirty or not
  execute_process(COMMAND
    ${GIT_EXECUTABLE} describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA1
    COMMAND_ERROR_IS_FATAL ANY OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(${git_sha1} "${GIT_SHA1}" PARENT_SCOPE)

  # the date of the commit
  execute_process(COMMAND
    ${GIT_EXECUTABLE} log -1 --format=%ad --date=local
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_DATE
    COMMAND_ERROR_IS_FATAL ANY OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(${git_date} "${GIT_DATE}" PARENT_SCOPE)
endfunction(generate_git_info)
