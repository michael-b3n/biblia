#include "bibstd/workflow/workflow_scripture_store.hpp"
#include "bibstd/bible/common.hpp"
#include "bibstd/bible/reference.hpp"
#include "bibstd/bible/scripture.hpp"
#include "bibstd/bible/scripture_types.hpp"
#include "bibstd/io/zip_file_reader.hpp"
#include "bibstd/util/enum.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/log.hpp"
#include "bibstd/util/string.hpp"
#include "bibstd/workflow/workflow_settings.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <ranges>
#include <system_error>
#include <tuple>
#include <utility>

namespace bibstd::workflow
{
namespace
{

///
/// \return true if \p path is a container a reader may read, a zip archive
///
[[nodiscard]] auto is_container(const std::filesystem::path& path) -> bool
{
  return path.extension() == ".zip";
}

///
/// Access the files directly inside \p folder, leaving out what is not a file of its own.
/// \return view of the file paths
///
[[nodiscard]] auto regular_files(const std::filesystem::path& folder, std::error_code& error)
{
  return std::filesystem::directory_iterator{folder, error} |
         std::views::filter([&error](const auto& entry) { return entry.is_regular_file(error); }) |
         std::views::transform([](const auto& entry) { return entry.path(); });
}

///
/// \return the book of the identifier \p name of the app, e.g. "john"
///
[[nodiscard]] auto book_of(const std::string& name) -> bible::book_id
{
  const auto book = util::to_enum<bible::book_id>(name);
  if(!book)
  {
    throw util::exception{std::format("unknown book: book=\"{}\"", name)};
  }
  return *book;
}

///
/// \return \p number as chapter or verse number, which start at 1
///
[[nodiscard]] auto number_of(const std::int64_t number) -> std::uint32_t
{
  static constexpr auto max_number = static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max());
  if(number < 1 || number > max_number)
  {
    throw util::exception{std::format("invalid chapter or verse number: number={}", number)};
  }
  return static_cast<std::uint32_t>(number);
}

///
/// Access the values of \p by_book, a map by book, chapter and verse as a
/// reader outputs them. Throws if it names an unknown book or a number below 1.
/// \return view of the pairs of reference and value
///
[[nodiscard]] auto by_reference(auto& by_book)
{
  const auto of_book = [](auto& book_entry)
  {
    auto& [book, chapters] = book_entry;
    const auto of_chapter = [book = book_of(book)](auto& chapter_entry)
    {
      auto& [chapter, verses] = chapter_entry;
      const auto of_verse = [book, chapter = number_of(chapter)](auto& verse_entry)
      {
        return std::pair<bible::reference, decltype((verse_entry.second))>{
          bible::reference::create_unguarded(book, chapter, number_of(verse_entry.first)), verse_entry.second
        };
      };
      return verses | std::views::transform(of_verse);
    };
    return chapters | std::views::transform(of_chapter) | std::views::join;
  };
  return by_book | std::views::transform(of_book) | std::views::join;
}

///
/// \return the scripture of the reader output \p output. Throws if
/// it names an unknown book or a number below 1, or holds no verse.
///
[[nodiscard]] auto scripture_of(workflow_scripture_store::read_manifest::output output) -> std::shared_ptr<bible::scripture>
{
  // A field left out holds nothing
  static constexpr auto filled = [](auto& field) -> auto&
  {
    if(!field)
    {
      field.emplace();
    }
    return *field;
  };
  static constexpr auto name_of = [](const std::map<std::string, std::string>& names, const std::string& form)
  {
    const auto it = names.find(form);
    return it != names.cend() ? it->second : std::string{};
  };
  static constexpr auto to_book_name = [](const auto& entry)
  {
    const auto& [book, names] = entry;
    auto book_name = bible::book_name{
      .abbreviation = name_of(names, "abbreviation"),
      .short_name = name_of(names, "short_name"),
      .long_name = name_of(names, "long_name")
    };
    return std::pair{book_of(book), std::move(book_name)};
  };
  auto book_names = filled(output.get<"book_names">()) | std::views::transform(to_book_name) |
                    std::ranges::to<bible::scripture::book_name_map_type>();
  // By value, the view hands out temporary pairs of reference and content
  static constexpr auto to_passage = [](auto entry)
  {
    auto& [ref, content] = entry;
    return std::pair{
      ref, bible::passage{.ref = ref, .content = std::move(content)}
    };
  };
  auto passages = by_reference(filled(output.get<"verses">())) | std::views::transform(to_passage) |
                  std::ranges::to<bible::scripture::passage_map_type>();
  if(passages.empty())
  {
    throw util::exception{"no verse of the app"};
  }
  // The cross references of a verse without passage are left out
  std::ranges::for_each(
    by_reference(filled(output.get<"cross_references">())),
    [&passages](auto verse)
    {
      auto& [ref, cross_references] = verse;
      if(const auto it = passages.find(ref); it != passages.end())
      {
        it->second.cross_references = std::move(cross_references);
      }
    }
  );

  auto info = bible::scripture_info{
    .name = *output.get<"name">(),
    // An abbreviation left out is unknown, a language or copyright stays empty
    .abbreviation = output.get<"abbreviation">().value_or("Unknown Abbreviation"),
    .language = output.get<"language">().value_or(""),
    .copyright = output.get<"copyright">()
  };
  LOG_INFO(
    "read scripture: name=\"{}\", abbreviation=\"{}\", language=\"{}\", verses={}, named_books={}",
    info.name,
    info.abbreviation,
    info.language,
    passages.size(),
    book_names.size()
  );
  return std::make_shared<bible::scripture>(std::move(info), std::move(book_names), std::move(passages));
}

///
/// \return the scriptures of \p files by name, named in the order of their
/// files. A name already taken gets a counting suffix, e.g. "Name (2)".
///
[[nodiscard]] auto named(const std::map<std::filesystem::path, std::shared_ptr<bible::scripture>>& files)
  -> workflow_scripture_store::scripture_map_type
{
  static constexpr auto uint_ending_format = " ({})";

  auto scriptures = workflow_scripture_store::scripture_map_type{};
  std::ranges::for_each(
    files | std::views::values,
    [&scriptures](const auto& scripture)
    {
      auto name = scripture->information().name;
      if(scriptures.contains(name))
      {
        // Count up from the suffixes of this name, not of another name starting alike, e.g. "Name
        // Study (3)". Not const: a filter view cannot be iterated through a const reference.
        const auto ending_of = [&name](const std::string& n) -> std::uint32_t
        {
          const auto number = util::string::ends_with_formatted_uint(n, uint_ending_format);
          return number && n == name + std::format(uint_ending_format, *number) ? *number : 0;
        };
        auto endings = scriptures | std::views::keys |
                       std::views::filter([&name](const auto& n) { return util::string::starts_with(n, name); }) |
                       std::views::transform(ending_of);
        name += std::format(uint_ending_format, std::ranges::max(endings) + 1);
      }
      scriptures.emplace(std::move(name), scripture);
    }
  );
  return scriptures;
}

} // namespace

