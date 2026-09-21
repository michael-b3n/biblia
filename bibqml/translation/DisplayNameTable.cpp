#include "bibqml/translation/DisplayNameTable.hpp"

#include <bibstd/io/csv_reader.hpp>
#include <bibstd/util/exception.hpp>
#include <bibstd/util/log.hpp>
#include <bibstd/util/ranges.hpp>

#include <algorithm>
#include <format>
#include <ranges>

namespace bibqml
{
namespace
{

// Index of the column holding the keys. All other columns hold display names.
constexpr std::size_t key_column_index = 0;

// Name of the key column. Without it a document that lost its header would have its first
// entry read as the languages.
constexpr auto key_column_name = std::string_view{"key"};

///
/// A language is found by its name, so an unnamed or a repeated one could never be reached.
/// \throws util::exception if the columns do not describe the languages of a display names table
/// \return language of every column behind the key column
///
[[nodiscard]] auto read_languages(const std::vector<std::string>& columns) -> std::vector<std::string>
{
  if(columns.size() <= key_column_index || columns.at(key_column_index) != key_column_name)
  {
    throw bibstd::util::exception(
      std::format(
        "display names document is not headed by the key column: columns={}, first=\"{}\"",
        columns.size(),
        columns.empty() ? std::string{} : columns.at(key_column_index)
      )
    );
  }
  if(columns.size() <= key_column_index + 1)
  {
    throw bibstd::util::exception(std::format("display names document holds no language column: columns={}", columns.size()));
  }
  auto languages = columns | std::views::drop(key_column_index + 1) | std::ranges::to<std::vector>();
  if(std::ranges::any_of(languages, [](const auto& language) { return language.empty(); }))
  {
    throw bibstd::util::exception("display names document holds an unnamed language column");
  }
  auto sorted = languages;
  std::ranges::sort(sorted);
  if(const auto duplicate = std::ranges::adjacent_find(sorted); duplicate != std::ranges::cend(sorted))
  {
    throw bibstd::util::exception(std::format("display names document names a language twice: language=\"{}\"", *duplicate));
  }
  return languages;
}

} // anonymous namespace

///
///
DisplayNameTable::DisplayNameTable(const std::span<const std::byte> csv)
{
  if(csv.empty())
  {
    return;
  }
  const auto reader = bibstd::io::csv_reader{
    csv, bibstd::io::csv_reader::params{.skip_comment_lines = true, .skip_empty_lines = true}
  };
  const auto columns = reader.column_names();
  languages_ = read_languages(columns);

  std::ranges::for_each(
    bibstd::util::ranges::index_view_to(reader.row_count()),
    [&](const auto row)
    {
      auto cells = reader.row(row);
      if(cells.size() != columns.size())
      {
        LOG_ERROR("skip display names row: unexpected cell count: row={}, cells={}", row, cells.size());
        return;
      }
      auto key = std::move(cells.at(key_column_index));
      if(key.empty())
      {
        LOG_ERROR("skip display names row: empty key: row={}", row);
        return;
      }
      auto names = std::move(cells) | std::views::drop(key_column_index + 1) | std::ranges::to<std::vector>();
      const auto [it, inserted] = entries_.try_emplace(std::move(key), std::move(names));
      if(!inserted)
      {
        LOG_ERROR("duplicate display names key: row={}, key=\"{}\"", row, it->first);
      }
    }
  );
  LOG_INFO("display names loaded: languages={}, keys={}", languages_.size(), entries_.size());
}

///
///
auto DisplayNameTable::languages() const -> const std::vector<std::string>&
{
  return languages_;
}

///
///
auto DisplayNameTable::name(const std::string_view language, const std::string_view key) const -> std::optional<std::string>
{
  const auto language_it = std::ranges::find(languages_, language);
  if(language_it == std::ranges::cend(languages_))
  {
    return std::nullopt;
  }
  const auto entry_it = entries_.find(std::string{key});
  if(entry_it == std::ranges::cend(entries_))
  {
    return std::nullopt;
  }
  const auto index = static_cast<std::size_t>(std::ranges::distance(std::ranges::cbegin(languages_), language_it));
  if(index >= entry_it->second.size() || entry_it->second.at(index).empty())
  {
    return std::nullopt;
  }
  return entry_it->second.at(index);
}

} // namespace bibqml
