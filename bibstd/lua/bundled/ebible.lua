-- Bundled script, offers the other scripts the files of eBible.org as scripts.ebible: the copyright page of its
-- archives. https://ebible.org/
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local ebible = {}

---
--- \return the text of the HTML \p fragment without its tags and the white space at its ends, nil if it is empty
---
local function text_of(fragment)
  if not fragment then
    return nil
  end
  local tree = util.parse_xml("<text>" .. fragment .. "</text>")
  -- HTML is no XML, e.g. with "&nbsp;", then only its tags are left out
  local text = util.trim(tree and util.xml_text(tree) or (string.gsub(fragment, "<[^>]*>", "")))
  return text ~= "" and text or nil
end

---
--- \return the `name`, `abbreviation`, `language` and `copyright` of the copyright page `copr.htm` \p text of an
--- archive, each nil if the page has none
---
function ebible.info(text)
  local language = text_of(string.match(text, "Language:%s*<a[^>]*>(.-)</a>"))
  return {
    name = text_of(string.match(text, "<h1[^>]*>(.-)</h1>")),
    -- The identifier of the translation in the link to its page, e.g. "deu1951" of https://ebible.org/deu1951
    abbreviation = string.match(text, "class=[\"']toc[\"'][^>]*>%s*<a href=[\"'][^\"']-/([%w_%-]+)[\"']"),
    -- Its own name, e.g. "Deutsch" of "Deutsch (German, Standard)"
    language = language and util.trim((string.gsub(language, "%(.*%)$", ""))),
    -- The first line of the first paragraph, e.g. "copyright 1951 Genfer Bibelgesellschaft"
    copyright = text_of(string.match(text, "<p[^>]*>(.-)<br")),
  }
end

-- Shown to the user, other scripts see the functions above at scripts.ebible
ebible.name = "eBible"
ebible.node = "ebible"
return ebible
