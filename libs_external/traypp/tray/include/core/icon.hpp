#pragma once

#include <cstddef>
#include <span>
#include <string>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>

namespace Tray
{

///
/// Windows icon class.
///
class Icon
{
  HICON hIcon;

public:
  Icon(std::span<const std::byte> icon_buffer);
  Icon(HICON icon);

  operator HICON();
};

} // namespace Tray

#endif
