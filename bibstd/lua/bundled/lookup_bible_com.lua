-- Bundled script, builds the urls opening bible references on bible.com.
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

-- Book codes in the urls of bible.com, the same for every translation
local codes = {
  genesis = "GEN", exodus = "EXO", leviticus = "LEV", numbers = "NUM", deuteronomy = "DEU", joshua = "JOS",
  judges = "JDG", ruth = "RUT", samuel1 = "1SA", samuel2 = "2SA", kings1 = "1KI", kings2 = "2KI",
  chronicles1 = "1CH", chronicles2 = "2CH", ezra = "EZR", nehemiah = "NEH", esther = "EST", job = "JOB",
  psalms = "PSA", proverbs = "PRO", ecclesiastes = "ECC", song_of_solomon = "SNG", isaiah = "ISA",
  jeremiah = "JER", lamentations = "LAM", ezekiel = "EZK", daniel = "DAN", hosea = "HOS", joel = "JOL",
  amos = "AMO", obadiah = "OBA", jonah = "JON", micah = "MIC", nahum = "NAM", habakkuk = "HAB",
  zephaniah = "ZEP", haggai = "HAG", zechariah = "ZEC", malachi = "MAL", matthew = "MAT", mark = "MRK",
  luke = "LUK", john = "JHN", acts = "ACT", romans = "ROM", corinthians1 = "1CO", corinthians2 = "2CO",
  galatians = "GAL", ephesians = "EPH", philippians = "PHP", colossians = "COL", thessalonians1 = "1TH",
  thessalonians2 = "2TH", timothy1 = "1TI", timothy2 = "2TI", titus = "TIT", philemon = "PHM", hebrews = "HEB",
  james = "JAS", peter1 = "1PE", peter2 = "2PE", john1 = "1JN", john2 = "2JN", john3 = "3JN", jude = "JUD",
  revelation = "REV",
}

-- Versions of the translations in the urls of bible.com, by their abbreviation there
local versions = {
  DELUT = 51, ELB = 57, ELB71 = 58, HFA = 73, NGU2011 = 108, SCH2000 = 157,
  ESV = 59, KJV = 1, NASB1995 = 100, NIV = 111, NKJV = 114, NLT = 116,
}

-- Translation looked up until the user chooses, by the language the user prefers
local defaults = {
  english = { "NIV" },
  german = { "ELB" },
}

local translations = util.keys(versions)
table.sort(translations)

---
--- \return the url showing the verses of \p input, nil without a translation or a book
---
local function url(input)
  -- bible.com shows the verses of one translation, the first one chosen
  local translation = util.filter(input.translations, function(name) return versions[name] ~= nil end)[1]
  local code = codes[input.book]
  if not translation or not code then
    return nil
  end
  local verses = tostring(input.verse_begin)
  if input.verse_end > input.verse_begin then
    verses = verses .. "-" .. input.verse_end
  end
  local passage = table.concat({ code, input.chapter, verses, translation }, ".")
  return { url = "https://www.bible.com/bible/" .. versions[translation] .. "/" .. passage }
end

return {
  id = "lookup_bible_com",
  -- Shown to the user
  name = "bible.com",
  functions = {
    ---
    --- \return the translations to choose from and the one of the language \p input.language
    ---
    ["lookup.translations"] = function(input)
      return { names = translations, defaults = defaults[input.language] }
    end,

    ["lookup.url"] = url,
  },
}
