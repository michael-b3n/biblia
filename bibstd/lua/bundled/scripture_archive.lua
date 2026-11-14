-- Bundled script, offers the other scripts the archive the app reads as
-- scripts.scripture_archive, and reads its scripture for a format like scripts.usx.
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local scripture_archive = {}

---
--- \return the part of the entry name \p entry after its last "/", in lower case
---
local function base_name(entry)
  return string.lower(string.match(entry, "[^/]*$"))
end

---
--- \return true if the entry \p entry ends with one of the \p extensions, e.g. ".usx"
---
local function has_extension(entry, extensions)
  for _, extension in ipairs(extensions) do
    if util.ends_with(base_name(entry), extension) then
      return true
    end
  end
  return false
end

---
--- \return the entry of \p entries named \p file_name in any folder, the one closest to the archive root, nil if none
---
function scripture_archive.find(entries, file_name)
  local found, depth
  file_name = string.lower(file_name)
  for _, entry in ipairs(entries) do
    if base_name(entry) == file_name then
      local _, slashes = string.gsub(entry, "/", "")
      if not depth or slashes < depth then
        found, depth = entry, slashes
      end
    end
  end
  return found
end

---
--- \return the text of the entry \p name of the archive being read, without a
--- byte order mark, or nil and the reason, e.g. for a text that is no UTF-8
---
function scripture_archive.read(name)
  local content, reason = scripture_store.read_entry(name)
  if not content then
    return nil, name .. ": " .. reason
  end
  -- The readers walk the text by its characters, which fails on other encodings
  if not utf8.len(content) then
    return nil, name .. ": no UTF-8 text"
  end
  -- Without the byte order mark "\239\187\191"
  return (string.gsub(content, "^\239\187\191", ""))
end

-- The files that tell about a scripture, each with the node whose
-- info reads it. The first one the archive holds that reads is taken.
local info_files = {
  { file = "Settings.xml", node = "paratext" },
  { file = "metadata.xml", node = "dbl" },
  { file = "copr.htm", node = "ebible" },
}

---
--- \return the name, abbreviation, language and copyright of the archive of \p input:
--- from the first of its info files that reads, else it is named after its file
---
local function info_of(input)
  local info
  for _, info_file in ipairs(info_files) do
    local entry = scripture_archive.find(input.entries, info_file.file)
    local text = entry and scripture_archive.read(entry)
    info = text and scripts[info_file.node].info(text)
    if info then
      break
    end
  end
  info = info or {}
  info.name = info.name or string.gsub(input.file, "%.[^.]*$", "")
  return info
end

-- Raised when what is kept of a file changes, e.g. the markup of a reader, so the kept files are read anew
local cache_version = 1

-- The key of what read_scripture returns of a file, kept together with its verses
local summary_key = "summary"

---
--- \return the cache keeping the file \p file: its name in letters, digits and "_" and a checksum of
--- it, so names that differ only in other characters keep apart
---
local function cache_of(file)
  local checksum = 5381
  for i = 1, #file do
    checksum = (checksum * 33 + string.byte(file, i)) % 4294967296
  end
  return string.format("scripture_%s_%08x", (string.gsub(file, "[^%w_]", "_")), checksum)
end

---
--- \return the key of verse \p verse of chapter \p chapter of the book \p book in the cache of a file
---
local function verse_key(book, chapter, verse)
  return book .. "." .. chapter .. "." .. verse
end

---
--- \return what read_scripture returned of the file in the cache \p cache_name, nil if it kept none of the \p stamp
---
local function kept_output(cache_name, stamp)
  local text = cache.get(cache_name, summary_key)
  local chunk = text and load("return " .. text, "=" .. cache_name, "t", {})
  if not chunk then
    return nil
  end
  local read, kept = pcall(chunk)
  return read and type(kept) == "table" and kept.stamp == stamp and kept.output or nil
end

---
--- \return the last verse of each chapter of \p verses, the verses of scripts.usx.book by chapter and verse
---
local function verse_counts(verses)
  local counts = {}
  for chapter, chapter_verses in pairs(verses) do
    counts[chapter] = math.max(table.unpack(util.keys(chapter_verses)))
  end
  return counts
end

---
--- Add the verses of \p book, a book of scripts.usx.book, to the cache \p values: the markup of each
--- verse by its key and the cross references of a verse after its key and ".x", one per line.
---
local function add_cache_values(values, book)
  for chapter, verses in pairs(book.verses) do
    local references = book.cross_references[chapter] or {}
    for verse, content in pairs(verses) do
      local key = verse_key(book.id, chapter, verse)
      values[key] = content
      if references[verse] then
        values[key .. ".x"] = table.concat(references[verse], "\n")
      end
    end
  end
end

---
--- \return the scripture of the archive of \p input, the input of `scripture.read`, in the format \p format: a node
--- like scripts.usx with the `extensions` of its files, `parse` and `book`. None if the archive holds no file of the
--- format, an error if it holds no book of the app. Each file names its book, the first file of a book is read. The
--- verses go to a cache of the file, scripture_archive.passage gives them. A file kept at the size and time of
--- \p input is not read again unless `reload` asks for it.
---
function scripture_archive.read_scripture(input, format)
  local files = util.filter(input.entries, function(entry) return has_extension(entry, format.extensions) end)
  if #files == 0 then
    return nil
  end
  local cache_name = cache_of(input.file)
  local stamp = string.format("%d|%d|%d", cache_version, input.size, input.modified)
  local kept = not input.reload and kept_output(cache_name, stamp)
  if kept then
    return kept
  end

  local info = info_of(input)
  local output = {
    name = info.name,
    abbreviation = info.abbreviation,
    language = info.language,
    copyright = info.copyright,
    book_names = {},
    verse_counts = {},
  }
  local values = {}
  for _, file in ipairs(files) do
    local text, reason = scripture_archive.read(file)
    if not text then
      return { error = reason }
    end
    local tree, failure = format.parse(text)
    if not tree then
      return { error = file .. ": " .. failure }
    end
    local book = format.book(tree)
    -- A file of a book without verses is no book, e.g. one holding headers only
    if book.id and next(book.verses) and not output.verse_counts[book.id] then
      output.book_names[book.id] = book.names
      output.verse_counts[book.id] = verse_counts(book.verses)
      add_cache_values(values, book)
    end
  end

  -- A book left out takes the closest default versification, e.g. the Old Testament of a New Testament
  if not next(output.verse_counts) then
    return { error = "no book of the app" }
  end
  -- Only a file read to its end replaces what is kept of a file of its name, e.g. on an import
  values[summary_key] = util.dump({ stamp = stamp, output = output })
  cache.clear(cache_name)
  cache.set_all(cache_name, values)
  return output
end

---
--- \return the verse of \p input, the input of `scripture.passage` with the name of a file read by read_scripture:
--- its `markup` and `cross_references`. None if the cache of the file does not keep it.
---
function scripture_archive.passage(input)
  local cache_name = cache_of(input.name)
  local key = verse_key(input.book, input.chapter, input.verse)
  local markup = cache.get(cache_name, key)
  if not markup then
    return nil
  end
  local references = cache.get(cache_name, key .. ".x")
  return { markup = markup, cross_references = references and util.split(references, "\n") }
end

-- Shown to the user
scripture_archive.name = "Scripture archive"
scripture_archive.description = "Reads the scripture of an archive for the USX and USFM readers."
scripture_archive.node = "scripture_archive"
return scripture_archive
