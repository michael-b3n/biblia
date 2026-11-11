#pragma once

#include <string>
#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include "icon.hpp"
  #include <windows.h>

namespace Tray
{

///
/// Windows image class.
///
class Image
{
  HBITMAP image;

public:
  Image(HBITMAP image);
  Image(WORD resource);
  Image(const char* path);
  Image(const std::string& path);

  operator HBITMAP();
};

} // namespace Tray

#endif
