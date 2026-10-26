-- Bundled script, offers the other scripts the archive the app reads as scripts.scripture_archive, and reads the
-- scripture of one for a format like scripts.usx.
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
--- \return the text of the entry \p name of the archive being read without a byte order mark, or nil and the reason,
--- e.g. for a text that is no UTF-8
---
function scripture_archive.read(name)
  local content, reason = workflow.scripture_store.read_entry(name)
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

-- The files telling about a scripture and the node whose info reads them, the first one an archive holds that reads
-- is taken
local info_files = {
  { file = "Settings.xml", node = "paratext" },
  { file = "metadata.xml", node = "dbl" },
  { file = "copr.htm", node = "ebible" },
}

---
--- \return the name, abbreviation, language and copyright of the archive of \p input: from the first of its info files
--- that reads, else named after its file
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

---
--- \return the scripture of the archive of \p input, the input of `scripture.read`, in the format \p format: a node
--- like scripts.usx with the `extensions` of its files, `parse` and `book`. None if the archive holds no file of the
--- format, an error if it holds no book of the app. Each file names its book, the first file of a book is read.
---
function scripture_archive.read_scripture(input, format)
  local files = util.filter(input.entries, function(entry) return has_extension(entry, format.extensions) end)
  if #files == 0 then
    return nil
  end
  local info = info_of(input)
  local output = {
    name = info.name,
    abbreviation = info.abbreviation,
    language = info.language,
    copyright = info.copyright,
    book_names = {},
    verses = {},
    cross_references = {},
  }

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
    if book.id and next(book.verses) and not output.verses[book.id] then
      output.book_names[book.id] = book.names
      output.verses[book.id] = book.verses
      if next(book.cross_references) then
        output.cross_references[book.id] = book.cross_references
      end
    end
  end

  -- A chapter left out takes the default versification closest to the others, e.g. the Old Testament of a New Testament
  if not next(output.verses) then
    return { error = "no book of the app" }
  end
  return output
end

-- Shown to the user
scripture_archive.name = "Scripture archive"
scripture_archive.description = "Reads the scripture of an archive for the USX and USFM readers."
scripture_archive.node = "scripture_archive"
return scripture_archive
