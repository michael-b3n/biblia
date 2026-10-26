#include "test_utils/script_sandbox.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace bibstd::lua
{

TEST_CASE("dbl_paratext_and_ebible_give_the_same_info", "[lua]")
{
  auto fixture = test_utils::script_sandbox{"dbl_paratext_and_ebible_give_the_same_info"};
  fixture.texts = {
    {"metadata",
     "<DBLMetadata><identification><name>Name</name><nameLocal>Local</nameLocal><abbreviation>ABC</abbreviation>"
     "</identification><language><name>German</name></language></DBLMetadata>"                                         },
    {"settings", "<ScriptureText><Name>ABC</Name><FullName>Local</FullName><Language>German</Language></ScriptureText>"},
    {"copyright_page",
     R"(<html><body><div class="toc"><a href="https://ebible.org/ABC">Local</a></div></br><h1 class='x'>Local</h1>)"
     R"(<div>Language: <a href='https://www.ethnologue.org/language/deu'>German</a></div></body></html>)"              },
  };
  const auto dump = [&](const std::string& code) { return fixture.value("util.dump(" + code + ")").as<std::string>(); };
  CHECK(dump("scripts.dbl.info(metadata)") == dump("scripts.paratext.info(settings)"));
  CHECK(dump("scripts.dbl.info(metadata)") == dump("scripts.ebible.info(copyright_page)"));
  // What the files leave out stays empty
  CHECK(fixture.check("scripts.dbl.info(metadata).copyright == nil"));
  CHECK(fixture.check("select(2, scripts.dbl.info('no xml')) ~= nil"));
  CHECK(fixture.check("select(2, scripts.paratext.info('no xml')) ~= nil"));
  CHECK(fixture.check("next(scripts.ebible.info('no page')) == nil"));
}

TEST_CASE("ebible_reads_the_copyright_page_of_its_archives", "[lua]")
{
  auto fixture = test_utils::script_sandbox{"ebible_reads_the_copyright_page_of_its_archives"};
  fixture.texts = {
    // As in the archive of the Schlachter Bible 1951 of eBible.org
    {"page",
     R"(<div class="toc"><a href="https://ebible.org/deu1951">Die Schlachter-Bibel 1951</a></div></br>)"
     R"(<h1 class='dejavu_serif'>Die Schlachter-Bibel 1951</h1><h2>The Holy Bible in German</h2>)"
     R"(<p>copyright &#169; 1951 <a href="https://www.bibelgesellschaft.com/de/">Genfer Bibelgesellschaft</a><br />)"
     R"(Language: <a href='http://www.ethnologue.org/language/deu'>Deutsch (German, Standard)</a><br />)"
     R"(Translation by: Franz-Eugen Schlachter<br /></p>)"},
    // Not XML, the text is taken without the tags
    {"html", R"(<p>Public Domain <b>text<br /></p>)"},
    // The first paragraph ends without a line break, the next one is no part of it
    {"paragraphs", R"(<p class="x">Public Domain</p><p>Language: <a href='x'>English</a><br /></p>)"},
    // Entities of HTML only
    {"entities", R"(<p>copyright &copy; 2001&nbsp;Society<br></p>)"},
  };
  CHECK(fixture.check("scripts.ebible.info(page).name == 'Die Schlachter-Bibel 1951'"));
  CHECK(fixture.check("scripts.ebible.info(page).abbreviation == 'deu1951'"));
  CHECK(fixture.check("scripts.ebible.info(page).language == 'Deutsch'"));
  CHECK(fixture.check("scripts.ebible.info(page).copyright == 'copyright \\u{A9} 1951 Genfer Bibelgesellschaft'"));
  CHECK(fixture.check("scripts.ebible.info(html).copyright == 'Public Domain text'"));
  CHECK(fixture.check("scripts.ebible.info(paragraphs).copyright == 'Public Domain'"));
  CHECK(fixture.check("scripts.ebible.info(entities).copyright == 'copyright \\u{A9} 2001\\u{A0}Society'"));
}

} // namespace bibstd::lua
