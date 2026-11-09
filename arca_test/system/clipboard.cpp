#include "test_utils/clipboard.hpp"

#include <arca/system/clipboard.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace bibstd::system
{

// Hidden, it replaces the clipboard of whoever runs it. Run it with: arca_test "[.clipboard]"
TEST_CASE("clipboard replaces the text", "[.clipboard]")
{
  const auto restorer = test_utils::clipboard_restorer{};
  const auto text = std::string{"\u201EDenn also\u201C\n(Johannes\u00A03,16)\tTKW"};
  REQUIRE(clipboard::set_text(text));
  CHECK(test_utils::clipboard_text() == text);
}

} // namespace bibstd::system
