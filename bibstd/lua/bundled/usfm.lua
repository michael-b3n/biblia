-- Bundled script for the format USFM: text with backslash markers, a file per book. It reads the scripture
-- files of USFM and offers the format to the other scripts as scripts.usfm. It gives the element tree of
-- scripts.usx, so books of both formats are read alike. USFM: https://docs.usfm.bible/usfm/3.1.1/index.html
-- Writing scripts: doc/lua_scripts.md of the VerseLens repository.

local usfm = {}

-- USFM markers of paragraphs by their name without level
local usfm_paragraphs = {}
for _, name in ipairs({
  "ide", "sts", "rem", "h", "toc", "toca", "imt", "is", "ip", "ipi", "im", "imi", "ipq", "imq", "ipr", "ipc", "iq",
  "ili", "ib", "iot", "io", "iex", "imte", "ie", "mt", "mte", "cl", "cd", "ms", "mr", "s", "sr", "r", "d", "sp", "sd",
  "p", "m", "po", "cls", "pr", "pc", "pm", "pmo", "pmc", "pmr", "pi", "mi", "lit", "nb", "b", "ph", "q", "qr", "qc",
  "qa", "qm", "qd", "lh", "li", "lf", "lim", "cp",
}) do
  usfm_paragraphs[name] = true
end

-- USX elements holding elements only, their white space is layout
local usfm_containers = { usx = true, table = true, row = true, sidebar = true, periph = true }

-- USFM markers of notes, of the character styles opening a section of a
-- note, which ends at the next one, and of table cells by their alignment
local usfm_notes = { f = true, fe = true, ef = true, efe = true, x = true, ex = true }
local usfm_note_sections = {}
for _, name in ipairs({
  "fr", "fq", "fqa", "fk", "ft", "fl", "fw", "fp", "fv", "fdc", "fm", "xo", "xop", "xk", "xq", "xt", "xta", "xot",
  "xnt", "xdc",
}) do
  usfm_note_sections[name] = true
end
local usfm_cells = { th = "start", thr = "end", thc = "center", tc = "start", tcr = "end", tcc = "center" }

-- Escaped characters and the ones standing for others, "~" is a no-break space, "\194\160" in UTF-8
-- An escaped bar is \1 until the attribute lists, which start with a bar, are read
local usfm_escapes = { ["\\\\"] = "\\", ["\\~"] = "~", ["\\/"] = "/", ["\\|"] = "\1", ["~"] = "\194\160" }

-- Default attributes of USFM markers, and the attributes USX names differently
local usfm_defaults = { w = "lemma", rb = "gloss", xt = "link-href", jmp = "link-href", ref = "loc" }
local usx_names = { src = "file" }

---
--- \return the text \p s of a USFM document as content
---
local function usfm_text(s)
  return (string.gsub(s, "\\?[\\~/|]", usfm_escapes))
end

---
--- \return the attributes of the attribute list \p s of the marker \p name, e.g. 'lemma="grace"' or a default one
---
local function usfm_attributes(name, s)
  local attributes = {}
  for key, value in string.gmatch(s, "([%w_%-]+)%s*=%s*\"([^\"]*)\"") do
    attributes[usx_names[key] or key] = value
  end
  if next(attributes) == nil and string.find(s, "%S") and usfm_defaults[name] then
    attributes[usfm_defaults[name]] = util.trim(s)
  end
  return attributes
end