///
///
workflow_scripture_store::workflow_scripture_store(
  std::shared_ptr<workflow_settings> workflow_settings, std::shared_ptr<workflow_script> workflow_script
)
  : workflow_script_{std::move(workflow_script)}
  , folder_{workflow_settings->data_folder() / folder_name}
{
}

///
///
workflow_scripture_store::~workflow_scripture_store() noexcept = default;

///
///
auto workflow_scripture_store::scriptures() const -> scripture_map_type
{
  const auto lock = std::scoped_lock{mtx_};
  return named(files_);
}

///
///
auto workflow_scripture_store::load() -> void
{
  auto error = std::error_code{};
  // Created here so a later import has somewhere to copy to
  std::ignore = std::filesystem::create_directories(folder_, error);
  // Held throughout, so an import waits until the folder is read
  const auto state = workflow_script_->state();
  const auto supported = [](const std::filesystem::path& file)
  {
    const auto container = is_container(file);
    if(!container)
    {
      LOG_WARN("file type not supported: file_name=\"{}\"", file.filename().string());
    }
    return container;
  };
  const auto read_file = [this](const std::filesystem::path& file)
  {
    LOG_INFO("loading scripture data: file_name=\"{}\"", file.filename().string());
    return std::pair{file, read(file)};
  };
  auto files = regular_files(folder_, error) | std::views::filter(supported) | std::views::transform(read_file) |
               std::ranges::to<decltype(files_)>();
  // A file no reader reads is left out
  std::erase_if(files, [](const auto& file) { return !file.second; });
  if(error)
  {
    LOG_WARN("failed to read the scripture folder: folder=\"{}\", error={}", folder_.generic_string(), error.message());
  }
  const auto lock = std::scoped_lock{mtx_};
  files_ = std::move(files);
}

