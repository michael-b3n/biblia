-- Bundled script for the format USX: the XML form of USFM, a file per book. It reads the scripture files of USX
-- and offers the format to the other scripts as scripts.usx. USX: https://docs.usfm.bible/usfm/3.1.1/index.html
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local usx = {}

-- The USFM codes of the books of the app, in the order of their identifiers
local usfm_codes = {
  "GEN", "EXO", "LEV", "NUM", "DEU", "JOS", "JDG", "RUT", "1SA", "2SA", "1KI", "2KI", "1CH", "2CH", "EZR", "NEH",
  "EST", "JOB", "PSA", "PRO", "ECC", "SNG", "ISA", "JER", "LAM", "EZK", "DAN", "HOS", "JOL", "AMO", "OBA", "JON",
  "MIC", "NAM", "HAB", "ZEP", "HAG", "ZEC", "MAL", "MAT", "MRK", "LUK", "JHN", "ACT", "ROM", "1CO", "2CO", "GAL",
  "EPH", "PHP", "COL", "1TH", "2TH", "1TI", "2TI", "TIT", "PHM", "HEB", "JAS", "1PE", "2PE", "1JN", "2JN", "3JN",
  "JUD", "REV",
}
local book_identifiers = {
  "genesis", "exodus", "leviticus", "numbers", "deuteronomy", "joshua", "judges", "ruth", "samuel1", "samuel2",
  "kings1", "kings2", "chronicles1", "chronicles2", "ezra", "nehemiah", "esther", "job", "psalms", "proverbs",
  "ecclesiastes", "song_of_solomon", "isaiah", "jeremiah", "lamentations", "ezekiel", "daniel", "hosea", "joel",
  "amos", "obadiah", "jonah", "micah", "nahum", "habakkuk", "zephaniah", "haggai", "zechariah", "malachi", "matthew",
  "mark", "luke", "john", "acts", "romans", "corinthians1", "corinthians2", "galatians", "ephesians", "philippians",
  "colossians", "thessalonians1", "thessalonians2", "timothy1", "timothy2", "titus", "philemon", "hebrews", "james",
  "peter1", "peter2", "john1", "john2", "john3", "jude", "revelation",
}

-- The verses of the longest chapter, Psalm 119
local longest_chapter = 176

-- The identifiers of the books of the app by their USFM code
local books = {}
for i, code in ipairs(usfm_codes) do
  books[code] = book_identifiers[i]
end

---
--- \return the element tree of the USX document \p text as util.parse_xml
--- gives it, or nil and the reason if it is no USX document
---
function usx.parse(text)
  local tree, reason = util.parse_xml(text)
  if not tree then
    return nil, reason
  end
  if tree.name ~= "usx" then
    return nil, "no usx root element"
  end
  return tree
end

-- The white space of a scripture is the Unicode one
local blanks = {}
for _, code in ipairs({ 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x20, 0x85, 0xA0, 0x1680, 0x180E, 0x2028, 0x2029, 0x202F,
  0x205F, 0x2060, 0x3000, 0xFEFF }) do
  blanks[code] = true
end
for code = 0x2000, 0x200D do
  blanks[code] = true
end

---
--- \return true if \p s holds white space only
---
local function is_blank(s)
  -- A printable ASCII character, "!" to "~", is no white space
  if string.find(s, "[!-~]") then
    return false
  end
  for _, code in utf8.codes(s, true) do
    if not blanks[code] then
      return false
    end
  end
  return true
end

