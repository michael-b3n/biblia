#if defined(_WIN32)
  #include <core/icon.hpp>

///
///
Tray::Icon::Icon(const std::span<const std::byte> icon_buffer)
{
  // The icon file starts with 3 unsigned short (WORD) values, the third value is icon_count.
  // `icon_count` is followed by `sizeof_icon_dir_entry * icon_count` bytes, followed by the bytes for the first icon.
  constexpr auto sizeof_icon_dir_entry = 16;
  const auto short_view = std::span(reinterpret_cast<const unsigned short*>(icon_buffer.data()), icon_buffer.size() / sizeof(unsigned short));
  if(short_view.size() > 3)
  {
    const unsigned short icon_count = short_view[2];
    const int offset = 3 * sizeof(unsigned short) + sizeof_icon_dir_entry * icon_count;
    [[maybe_unused]] const auto data = reinterpret_cast<const unsigned char*>(icon_buffer.data());
    if(offset != 0)
    {
      hIcon = CreateIconFromResource(const_cast<unsigned char*>(data) + offset, icon_buffer.size() - offset, true, 0x30000);
    }
  }
}

///
///
Tray::Icon::Icon(HICON icon)
  : hIcon(icon)
{
}

///
///
Tray::Icon::operator HICON()
{
  return hIcon;
}

#endif
