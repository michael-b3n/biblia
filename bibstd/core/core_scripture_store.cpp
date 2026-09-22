#include "bibstd/core/core_scripture_store.hpp"
#include "bibstd/bible/scripture_reader.hpp"
#include "bibstd/bible/scripture_reader_usx.hpp"
#include "bibstd/io/zip_file_reader.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/string.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace bibstd::core
{
namespace
{

///
/// \return true if the given path is a zip file, false otherwise
///
auto is_zip_file(const std::filesystem::path& path) -> bool
{
  return path.extension() == std::string_view(".zip");
}

///
/// \return Container type of the given path, std::nullopt if its extension names none
///
auto container_type_of(const std::filesystem::path& path) -> std::optional<core_scripture_store::container_type>
{
  if(is_zip_file(path))
  {
    return core_scripture_store::container_type::zip;
  }
  return std::nullopt;
}

///
/// The readers the store offers a container to, in order. A newly supported scripture format is
/// added here and nowhere else.
/// \return one reader per supported scripture format
///
auto default_readers() -> std::vector<std::unique_ptr<const bible::scripture_reader>>
{
  auto result = std::vector<std::unique_ptr<const bible::scripture_reader>>{};
  result.emplace_back(std::make_unique<bible::scripture_reader_usx>());
  return result;
}

///
/// Access the files directly inside \p folder, leaving out what is not a file of its own.
/// \return view of the file paths
///
auto regular_files(const std::filesystem::path& folder, std::error_code& error) -> auto
{
  return std::filesystem::directory_iterator{folder, error} |
         std::views::filter([&error](const auto& entry) { return entry.is_regular_file(error); }) |
         std::views::transform([](const auto& entry) { return entry.path(); });
}

} // namespace

///
///
core_scripture_store::core_scripture_store(std::filesystem::path folder)
  : folder_{std::move(folder)}
  , readers_{default_readers()}
{
  load();
}

///
///
core_scripture_store::~core_scripture_store() noexcept = default;

///
///
auto core_scripture_store::scriptures() const -> const scripture_map_type&
{
  return scripture_data_;
}

///
///
auto core_scripture_store::import(const std::filesystem::path& source) -> std::size_t
{
  auto error = std::error_code{};
  if(!std::filesystem::is_directory(source, error))
  {
    LOG_WARN("scripture import folder not found: folder=\"{}\"", source.generic_string());
    return 0;
  }
  if(std::filesystem::equivalent(source, folder_, error))
  {
    return 0;
  }
  std::ignore = std::filesystem::create_directories(folder_, error);

  auto imported = std::size_t{0};
  for(const auto& file :
      regular_files(source, error) | std::views::filter([](const auto& path) { return container_type_of(path).has_value(); }))
  {
    // Read before the copy, so a file this store cannot load never reaches the folder
    auto scripture = read(file);
    if(!scripture)
    {
      LOG_WARN("scripture file not taken over: file_name=\"{}\"", file.filename().string());
      continue;
    }
    const auto target = folder_ / file.filename();
    if(!std::filesystem::copy_file(file, target, std::filesystem::copy_options::overwrite_existing, error))
    {
      LOG_ERROR("failed to copy scripture file: file_name=\"{}\", {}", file.filename().string(), error.message());
      continue;
    }
    LOG_INFO("scripture file copied: file_name=\"{}\"", file.filename().string());
    scripture_files_.insert_or_assign(target, std::move(scripture));
    ++imported;
  }
  if(imported > 0)
  {
    name_scriptures();
  }
  return imported;
}

///
///
auto core_scripture_store::load() -> void
{
  scripture_files_.clear();
  auto error = std::error_code{};
  if(!std::filesystem::exists(folder_, error))
  {
    // Created here so a later import has somewhere to copy to
    std::ignore = std::filesystem::create_directories(folder_, error);
  }
  else if(!std::filesystem::is_directory(folder_, error))
  {
    LOG_WARN("scripture folder not found: folder=\"{}\"", folder_.generic_string());
  }
  else
  {
    std::ranges::for_each(
      regular_files(folder_, error),
      [this](const auto& file)
      {
        if(!container_type_of(file))
        {
          LOG_WARN("file type not supported: file_name=\"{}\"", file.filename().string());
          return;
        }
        LOG_INFO("loading scripture data: file_name=\"{}\"", file.filename().string());
        if(auto scripture = read(file))
        {
          scripture_files_.emplace(file, std::move(scripture));
        }
      }
    );
  }
  name_scriptures();
}

///
///
auto core_scripture_store::name_scriptures() -> void
{
  static constexpr auto uint_ending_format = " ({})";

  scripture_data_.clear();
  // The files are keyed by path, so the scriptures are named in the order of their file names
  std::ranges::for_each(
    scripture_files_ | std::views::values,
    [this](const auto& scripture)
    {
      auto name = scripture->information().name;
      if(scripture_data_.contains(name))
      {
        // Scriptures sharing a name are told apart by a counting suffix, counted up from the names
        // already taken. Not const: a filter view cannot be iterated through a const reference.
        auto endings =
          scripture_data_ | std::views::keys |
          std::views::filter([&name](const auto& n) { return util::string::starts_with(n, name); }) |
          std::views::transform([](const auto& n)
                                { return util::string::ends_with_formatted_uint(n, uint_ending_format).value_or(0); });
        const auto highest =
          std::ranges::fold_left(endings, std::uint32_t{0}, [](const auto a, const auto b) { return std::max(a, b); });
        name += std::format(uint_ending_format, highest + 1);
      }
      scripture_data_.emplace(std::move(name), scripture);
    }
  );
}

///
///
auto core_scripture_store::read(const std::filesystem::path& file) const -> std::shared_ptr<bible::scripture>
{
  const auto container = container_type_of(file);
  if(!container)
  {
    return nullptr;
  }
  try
  {
    switch(*container)
    {
    case container_type::zip:
    {
      const auto file_name = file.filename().string();
      const auto archive = io::zip_file_reader{file};
      if(!archive.is_open())
      {
        LOG_ERROR("failed to open scripture archive: file_name=\"{}\"", file_name);
        return nullptr;
      }
      const auto reader =
        std::ranges::find_if(readers_, [&archive](const auto& candidate) { return candidate->recognizes(archive); });
      if(reader == std::ranges::end(readers_))
      {
        LOG_WARN("no known scripture format in file: file_name=\"{}\"", file_name);
        return nullptr;
      }
      auto scripture = (*reader)->read(archive);
      if(!scripture)
      {
        LOG_ERROR("failed to read scripture: file_name=\"{}\", format=\"{}\"", file_name, (*reader)->name());
        return nullptr;
      }
      return scripture;
    }
    }
  }
  catch(...)
  {
    LOG_ERROR("failed to read scripture data: file_name=\"{}\", {}", file.filename().string(), util::exception_report());
  }
  return nullptr;
}

} // namespace bibstd::core