---
--- \return \p s without the white space at its start and end
---
local function trimmed(s)
  local first, last
  for position, code in utf8.codes(s, true) do
    if not blanks[code] then
      first = first or position
      last = position
    end
  end
  if not first then
    return ""
  end
  return string.sub(s, first, (utf8.offset(s, 2, last) or #s + 1) - 1)
end

local markup_escapes = { ["&"] = "&amp;", ["<"] = "&lt;", [">"] = "&gt;" }

---
--- \return \p s as text of passage markup
---
local function escaped(s)
  return (string.gsub(s, "[&<>]", markup_escapes))
end

-- Paragraph styles carrying no scripture text: headers, headings and their references, titles, labels and remarks.
-- The introduction of a book is left out too, its styles begin with "i".
local text_free = { "h", "toc", "mt", "s", "ms", "sd", "sp", "r", "sr", "mr", "d", "cl", "cd", "rem" }

-- Character styles kept as markup
local inline = { bd = "b", it = "i", em = "i", nd = "i", add = "i" }

-- Paragraph styles carrying the name of a book, most preferred first for each form
local name_styles = { "h", "toc1", "toc2", "toc3", "mt", "mt1", "mt2", "mt3" }
local name_forms = {
  abbreviation = { "toc3", "h", "toc2" },
  short_name = { "toc2", "h", "toc1" },
  long_name = { "toc1", "mt1", "mt", "toc2" },
}

---
--- \return true if \p style is \p base at any level, e.g. "s1" of "s"
---
local function is_leveled(style, base)
  return string.sub(style, 1, #base) == base and string.find(style, "^%d*$", #base + 1) ~= nil
end

---
--- \return true if paragraphs of \p style carry no scripture text
---
local function is_text_free(style)
  if string.sub(style, 1, 1) == "i" then
    return true
  end
  for _, base in ipairs(text_free) do
    if is_leveled(style, base) then
      return true
    end
  end
  return false
end

---
--- \return the markup of the content of \p node, notes left out and character styles without markup as text
---
local function serialize(node)
  local parts = {}
  for _, child in ipairs(node.children) do
    if type(child) == "string" then
      parts[#parts + 1] = escaped(child)
    elseif child.name == "char" then
      local inner = serialize(child)
      local tag = inline[child.attributes.style]
      parts[#parts + 1] = tag and ("<" .. tag .. ">" .. inner .. "</" .. tag .. ">") or inner
    elseif child.name ~= "note" then
      parts[#parts + 1] = serialize(child)
    end
  end
  return table.concat(parts)
end

---
--- \return the first number of the chapter or verse marker \p node and the highest of a range, nil if it ends one
---
local function marker_numbers(node)
  local number = node.attributes.number
  if not number then
    return nil
  end
  local highest = 0
  for n in string.gmatch(number, "%d+") do
    highest = math.max(highest, tonumber(n))
  end
  return tonumber(string.match(number, "^%s*(%d+)")) or 0, highest
end

---
--- \return the names of the book of the element tree \p tree, from the header paragraphs before its first chapter
---
local function names_of(tree)
  local headers = {}
  for _, child in ipairs(tree.children) do
    if type(child) == "table" then
      if child.name == "chapter" then
        break
      end
      local style = child.attributes.style
      if child.name == "para" and util.contains(name_styles, style) and not headers[style] then
        local name = trimmed(util.xml_text(child))
        if name ~= "" then
          headers[style] = name
        end
      end
    end
  end
  local names = {}
  for form, styles in pairs(name_forms) do
    names[form] = ""
    for _, style in ipairs(styles) do
      if headers[style] then
        names[form] = headers[style]
        break
      end
    end
  end
  return names
end

---
--- \return the book of the element tree \p tree of scripts.usx.parse or scripts.usfm.parse: its identifier `id` in the
--- app, nil for a book the app does not know, its `code`, its `names` (`abbreviation`, `short_name`, `long_name`) from
--- the headers before its first chapter, and its `verses` in the markup of `scripture.read` and their
--- `cross_references` by chapter and verse. A verse reaches from its marker up to the next one and may span several
--- paragraphs. Each verse the book numbers is in it, empty if it has no text of its own, e.g. 3 and 4 of a verse "2-4".
---
function usx.book(tree)
  local book = util.filter(tree.children, function(child) return type(child) == "table" and child.name == "book" end)[1]
  local verses, cross_references = {}, {}
  local chapter, verse
  local segments, references = {}, {}
  local paragraph, paragraphs, paragraph_filled = nil, 0, false

  -- Text of a verse replaces an empty one, a verse marked twice keeps its first text
  local function keep(number, content)
    verses[chapter] = verses[chapter] or {}
    local kept = verses[chapter][number]
    if not kept or (kept == "" and content ~= "") then
      verses[chapter][number] = content
      return true
    end
    return false
  end

  local function flush()
    if chapter and verse then
      local sections = {}
      for i, segment in ipairs(segments) do
        sections[i] = '<p data-id="' .. segment.position .. '">' .. table.concat(segment.parts) .. "</p>"
      end
      if keep(verse, table.concat(sections)) and #references > 0 then
        cross_references[chapter] = cross_references[chapter] or {}
        cross_references[chapter][verse] = references
      end
    end
    segments, references = {}, {}
  end

  local function append(content)
    if not chapter or not verse or content == "" then
      return
    end
    local last = segments[#segments]
    if last and last.paragraph == paragraph then
      last.parts[#last.parts + 1] = content
      return
    end
    -- The white space a document is laid out with is no content of its own
    if is_blank(content) then
      return
    end
    -- The verse that puts the first content into a paragraph opens it, the ones after it continue it
    local position = "undefined"
    if paragraph then
      position = paragraph_filled and "continue" or "begin"
      paragraph_filled = true
    end
    segments[#segments + 1] = { paragraph = paragraph, position = position, parts = { content } }
  end

  local visit

  local function visit_element(node)
    local name = node.name
    if name == "chapter" then
      local number = marker_numbers(node)
      if number then
        flush()
        -- Numbers start at 1, the text of a chapter 0 is left out
        chapter, verse = number > 0 and number or nil, nil
      end
    elseif name == "verse" then
      local number, last = marker_numbers(node)
      if number then
        flush()
        verse = number > 0 and number or nil
        -- The other verses of a range have no text of their own. No chapter has more
        -- verses than Psalm 119, so a range going beyond is no range of verses.
        if chapter and verse and last <= longest_chapter then
          for covered = verse + 1, last do
            keep(covered, "")
          end
        end
      end
    elseif name == "note" then
      if node.attributes.style == "x" then
        for _, child in ipairs(node.children) do
          if type(child) == "table" and child.name == "char" and child.attributes.style == "xt" then
            local text = util.xml_text(child)
            if text ~= "" then
              references[#references + 1] = text
            end
          end
        end
      end
    else
      append(serialize(node))
    end
  end

  visit = function(node)
    if type(node) == "string" then
      append(escaped(node))
    elseif node.name ~= "para" then
      visit_element(node)
    elseif not is_text_free(node.attributes.style or "") then
      -- A paragraph carrying no scripture text introduces what follows it, it is no content of a verse
      paragraphs = paragraphs + 1
      paragraph, paragraph_filled = paragraphs, false
      for _, child in ipairs(node.children) do
        visit(child)
      end
      paragraph = nil
    end
  end

  for _, child in ipairs(tree.children) do
    visit(child)
  end
  flush()
  return {
    id = book and books[book.attributes.code],
    code = book and book.attributes.code,
    names = names_of(tree),
    verses = verses,
    cross_references = cross_references,
  }
end

-- The extensions of the files of the format
usx.extensions = { ".usx" }

---
--- \return the scripture of the archive \p input of USX files, \see scripts.scripture_archive.read_scripture
---
usx["scripture.read"] = function(input)
  return scripts.scripture_archive.read_scripture(input, usx)
end

-- Shown to the user
usx.name = "USX"
usx.description = "Reads scriptures of USX files, e.g. the bundles of the Digital Bible Library."
usx.node = "usx"
return usx
