#include "arcstd/util/url.hpp"

#include <boost/url.hpp>

namespace arcstd::util::url
{

///
///
auto is_web_url(const std::string_view url) -> bool
{
  const auto parsed = boost::urls::parse_uri(url);
  return parsed && (parsed->scheme_id() == boost::urls::scheme::http || parsed->scheme_id() == boost::urls::scheme::https);
}

} // namespace arcstd::util::url
