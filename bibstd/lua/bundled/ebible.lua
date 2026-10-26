-- Bundled script, offers the other scripts the files of eBible.org as scripts.ebible: the copyright page of its
-- archives. https://ebible.org/
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local ebible = {}

-- The entities of HTML pages XML does not know, as character references
local html_entities = { nbsp = "&#160;", copy = "&#169;", reg = "&#174;", ndash = "&#8211;", mdash = "&#8212;" }

---
--- \return the text of the HTML \p fragment without its tags and the white space at its ends, nil if it is empty
---
local function text_of(fragment)
  if not fragment then
    return nil
  end
  local xml = string.gsub(fragment, "&(%a+);", function(name) return html_entities[name] end)
  -- HTML is no XML, e.g. with an unclosed "<br>", then its tags are left out first
  local untagged = string.gsub(xml, "<[^>]*>", "")
  local tree = util.parse_xml("<text>" .. xml .. "</text>") or util.parse_xml("<text>" .. untagged .. "</text>")
  local text = util.trim(tree and util.xml_text(tree) or untagged)
  return text ~= "" and text or nil
end

---
--- \return the HTML \p paragraph up to its first line break, nil if there is none
---
local function first_line(paragraph)
  return paragraph and (string.match(paragraph, "^(.-)<br") or paragraph)
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
    copyright = text_of(first_line(string.match(text, "<p%f[%s>][^>]*>(.-)</p>"))),
  }
end

-- Shown to the user, other scripts see the functions above at scripts.ebible
ebible.name = "eBible"
ebible.description = "Reads the copyright page of the archives of eBible.org."
ebible.node = "ebible"
return ebible
