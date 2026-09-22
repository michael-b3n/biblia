#include "bibstd/bible/scripture.hpp"
#include "bibstd/util/log.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <vector>

namespace bibstd::bible
{

///
///
scripture::scripture(scripture_info info, book_name_map_type book_name_data, passage_map_type passage_data)
  : info_{std::move(info)}
  , book_name_data_{std::move(book_name_data)}
  , passage_data_{std::move(passage_data)}
  , versification_{[&]
                   {
                     auto view = passage_data_ | std::views::keys;
                     const auto v = bible::versification{info_.name, view | std::ranges::to<std::vector>()};
                     const auto it = std::ranges::find(versifications_default, v);
                     return it != std::ranges::cend(versifications_default) ? *it : v;
                   }()}
{
}

///
///
auto scripture::information() const -> const scripture_info&
{
  return info_;
}

///
///
auto scripture::book_information(const book_id book) const -> std::optional<book_name>
{
  const auto it = book_name_data_.find(book);
  return it != std::cend(book_name_data_) ? std::make_optional(it->second) : std::nullopt;
}

///
///
auto scripture::passage(const reference& ref) const -> std::optional<bible::passage>
{
  const auto it = passage_data_.find(ref);
  if(it != std::cend(passage_data_))
  {
    return it->second;
  }
  LOG_ERROR("verse not found: {}", ref);
  return std::nullopt;
}

///
///
auto scripture::versification() const -> const bible::versification&
{
  return versification_;
}

} // namespace bibstd::bible
