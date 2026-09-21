#include <bibstd/bible/scripture.hpp>
#include <bibstd/core/core_scripture_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <string>

namespace bibstd::core
{
namespace
{

///
/// Folder of its own for a test case, emptied on construction so a leftover of an earlier run
/// cannot count towards the result.
/// \return folder path
///
auto test_folder(const std::string_view name) -> std::filesystem::path
{
  const auto folder = std::filesystem::temp_directory_path() / "bibstd_test_scripture_store" / name;
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  return folder;
}

///
/// Create an empty file at \p path.
///
auto touch(const std::filesystem::path& path) -> void
{
  const auto stream = std::ofstream{path};
}

///
/// Copy the scripture files of the test resources into \p folder.
/// \return the folder
///
auto fill_with_scriptures(const std::filesystem::path& folder) -> std::filesystem::path
{
  std::ranges::for_each(
    std::filesystem::directory_iterator{BIBSTD_TEST_SCRIPTURE_DIR} |
      std::views::filter([](const auto& entry) { return entry.is_regular_file(); }) |
      std::views::transform([](const auto& entry) { return entry.path(); }),
    [&folder](const auto& file) { std::filesystem::copy_file(file, folder / file.filename()); }
  );
  return folder;
}

///
/// Copy the first scripture file of the test resources into \p folder under \p count names.
/// \return the folder, empty if the test resources hold no scripture file
///
auto fill_with_one_scripture(const std::filesystem::path& folder, const std::size_t count) -> std::filesystem::path
{
  auto files = std::filesystem::directory_iterator{BIBSTD_TEST_SCRIPTURE_DIR} |
               std::views::transform([](const auto& entry) { return entry.path(); }) |
               std::views::filter([](const auto& file) { return file.extension() == ".zip"; });
  const auto found = std::ranges::begin(files);
  if(found == std::ranges::end(files))
  {
    return {};
  }
  std::ranges::for_each(
    std::views::iota(std::size_t{0}, count),
    [&](const auto i) { std::filesystem::copy_file(*found, folder / std::format("scripture_{}.zip", i)); }
  );
  return folder;
}

} // namespace

TEST_CASE("core_scripture_store holds usable scriptures", "[core]")
{
  const core_scripture_store store{BIBSTD_TEST_SCRIPTURE_DIR};
  if(store.scriptures().empty())
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }
  for(const auto& [name, scripture] : store.scriptures())
  {
    INFO(std::format("scripture: {}", name));
    CHECK(!name.empty());
    REQUIRE(scripture != nullptr);

    // The key is the scripture name, possibly with a " (n)" suffix to disambiguate duplicates.
    CHECK(name.starts_with(scripture->information().name));
    CHECK(!scripture->versification().name().empty());
    CHECK(scripture->versification().count() > 0);
  }
}

TEST_CASE("core_scripture_store keeps scriptures of equal name apart", "[core]")
{
  const core_scripture_store store{BIBSTD_TEST_SCRIPTURE_DIR};
  if(store.scriptures().empty())
  {
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }

  // The store keys by scripture name, so scriptures sharing one would overwrite each other without
  // the disambiguating suffix. Every loaded scripture has to survive that.
  const auto names = store.scriptures() | std::views::values |
                     std::views::transform([](const auto& scripture) { return scripture->information().name; }) |
                     std::ranges::to<std::vector<std::string>>();
  for(const auto& name : names)
  {
    INFO(std::format("scripture name: {}", name));
    const auto keyed =
      std::ranges::count_if(store.scriptures(), [&](const auto& entry) { return entry.first.starts_with(name); });
    CHECK(keyed == std::ranges::count(names, name));
  }
}

TEST_CASE("core_scripture_store keeps three scriptures of equal name apart", "[core]")
{
  // The same scripture under three file names, so all three carry the very same name
  const auto folder = fill_with_one_scripture(test_folder("three_equal_names"), 3);
  if(folder.empty())
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }

  const core_scripture_store store{folder};
  REQUIRE(store.scriptures().size() == 3);

  // The first keeps the name of the scripture, the ones after it count up from the names taken
  const auto name = store.scriptures().begin()->second->information().name;
  CHECK(store.scriptures().contains(name));
  CHECK(store.scriptures().contains(std::format("{} (1)", name)));
  CHECK(store.scriptures().contains(std::format("{} (2)", name)));
}

TEST_CASE("core_scripture_store leaves files it cannot read where they are", "[core]")
{
  const auto source = test_folder("import_unreadable_source");
  const auto target = test_folder("import_unreadable_target");
  // A zip holding no scripture data, and a file of a type the store does not know
  touch(source / "scripture.zip");
  touch(source / "notes.txt");

  core_scripture_store store{target};
  CHECK(store.import(source) == 0);
  CHECK(store.scriptures().empty());
  // The folder of the store only ever holds files it can load
  CHECK(!std::filesystem::exists(target / "scripture.zip"));
  CHECK(!std::filesystem::exists(target / "notes.txt"));
}

TEST_CASE("core_scripture_store takes over the scripture files of a folder", "[core]")
{
  const auto scriptures = core_scripture_store{BIBSTD_TEST_SCRIPTURE_DIR};
  if(scriptures.scriptures().empty())
  {
    // The scriptures are not part of the repository, \see bibstd_test/res/scripture/README.md.
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }
  const auto source = fill_with_scriptures(test_folder("import_source"));
  touch(source / "notes.txt");
  std::filesystem::create_directories(source / "nested");
  touch(source / "nested" / "nested.zip");

  const auto target = test_folder("import_target");
  core_scripture_store store{target};
  CHECK(store.import(source) == scriptures.scriptures().size());
  CHECK(store.scriptures().size() == scriptures.scriptures().size());
  CHECK(!std::filesystem::exists(target / "notes.txt"));
  // Only the files of the folder itself are taken, the scripture folder is flat
  CHECK(!std::filesystem::exists(target / "nested.zip"));
}

TEST_CASE("core_scripture_store holds a scripture once after importing it twice", "[core]")
{
  const auto scriptures = core_scripture_store{BIBSTD_TEST_SCRIPTURE_DIR};
  if(scriptures.scriptures().empty())
  {
    SKIP(std::format("no scriptures in {}", BIBSTD_TEST_SCRIPTURE_DIR));
  }
  const auto source = fill_with_scriptures(test_folder("import_twice_source"));

  core_scripture_store store{test_folder("import_twice_target")};
  CHECK(store.import(source) == scriptures.scriptures().size());
  // The second import replaces the files of the first, it does not add to them
  CHECK(store.import(source) == scriptures.scriptures().size());
  CHECK(store.scriptures().size() == scriptures.scriptures().size());
}

TEST_CASE("core_scripture_store takes nothing from its own folder", "[core]")
{
  const auto folder = test_folder("import_onto_itself");
  touch(folder / "scripture.zip");

  core_scripture_store store{folder};
  CHECK(store.import(folder) == 0);
  CHECK(std::filesystem::exists(folder / "scripture.zip"));
}

} // namespace bibstd::core
