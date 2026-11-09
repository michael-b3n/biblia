#pragma once

#include <string_view>

namespace bibstd::util::url
{

///
/// \return true if \p url is a valid url of a web page, one of http or https
///
[[nodiscard]] auto is_web_url(std::string_view url) -> bool;

} // namespace bibstd::util::url
