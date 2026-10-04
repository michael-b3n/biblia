#pragma once

// sol2 reads its configuration while being included, every translation unit has to see the same one.
#if defined(SOL_HPP)
  #error "sol2 was included without its configuration, include bibstd/lua/sol.hpp instead of <sol/sol.hpp>"
#endif

// Scripts are user input, a wrong type raises an error instead of crashing.
#define SOL_ALL_SAFETIES_ON 1

// Errors reach the caller, the safeties would print them to std::cerr as well.
#define SOL_PRINT_ERRORS 0

// The MSYS2 Lua is built as C, its headers need C linkage.
#define SOL_USING_CXX_LUA 0

#include <sol/sol.hpp>
