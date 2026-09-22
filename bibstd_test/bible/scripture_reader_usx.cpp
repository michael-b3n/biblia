#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture_reader.hpp>
#include <bibstd/bible/scripture_reader_usx.hpp>
#include <bibstd/bible/scripture_types.hpp>
#include <bibstd/io/zip_file_reader.hpp>
#include <bibstd/util/enum.hpp>
#include <bibstd/util/non_owning_ptr.hpp>
#include <test_utils/zip_archive.hpp>

#include <catch2/catch_test_macros.hpp>
#include <pugixml.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace bibstd::bible
{
namespace
{

///
/// A scripture shipped with the application, together with the bundle it was loaded from.
///
struct shipped_scripture final
{
  std::string bundle;
  std::unique_ptr<scripture> loaded;
};

///
/// Load every scripture shipped with the application.
/// \return loaded scriptures
///
auto load_shipped_scriptures() -> std::vector<shipped_scripture>
{
  auto result = std::vector<shipped_scripture>{};
  if(!std::filesystem::is_directory(BIBSTD_TEST_SCRIPTURE_DIR))
  {
    return result;
  }
  for(const auto& file : std::filesystem::directory_iterator{BIBSTD_TEST_SCRIPTURE_DIR})
  {
    if(file.path().extension() != std::filesystem::path{".zip"})
    {
      continue;
    }
    INFO(std::format("bundle: {}", file.path().filename().string()));
    const auto reader = io::zip_file_reader{file.path()};
    REQUIRE(reader.is_open());
    auto loaded = scripture_reader_usx{}.read(reader);
    REQUIRE(loaded != nullptr);
    result.emplace_back(file.path().filename().string(), std::move(loaded));
  }
  return result;
}

///
/// Access the scriptures shipped with the application. Reading a bundle is expensive, so they are
/// read once and shared by every test case.
/// \return loaded scriptures
///
auto shipped_scriptures() -> const std::vector<shipped_scripture>&
{
  static const auto scriptures = load_shipped_scriptures();
  if(scriptures.empty())
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }
  return scriptures;
}

///
/// Access the scripture shipped in the given bundle.
/// \return the scripture, or nullptr if no such bundle is shipped
///
auto find_bundle(const std::string_view bundle) -> util::non_owning_ptr<const scripture>
{
  const auto& scriptures = shipped_scriptures();
  const auto found = std::ranges::find(scriptures, bundle, &shipped_scripture::bundle);
  return found != std::ranges::cend(scriptures) ? found->loaded.get() : nullptr;
}

///
/// \return the passage markup a paragraph section of the given position opens with
///
auto paragraph_open(const std::string_view position) -> std::string
{
  return std::format(R"(<{} {}="{}">)", passage_markup::paragraph, passage_markup::paragraph_attribute, position);
}

} // namespace

TEST_CASE("scripture_reader_usx recognizes a bundle by its descriptor", "[bible]")
{
  const auto reader = scripture_reader_usx{};
  CHECK(!reader.name().empty());

  const auto folder = std::filesystem::temp_directory_path() / "bibstd_test_scripture_reader_usx";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  const auto archive_path = folder / "unknown.zip";
  const auto entries = std::vector<io::test_utils::zip_entry>{
    {"readme.txt", "no scripture here"}
  };
  io::test_utils::write_zip_archive(archive_path, entries);

  // An archive without the bundle descriptor holds another reader's format, if any
  const auto unknown = io::zip_file_reader{archive_path};
  REQUIRE(unknown.is_open());
  CHECK(!reader.recognizes(unknown));

  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto archive = io::zip_file_reader{std::filesystem::path{BIBSTD_TEST_SCRIPTURE_DIR} / bundle};
    REQUIRE(archive.is_open());
    CHECK(reader.recognizes(archive));
  }
}

