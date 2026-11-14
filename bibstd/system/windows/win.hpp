#pragma once

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#undef _WIN32_WINNT
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _WIN32_WINNT 0x0A00 // Windows 10
#undef WINVER
#define WINVER 0x0A00 // Windows 10
#include <windows.h>
