#include "bibstd/util/path.hpp"
#include "bibstd/util/string.hpp"

#include <algorithm>
#include <string_view>

namespace bibstd::util
{

///
///
auto path::string() const -> std::string
{
  return util::string::join(sections_, delimiter);
}

///
///
auto path::empty() const -> bool
{
  return sections_.empty();
}

///
///
auto path::sections() const -> const sections_type&
{
  return sections_;
}

///
///
auto path::contains(const path& other) const -> bool
{
  return std::ranges::starts_with(other.sections_, sections_);
}

///
///
auto path::normalize(const std::string_view p) -> sections_type
{
  auto sections = util::string::split(p, delimiter);
  std::erase_if(sections, [](const auto& s) { return s.empty(); });
  return sections;
}

} // namespace bibstd::util