TEST_CASE("scripture_reader_usx provides names for every book of every shipped scripture", "[bible]")
{
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    static constexpr auto books = util::enum_values<book_id>();
    for(const auto book : books)
    {
      INFO(std::format("book: {}", util::enum_name(book)));
      const auto names = loaded->book_information(book);
      REQUIRE(names.has_value());
      // every form is filled from the book's header paragraphs, falling back to the closest alternative
      CHECK(!names->short_name.empty());
      CHECK(!names->abbreviation.empty());
      CHECK(!names->long_name.empty());
    }
  }
}

TEST_CASE("scripture_reader_usx book names are unique within a scripture", "[bible]")
{
  // A collision would mean the parser picked up something other than the book's own header, e.g. content shared
  // between documents.
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    static constexpr auto books = util::enum_values<book_id>();
    auto short_names = std::vector<std::string>{};
    for(const auto book : books)
    {
      short_names.push_back(loaded->book_information(book).value().short_name);
    }
    std::ranges::sort(short_names);
    CHECK(std::ranges::adjacent_find(short_names) == std::ranges::cend(short_names));
  }
}

TEST_CASE("scripture_reader_usx book names are read from the book documents", "[bible]")
{
  // The names below are the header paragraphs of the respective USX documents. Reading them there rather than from
  // the bundle metadata keeps them available even for bundles whose metadata lists no names at all.
  SECTION("german scripture")
  {
    const auto* const loaded = find_bundle("text-542b32484b6e38c2-246437.zip");
    REQUIRE(loaded != nullptr);

    const auto genesis = loaded->book_information(book_id::genesis);
    REQUIRE(genesis.has_value());
    CHECK(genesis->short_name == "1. Mose");
    CHECK(genesis->long_name == "Das 1. Buch Mose (Genesis)");

    const auto revelation = loaded->book_information(book_id::revelation);
    REQUIRE(revelation.has_value());
    CHECK(revelation->short_name == "Offenbarung");
    CHECK(revelation->long_name == "Das Buch der Offenbarung Jesu Christi");
  }

  SECTION("english scripture")
  {
    const auto* const loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
    REQUIRE(loaded != nullptr);

    const auto genesis = loaded->book_information(book_id::genesis);
    REQUIRE(genesis.has_value());
    CHECK(genesis->short_name == "Genesis");
    CHECK(genesis->abbreviation == "Gen");
    CHECK(genesis->long_name == "The First Book of Moses, called Genesis");
  }

  SECTION("scripture without running header paragraphs")
  {
    // This bundle ships no "h" paragraph at all, the table of contents entries have to carry the names.
    const auto* const loaded = find_bundle("text-f492a38d0e52db0f-258505.zip");
    REQUIRE(loaded != nullptr);

    const auto song = loaded->book_information(book_id::song_of_solomon);
    REQUIRE(song.has_value());
    CHECK(song->short_name == "Hohelied");
    CHECK(song->abbreviation == "Hld.");
  }
}

TEST_CASE("scripture_reader_usx book names are not the raw identifier", "[bible]")
{
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto names = loaded->book_information(book_id::revelation);
    REQUIRE(names.has_value());
    CHECK(names->short_name != util::enum_name(book_id::revelation));
  }
}

TEST_CASE("scripture_reader_usx book names carry no scripture text", "[bible]")
{
  // The header block ends at the first chapter marker. Should the parser run past it, the names would grow into
  // whole verses.
  static constexpr auto max_name_length = 100u;
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    static constexpr auto books = util::enum_values<book_id>();
    for(const auto book : books)
    {
      INFO(std::format("book: {}", util::enum_name(book)));
      const auto names = loaded->book_information(book);
      REQUIRE(names.has_value());
      CHECK(names->long_name.size() < max_name_length);
      CHECK(names->short_name.size() <= names->long_name.size());
    }
  }
}

