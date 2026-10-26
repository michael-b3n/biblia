#include "test_utils/scripts.hpp"
#include "test_utils/scripture_store.hpp"
#include "test_utils/temp_folder.hpp"
#include "test_utils/zip_archive.hpp"

#include <bibstd/bible/reference.hpp>
#include <bibstd/bible/scripture.hpp>
#include <bibstd/workflow/workflow_script.hpp>
#include <bibstd/workflow/workflow_scripture_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bibstd::workflow
{
namespace
{

///
/// Create an empty file at \p path.
///
auto touch(const std::filesystem::path& path) -> void
{
  const auto stream = std::ofstream{path};
}

///
/// \return the entries of a USFM archive of the scripture named \p name in its Paratext settings
///
[[nodiscard]] auto named_usfm(const std::string& name) -> std::vector<io::test_utils::zip_entry>
{
  auto entries = test_utils::usfm_entries();
  entries.emplace_back("Settings.xml", "<ScriptureText><FullName>" + name + "</FullName></ScriptureText>");
  return entries;
}

} // namespace

TEST_CASE("workflow_scripture_store_reads_the_files_of_its_folder", "[workflow]")
{
  const auto fixture = test_utils::scripture_store{
    "workflow_scripture_store_reads_the_files_of_its_folder",
    {{"a.zip", test_utils::usfm_entries()}, {"b.zip", test_utils::usfm_entries()}}
  };
  const auto scriptures = fixture.store->scriptures();
  REQUIRE(scriptures.size() == 2);
  // Without settings the name is the one of the file, language and copyright stay empty
  REQUIRE(scriptures.contains("a"));
  CHECK(scriptures.at("a")->information().language.empty());
  CHECK_FALSE(scriptures.at("a")->information().copyright);
  const auto passage = scriptures.at("a")->passage(bible::reference::create_unguarded(bible::book_id::john, 1u, 1u));
  REQUIRE(passage);
  CHECK(passage->content == R"(<p data-id="begin">JHN 1:1</p>)");
}

TEST_CASE("workflow_scripture_store_keeps_three_scriptures_of_equal_name_apart", "[workflow]")
{
  const auto fixture = test_utils::scripture_store{
    "workflow_scripture_store_keeps_three_scriptures_of_equal_name_apart",
    {{"a.zip", named_usfm("Same")}, {"b.zip", named_usfm("Same")}, {"c.zip", named_usfm("Same")}}
  };
  const auto scriptures = fixture.store->scriptures();
  REQUIRE(scriptures.size() == 3);
  // The first keeps the name of the scripture, the ones after it count up from the names taken
  CHECK(scriptures.contains("Same"));
  CHECK(scriptures.contains("Same (1)"));
  CHECK(scriptures.contains("Same (2)"));
}

TEST_CASE("workflow_scripture_store_counts_only_the_suffixes_of_its_name", "[workflow]")
{
  // Another name starting alike plays no role
  const auto fixture = test_utils::scripture_store{
    "workflow_scripture_store_counts_only_the_suffixes_of_its_name",
    {{"a.zip", named_usfm("Same Study (5)")}, {"b.zip", named_usfm("Same")}, {"c.zip", named_usfm("Same")}}
  };
  const auto scriptures = fixture.store->scriptures();
  REQUIRE(scriptures.size() == 3);
  CHECK(scriptures.contains("Same Study (5)"));
  CHECK(scriptures.contains("Same"));
  CHECK(scriptures.contains("Same (1)"));
}

TEST_CASE("workflow_scripture_store_leaves_out_a_file_its_reader_rejects", "[workflow]")
{
  // No book of the app, and no format of a reader
  const auto fixture = test_utils::scripture_store{
    "workflow_scripture_store_leaves_out_a_file_its_reader_rejects",
    {{"other_book.zip", {{"XXA.usfm", "\\id XXA\n\\c 1\n\\p\n\\v 1 Extra\n"}}},
     {"unknown.zip", {{"readme.txt", "no scripture here"}}}}
  };
  CHECK(fixture.store->scriptures().empty());
}

TEST_CASE("workflow_scripture_store_reads_entries_only_while_reading", "[workflow]")
{
  const auto fixture = test_utils::scripture_store{"workflow_scripture_store_reads_entries_only_while_reading"};
  const auto result = fixture.script->state()->safe_script(
    "return root.interface.workflow.scripture_store.read_entry('GEN.usfm')", sol::script_pass_on_error
  );
  CHECK_FALSE(result.valid());
}

TEST_CASE("workflow_scripture_store_leaves_files_it_cannot_read_where_they_are", "[workflow]")
{
  const auto source = test_utils::temp_folder{"workflow_scripture_store_import_unreadable_source"};
  // A zip holding no scripture, an archive of an unknown format and a file of a type the store does not know
  touch(source.path() / "scripture.zip");
  io::test_utils::write_zip_archive(
    source.path() / "unknown.zip",
    {
      {"readme.txt", "no scripture here"}
  }
  );
  touch(source.path() / "notes.txt");

  auto fixture = test_utils::scripture_store{"workflow_scripture_store_import_unreadable_target"};
  CHECK(fixture.store->import(source.path()) == 0);
  CHECK(fixture.store->import(source.path() / "scripture.zip") == 0);
  CHECK(fixture.store->import(source.path() / "missing.zip") == 0);
  CHECK(fixture.store->scriptures().empty());
  // The folder of the store only ever holds files it can read
  CHECK(std::filesystem::is_empty(fixture.scripture_folder()));
}

TEST_CASE("workflow_scripture_store_takes_over_the_scripture_files_of_a_folder", "[workflow]")
{
  const auto source = test_utils::temp_folder{"workflow_scripture_store_import_source"};
  io::test_utils::write_zip_archive(source.path() / "a.zip", test_utils::usfm_entries());
  io::test_utils::write_zip_archive(source.path() / "b.zip", test_utils::usfm_entries());
  touch(source.path() / "notes.txt");
  std::filesystem::create_directories(source.path() / "nested");
  io::test_utils::write_zip_archive(source.path() / "nested" / "nested.zip", test_utils::usfm_entries());

  auto fixture = test_utils::scripture_store{"workflow_scripture_store_import_target"};
  CHECK(fixture.store->import(source.path()) == 2);
  CHECK(fixture.store->scriptures().size() == 2);
  CHECK(!std::filesystem::exists(fixture.scripture_folder() / "notes.txt"));
  // Only the files of the folder itself are taken, the scripture folder is flat
  CHECK(!std::filesystem::exists(fixture.scripture_folder() / "nested.zip"));

  // A second import replaces the files of the first, it does not add to them
  CHECK(fixture.store->import(source.path()) == 2);
  CHECK(fixture.store->scriptures().size() == 2);

  // A single file
  CHECK(fixture.store->import(source.path() / "nested" / "nested.zip") == 1);
  CHECK(fixture.store->scriptures().size() == 3);
}

TEST_CASE("workflow_scripture_store_takes_nothing_from_its_own_folder", "[workflow]")
{
  auto fixture = test_utils::scripture_store{
    "workflow_scripture_store_takes_nothing_from_its_own_folder", {{"a.zip", test_utils::usfm_entries()}}
  };
  CHECK(fixture.store->import(fixture.scripture_folder()) == 0);
  CHECK(fixture.store->import(fixture.scripture_folder() / "a.zip") == 0);
  CHECK(std::filesystem::exists(fixture.scripture_folder() / "a.zip"));
}

TEST_CASE("workflow_scripture_store_reads_an_import_before_the_scripts_once_they_are_loaded", "[workflow]")
{
  const auto source = test_utils::temp_folder{"workflow_scripture_store_import_early_source"};
  io::test_utils::write_zip_archive(source.path() / "a.zip", test_utils::usfm_entries());
  const auto folder = test_utils::temp_folder{"workflow_scripture_store_import_early"};
  const auto settings = test_utils::make_script_settings(folder.path());
  const auto script = std::make_shared<workflow_script>(settings);
  const auto store = std::make_shared<workflow_scripture_store>(settings, script);

  // No reader yet, the file is taken over unread
  CHECK(store->import(source.path() / "a.zip") == 1);
  CHECK(store->scriptures().empty());
  test_utils::load_scripts(*script);
  store->load();
  CHECK(store->scriptures().contains("a"));
}

TEST_CASE("workflow_scripture_store_reads_a_changed_file_anew", "[workflow]")
{
  auto fixture =
    test_utils::scripture_store{"workflow_scripture_store_reads_a_changed_file_anew", {{"a.zip", test_utils::usfm_entries()}}};
  REQUIRE(fixture.store->scriptures().contains("a"));
  // An unchanged file keeps what was read
  fixture.store->load();
  CHECK(fixture.store->scriptures().contains("a"));

  auto entries = test_utils::usfm_entries();
  entries.emplace_back("Settings.xml", "<ScriptureText><FullName>Changed</FullName></ScriptureText>");
  io::test_utils::write_zip_archive(fixture.scripture_folder() / "a.zip", entries);
  const auto file = fixture.scripture_folder() / "a.zip";
  std::filesystem::last_write_time(file, std::filesystem::last_write_time(file) + std::chrono::seconds{10});
  fixture.store->load();
  CHECK(fixture.store->scriptures().contains("Changed"));
}

} // namespace bibstd::workflow