///
///
auto workflow_scripture_store::import(const std::filesystem::path& source) -> std::size_t
{
  auto error = std::error_code{};
  const auto is_file = std::filesystem::is_regular_file(source, error);
  if(!is_file && !std::filesystem::is_directory(source, error))
  {
    LOG_WARN("scripture import source not found: source=\"{}\"", source.generic_string());
    return 0;
  }
  if(std::filesystem::equivalent(is_file ? source.parent_path() : source, folder_, error))
  {
    LOG_WARN("scripture import skipped, the source is in the scripture folder: source=\"{}\"", source.generic_string());
    return 0;
  }
  std::ignore = std::filesystem::create_directories(folder_, error);

  // Held throughout, so a load takes turns with the import
  const auto state = workflow_script_->state();
  // Before the scripts are loaded there is no reader, the load after them reads the copied files
  const auto readers_loaded = !workflow_script_->scripts<read_manifest>().empty();
  const auto take_over = [&](const std::filesystem::path& file)
  {
    // Read before the copy, so a file this store cannot load never reaches the folder
    auto scripture = readers_loaded ? read(file) : nullptr;
    if(readers_loaded && !scripture)
    {
      LOG_WARN("scripture file not taken over: file_name=\"{}\"", file.filename().string());
      return false;
    }
    const auto target = folder_ / file.filename();
    if(!std::filesystem::copy_file(file, target, std::filesystem::copy_options::overwrite_existing, error))
    {
      LOG_ERROR("failed to copy scripture file: file_name=\"{}\", {}", file.filename().string(), error.message());
      return false;
    }
    if(!scripture)
    {
      LOG_INFO("scripture file copied unread, the scripts are not loaded yet: file_name=\"{}\"", file.filename().string());
      return true;
    }
    const auto lock = std::scoped_lock{mtx_};
    LOG_INFO("scripture file {}: file_name=\"{}\"", files_.contains(target) ? "replaced" : "copied", file.filename().string());
    files_.insert_or_assign(target, std::move(scripture));
    return true;
  };
  return static_cast<std::size_t>(std::ranges::count_if(
    (is_file ? std::vector{source} : regular_files(source, error) | std::ranges::to<std::vector>()) |
      std::views::filter(is_container),
    take_over
  ));
}

///
///
auto workflow_scripture_store::read(const std::filesystem::path& file) const -> std::shared_ptr<bible::scripture>
{
  const auto file_name = file.filename().string();
  try
  {
    const auto archive = io::zip_file_reader{file};
    if(!archive.is_open())
    {
      LOG_ERROR("failed to open scripture archive: file_name=\"{}\"", file_name);
      return nullptr;
    }
    auto entries = archive.entries() |
                   std::views::filter([](const auto& entry) { return entry.name && !entry.name->ends_with('/'); }) |
                   std::views::transform([](const auto& entry) { return *entry.name; }) | std::ranges::to<std::vector>();
    const auto input = read_manifest::input{file_name, std::move(entries)};

    // Held while the readers run, so no other script reaches the archive
    const auto state = workflow_script_->state();
    ///
    /// \lua Reads an entry of the archive a reader reads, \see the
    /// manifest `scripture.read`. It exists only while a reader runs.
    /// \param name the name of the entry, one of the `entries` of the input
    /// \return its content, or `nil` and `not found`
    ///
    const auto registration = workflow_script_->register_function(
      "scripture_store.read_entry",
      [&archive](const std::string& name) -> std::expected<std::string, std::string>
      {
        const auto entry = archive.entry(name);
        if(!entry)
        {
          return std::unexpected{std::string{"not found"}};
        }
        return archive.read_entry_as_string(*entry);
      }
    );
    for(const auto& reader : workflow_script_->scripts<read_manifest>() | std::views::keys)
    {
      // A reader of another format answers with neither an error nor a name
      auto output = workflow_script_->run<read_manifest>(reader, input);
      if(output && output->get<"error">())
      {
        LOG_ERROR(
          "failed to read scripture: file_name=\"{}\", reader=\"{}\", error=\"{}\"", file_name, reader, *output->get<"error">()
        );
        return nullptr;
      }
      if(output && output->get<"name">())
      {
        return scripture_of(std::move(*output));
      }
    }
    LOG_WARN("no reader for the scripture file: file_name=\"{}\"", file_name);
  }
  catch(...)
  {
    LOG_ERROR("failed to read scripture data: file_name=\"{}\", {}", file_name, util::exception_report());
  }
  return nullptr;
}

} // namespace bibstd::workflow