---
--- \return the element tree of the USFM document \p text as scripts.usx.parse gives it for USX, or nil
--- and the reason if it identifies no book. A usx root holds book, chapter, para, table, sidebar and
--- periph elements, paragraphs hold verse, char, ref, note, ms, figure and optbreak elements. Markers
--- USFM does not define are character styles. Chapters and verses have no sid and eid, as in USX 2.
---
function usfm.parse(text)
  assert(type(text) == "string", "text must be a string")
  -- Without the byte order mark "\239\187\191", lines end in "\n" only
  text = string.gsub(string.gsub(text, "^\239\187\191", ""), "\r\n?", "\n")
  local document = { name = "usx", attributes = {}, children = {} }
  -- The open elements, the root first
  local open = { document }
  local position = 1

  local function top()
    return open[#open]
  end
  local function element(name, attributes)
    return { name = name, attributes = attributes, children = {} }
  end
  local function add(node)
    local parent = top()
    local children = parent.children
    if type(node) == "string" then
      -- Containers of elements only have their white space for layout
      if node == "" or (usfm_containers[parent.name] and string.find(node, "^%s*$")) then
        return
      end
      if type(children[#children]) == "string" then
        children[#children] = children[#children] .. node
        return
      end
    end
    children[#children + 1] = node
  end
  local function push(node)
    add(node)
    open[#open + 1] = node
  end
  -- Close the open elements down to the first one \p stop is true for, that one too unless \p keep
  local function close_until(stop, keep)
    for i = #open, 2, -1 do
      if stop(open[i]) then
        for j = #open, keep and i + 1 or i, -1 do
          local node = open[j]
          local children = node.children
          -- White space at the end of a paragraph or cell is no content
          local trims = node.name == "para" or node.name == "cell" or node.name == "book"
          if trims and type(children[#children]) == "string" then
            children[#children] = string.gsub(children[#children], "%s+$", "")
            if children[#children] == "" then
              children[#children] = nil
            end
          end
          open[j] = nil
        end
        return true
      end
    end
    return false
  end
  local function close_paragraph()
    close_until(function(node) return node.name == "para" or node.name == "book" or node.name == "table" end)
  end
  local function close_to_root()
    close_until(function(node) return node == open[2] end)
  end
  -- Close all down to an open sidebar or peripheral division
  local function close_all()
    if not close_until(function(node) return node.name == "sidebar" or node.name == "periph" end, true) then
      close_to_root()
    end
  end
  -- The next word, e.g. a number, and the white space after it
  local function word()
    local _, finish, value = string.find(text, "^%s*(%S+)%s?", position)
    if not value then
      return nil
    end
    position = finish + 1
    return value
  end
  -- The text up to the closing marker \p marker
  local function up_to_closing(marker)
    local finish = string.find(text, "\\" .. marker .. "*", position, true)
    local content = string.sub(text, position, (finish or #text + 1) - 1)
    position = finish and finish + #marker + 2 or #text + 1
    return content
  end

  while position <= #text do
    local marker = string.find(text, "\\", position, true) or #text + 1
    -- Escaped characters belong to the text
    while marker <= #text and string.find(text, "^\\[\\~/|]", marker) do
      marker = string.find(text, "\\", marker + 2, true) or #text + 1
    end
    if marker > position then
      -- Optional line breaks are elements of their own
      local s, first = string.sub(text, position, marker - 1), 1
      for before, after in string.gmatch(s, "()//()") do
        add(usfm_text((string.gsub(string.sub(s, first, before - 1), "%s+", " "))))
        add(element("optbreak", {}))
        first = after
      end
      add(usfm_text((string.gsub(string.sub(s, first), "%s+", " "))))
      position = marker
    end
    if marker > #text then
      break
    end
    local _, finish, plus, name, level, milestone, closing =
      string.find(text, "^\\(%+?)(%a+)(%d*)(%-?[se]?)(%*?)", marker)
    if not name then
      -- A backslash on its own
      add("\\")
      position = marker + 1
    else
      local style = name .. level .. milestone
      position = finish + 1
      local _, milestone_end, list = string.find(text, "^([^\\\n]*)\\%*", position)
      local children = top().children
      local last = children[#children]
      if closing == "" and milestone_end then
        -- A milestone, e.g. \qt-s |who="Pilate"\*
        position = milestone_end + 1
        local attributes = usfm_attributes(style, string.match(list, "|(.*)$") or "")
        attributes.style = style
        add(element("ms", attributes))
      elseif closing == "*" then
        close_until(function(node) return (node.attributes.style or node.name) == style end)
      elseif name == "id" then
        close_all()
        push(element("book", { code = word() or "", style = "id" }))
      elseif name == "usfm" then
        close_paragraph()
        document.attributes.version = word()
      elseif name == "c" then
        close_all()
        add(element("chapter", { number = word() or "", style = "c" }))
      elseif name == "cp" and type(last) == "table" and last.name == "chapter" then
        last.attributes.pubnumber = word()
      elseif (name == "ca" or name == "va" or name == "vp") and type(last) == "table"
          and last.name == (name == "ca" and "chapter" or "verse") then
        -- Another numbering of the chapter or verse before it
        last.attributes[name == "vp" and "pubnumber" or "altnumber"] = util.trim(up_to_closing(style))
      elseif name == "v" then
        -- Verses are no part of character styles or notes, those still open end here
        while top().name == "char" or top().name == "ref" or top().name == "note" do
          open[#open] = nil
        end
        add(element("verse", { number = word() or "", style = "v" }))
      elseif name == "esb" then
        close_all()
        push(element("sidebar", { style = "esb" }))
      elseif name == "esbe" then
        close_until(function(node) return node.name == "sidebar" end)
      elseif name == "periph" then
        close_until(function(node) return node.name == "periph" end)
        close_all()
        local line = string.match(text, "^[^\n]*", position)
        position = position + #line
        local title, list = string.match(line, "^(.-)|(.*)$")
        local attributes = usfm_attributes(style, list or "")
        attributes.alt = util.trim(title or line)
        push(element("periph", attributes))
      elseif name == "cat" then
        top().attributes.category = util.trim(up_to_closing(style))
      elseif name == "fig" then
        position = string.match(text, "^%s*()", position)
        local content = up_to_closing(style)
        local caption, list = string.match(content, "^(.-)|(.*)$")
        local attributes = usfm_attributes(style, list or "")
        attributes.style = "fig"
        caption = usfm_text(caption or content)
        add({ name = "figure", attributes = attributes, children = caption ~= "" and { caption } or {} })
      elseif name == "tr" and plus == "" then
        if not close_until(function(node) return node.name == "table" end, true) then
          close_paragraph()
          push(element("table", {}))
        end
        push(element("row", { style = "tr" }))
      elseif usfm_cells[name] and plus == "" then
        close_until(function(node) return node.name == "row" end, true)
        push(element("cell", { style = style, align = usfm_cells[name] }))
        position = string.match(text, "^%s*()", position)
      elseif usfm_paragraphs[name] and plus == "" then
        close_paragraph()
        push(element("para", { style = style }))
        position = string.match(text, "^%s*()", position)
      elseif usfm_notes[name] and plus == "" then
        push(element("note", { caller = word() or "+", style = style }))
      else
        -- A character style. One that opens a section of a note ends the section before it.
        if usfm_note_sections[name] and plus == "" then
          close_until(function(node) return node.name == "note" end, true)
        end
        push(name == "ref" and element("ref", {}) or element("char", { style = style }))
        position = string.match(text, "^%s*()", position)
      end
    end
  end
  close_to_root()

  ---
  --- Move the attribute list at the end of each character style of \p node
  --- and below into its attributes, then turn the escaped bars back into bars
  ---
  local function move_attributes(node)
    local last = node.children[#node.children]
    if (node.name == "char" or node.name == "ref") and type(last) == "string" then
      local content, list = string.match(last, "^(.-)|(.*)$")
      if content then
        node.children[#node.children] = content ~= "" and content or nil
        for key, value in pairs(usfm_attributes(node.attributes.style or node.name, list)) do
          node.attributes[key] = value
        end
      end
    end
    for key, value in pairs(node.attributes) do
      node.attributes[key] = string.gsub(value, "\1", "|")
    end
    for i, child in ipairs(node.children) do
      if type(child) == "table" then
        move_attributes(child)
      else
        node.children[i] = string.gsub(child, "\1", "|")
      end
    end
  end
  move_attributes(document)
  local book = document.children[1]
  if not (type(book) == "table" and book.name == "book") then
    return nil, "no book identification"
  end
  return document
end

---
--- \return the book of the element tree \p tree of scripts.usfm.parse, \see scripts.usx.book
---
function usfm.book(tree)
  return scripts.usx.book(tree)
end

-- The extensions of the files of the format
usfm.extensions = { ".usfm", ".sfm" }

---
--- \return the scripture of the archive \p input of USFM files, \see scripts.scripture_archive.read_scripture
---
usfm["scripture.read"] = function(input)
  return scripts.scripture_archive.read_scripture(input, usfm)
end

-- Shown to the user
usfm.name = "USFM"
usfm.description = "Reads scriptures of USFM files, e.g. the archives of eBible.org."
usfm.node = "usfm"
return usfm
