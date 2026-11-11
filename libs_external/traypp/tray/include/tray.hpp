#pragma once

#include <components/button.hpp>
#include <components/imagebutton.hpp>
#include <components/label.hpp>
#include <components/separator.hpp>
#include <components/syncedtoggle.hpp>
#include <components/toggle.hpp>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <core/tray.hpp>
#endif
