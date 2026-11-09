#include <arca/framework/property_tree.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace bibstd::framework
{
namespace
{

[[nodiscard]] auto read_file(const std::filesystem::path& path) -> std::string
{
  auto stream = std::ostringstream{};
  stream << std::ifstream{path}.rdbuf();
  return stream.str();
}

auto write_file(const std::filesystem::path& path, const std::string& content) -> void
{
  std::ofstream{path} << content;
}

[[nodiscard]] auto load_and_store(const std::filesystem::path& path) -> std::string
{
  auto tree = property_tree::create(path);
  const auto value = tree->create_property(property_tree::path_type{"settings.name"}, std::string{"default"});
  return value.value();
}

} // namespace

TEST_CASE("property_tree_whitespace", "[framework]")
{
  const auto path = std::filesystem::temp_directory_path() / "bibstd_test_property_tree_whitespace.xml";
  // Whitespace an earlier version piled up between the elements on every write
  write_file(
    path, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<settings>\n\n\n  \n  \n<name>\n\n  stored\n</name></settings>\n"
  );

  CHECK(load_and_store(path) == "stored");
  const auto first = read_file(path);
  CHECK(first.find("\n\n") == std::string::npos);
  CHECK(first.find("<name>stored</name>") != std::string::npos);

  // A file that was written once stays as it is
  CHECK(load_and_store(path) == "stored");
  CHECK(read_file(path) == first);

  std::filesystem::remove(path);
}

} // namespace bibstd::framework