TEST_CASE("scripture_reader_usx reads its information from the bundle root metadata", "[bible]")
{
  // The bundles carry a second, reduced "metadata.xml" inside their "release" directory. Resolving the descriptor
  // against the archive root is what makes the local name and the copyright statement available here.
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto info = loaded->information();
    CHECK(info.name != scripture_reader::unknown_name);
    CHECK(info.abbreviation != scripture_reader::unknown_abbreviation);
    CHECK(info.language != scripture_reader::unknown_language);
    REQUIRE(info.copyright.has_value());
    CHECK(!info.copyright->empty());
  }
}

TEST_CASE("scripture_reader_usx reads the text of a verse", "[bible]")
{
  const auto* const loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
  REQUIRE(loaded != nullptr);

  // The verse sits inside a "wj" character style, which the markup has no node for and takes over as text
  const auto found = loaded->passage(reference::create_unguarded(book_id::john, 3u, 16u));
  REQUIRE(found.has_value());
  CHECK(found->content.contains("For God so loved the world, that he gave his only begotten Son"));
  CHECK(found->ref == reference::create_unguarded(book_id::john, 3u, 16u));
}

TEST_CASE("scripture_reader_usx keeps the headings of a book out of its verses", "[bible]")
{
  // A heading between two sections of a chapter introduces the verses after it, it is no scripture
  // text of its own. Taking it over would append it to the verse before it.
  const auto* const loaded = find_bundle("text-542b32484b6e38c2-246437.zip");
  REQUIRE(loaded != nullptr);

  const auto found = loaded->passage(reference::create_unguarded(book_id::chronicles1, 1u, 34u));
  REQUIRE(found.has_value());
  CHECK(found->content.contains("Abraham"));
  CHECK(!found->content.contains("Stammbaum"));
  // the verse sits in a single paragraph, so its markup holds a single section
  const auto section = std::format("<{} ", passage_markup::paragraph);
  CHECK(found->content.starts_with(section));
  CHECK(found->content.find(section, section.size()) == std::string::npos);
}

TEST_CASE("scripture_reader_usx tells where a verse sits in its paragraph", "[bible]")
{
  // A paragraph carries many verses: the first of them opens it, every later one continues it.
  const auto* const loaded = find_bundle("text-de4e12af7f28f599-245514.zip");
  REQUIRE(loaded != nullptr);

  const auto opening = loaded->passage(reference::create_unguarded(book_id::john, 3u, 1u));
  REQUIRE(opening.has_value());
  CHECK(opening->content.starts_with(paragraph_open(passage_markup::paragraph_begin)));

  const auto continuing = loaded->passage(reference::create_unguarded(book_id::john, 3u, 2u));
  REQUIRE(continuing.has_value());
  CHECK(continuing->content.starts_with(paragraph_open(passage_markup::paragraph_continue)));
}

TEST_CASE("scripture_reader_usx passages are well formed markup", "[bible]")
{
  // Every passage is a sequence of paragraph sections and parses as XML, so that a renderer can rely on it
  // without repairing anything. Text carrying "&" or "<" would break that if it were not escaped.
  for(const auto& [bundle, loaded] : shipped_scriptures())
  {
    INFO(std::format("bundle: {}", bundle));
    const auto& versification = loaded->versification();
    auto checked = std::size_t{0};
    for(auto ref = std::optional{reference::create_unguarded(book_id::john, 1u, 1u)}; ref && ref->book() == book_id::john;
        ref = versification.next(*ref))
    {
      INFO(std::format("reference: {}", *ref));
      const auto found = loaded->passage(*ref);
      REQUIRE(found.has_value());
      CHECK(found->content.starts_with(std::format("<{} ", passage_markup::paragraph)));

      auto document = pugi::xml_document{};
      // the passage is a sequence of paragraph sections, a single root is needed to parse it
      CHECK(document.load_string(std::format("<root>{}</root>", found->content).c_str()));
      ++checked;
    }
    CHECK(checked > 0);
  }
}

} // namespace bibstd::bible
