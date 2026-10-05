#include <bibstd/util/url.hpp>

#include <catch2/catch_test_macros.hpp>

namespace bibstd::util::url
{

TEST_CASE("is_web_url", "[util]")
{
  CHECK(is_web_url("https://www.bibleserver.com/NG%C3%9C.ELB/Johannes3%2C16-18"));
  CHECK(is_web_url("http://example.com"));
  CHECK(is_web_url("HTTPS://example.com/path?query=1#fragment"));
  // What the system would open as something else than a web page
  CHECK_FALSE(is_web_url("file:///C:/Windows/notepad.exe"));
  CHECK_FALSE(is_web_url("calc.exe"));
  CHECK_FALSE(is_web_url("C:\\Windows\\notepad.exe"));
  CHECK_FALSE(is_web_url("javascript:alert(1)"));
  CHECK_FALSE(is_web_url("mailto:someone@example.com"));
  CHECK_FALSE(is_web_url("www.example.com"));
  // No valid url, a space or a quote would end the argument the browser is started with
  CHECK_FALSE(is_web_url("https://example.com/a b"));
  CHECK_FALSE(is_web_url("https://example.com/\" --flag"));
  CHECK_FALSE(is_web_url("https://example.com/Könige"));
  CHECK_FALSE(is_web_url(""));
}

} // namespace bibstd::util::url
