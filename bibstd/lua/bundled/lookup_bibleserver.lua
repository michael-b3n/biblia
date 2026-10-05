-- Bundled script, builds the urls opening bible references on bibleserver.com.
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

-- Book names in the urls of bibleserver.com, the German ones work for every translation
local books = {
  genesis = "1.Mose", exodus = "2.Mose", leviticus = "3.Mose", numbers = "4.Mose", deuteronomy = "5.Mose",
  joshua = "Josua", judges = "Richter", ruth = "Rut", samuel1 = "1.Samuel", samuel2 = "2.Samuel",
  kings1 = "1.Könige", kings2 = "2.Könige", chronicles1 = "1.Chronik", chronicles2 = "2.Chronik",
  ezra = "Esra", nehemiah = "Nehemia", esther = "Ester", job = "Hiob", psalms = "Psalm", proverbs = "Sprüche",
  ecclesiastes = "Prediger", song_of_solomon = "Hoheslied", isaiah = "Jesaja", jeremiah = "Jeremia",
  lamentations = "Klagelieder", ezekiel = "Hesekiel", daniel = "Daniel", hosea = "Hosea", joel = "Joel",
  amos = "Amos", obadiah = "Obadja", jonah = "Jona", micah = "Micha", nahum = "Nahum", habakkuk = "Habakuk",
  zephaniah = "Zefanja", haggai = "Haggai", zechariah = "Sacharja", malachi = "Maleachi",
  matthew = "Matthäus", mark = "Markus", luke = "Lukas", john = "Johannes", acts = "Apostelgeschichte",
  romans = "Römer", corinthians1 = "1.Korinther", corinthians2 = "2.Korinther", galatians = "Galater",
  ephesians = "Epheser", philippians = "Philipper", colossians = "Kolosser",
  thessalonians1 = "1.Thessalonicher", thessalonians2 = "2.Thessalonicher", timothy1 = "1.Timotheus",
  timothy2 = "2.Timotheus", titus = "Titus", philemon = "Philemon", hebrews = "Hebräer", james = "Jakobus",
  peter1 = "1.Petrus", peter2 = "2.Petrus", john1 = "1.Johannes", john2 = "2.Johannes", john3 = "3.Johannes",
  jude = "Judas", revelation = "Offenbarung",
}

-- Abbreviations of the translations on bibleserver.com
local translations = {
  "DBU", "ELB", "ESV", "EU", "GNB", "HFA", "KJV", "LUT", "MENG", "NeÜ", "NGÜ", "NIRV", "NIV", "NLB", "SLT", "VXB", "ZB",
}

-- Translations looked up until the user chooses, by the language the user prefers
local defaults = {
  english = { "NIV", "ESV" },
  german = { "NGÜ", "ELB" },
}

---
--- \return \p text percent encoded, so umlauts reach bibleserver.com as UTF-8
---
local function encode(text)
  return (string.gsub(text, "[^%w%.%-_~]", function(c) return string.format("%%%02X", string.byte(c)) end))
end

---
--- \return the url showing the verses of \p input in its translations, nil without a translation or a book
---
local function url(input)
  local known = util.filter(input.translations, function(name) return util.contains(translations, name) end)
  local book = books[input.book]
  if #known == 0 or not book then
    return nil
  end
  local verses = tostring(input.verse_begin)
  if input.verse_end > input.verse_begin then
    verses = verses .. "-" .. input.verse_end
  end
  -- Several translations are shown side by side
  local names = table.concat(util.map(known, encode), ".")
  return { url = "https://www.bibleserver.com/" .. names .. "/" .. encode(book) .. input.chapter .. "%2C" .. verses }
end

return {
  id = "lookup_bibleserver",
  -- Shown to the user
  name = "Bibleserver",
  functions = {
    ---
    --- \return the translations to choose from and the ones of the language \p input.language
    ---
    ["lookup.translations"] = function(input)
      return { names = translations, defaults = defaults[input.language] }
    end,

    ["lookup.url"] = url,
  },
}
