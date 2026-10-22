-- Helpers for scripts, which see them as util.
local util = {}

---
--- \return the values \p ... as one text, separated by tabs
---
local function text(...)
  local texts = {}
  for i = 1, select("#", ...) do
    texts[i] = tostring((select(i, ...)))
  end
  return table.concat(texts, "\t")
end

---
--- \lua util.log_debug(...)
--- \lua util.log_info(...)
--- \lua util.log_warning(...)
--- \lua util.log_error(...)
--- Writes to the app log at that level. Scripts have no console and no print.
--- \param ... the values, written like print writes them
---
for _, level in ipairs({ "debug", "info", "warning", "error" }) do
  local log = root.system["log_" .. level]
  util["log_" .. level] = function(...)
    log(text(...))
  end
end

---
--- \return the parts of \p s between the occurrences of the plain string \p separator, empty parts are kept
---
function util.split(s, separator)
  assert(type(separator) == "string" and #separator > 0, "separator must be a non-empty string")
  local parts, first = {}, 1
  while true do
    local i, j = string.find(s, separator, first, true)
    if not i then
      parts[#parts + 1] = string.sub(s, first)
      return parts
    end
    parts[#parts + 1] = string.sub(s, first, i - 1)
    first = j + 1
  end
end

---
--- \return \p s without the white space at its start and end
---
function util.trim(s)
  return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

---
--- \return true if \p s starts with \p prefix
---
function util.starts_with(s, prefix)
  return string.sub(s, 1, #prefix) == prefix
end

---
--- \return true if \p s ends with \p suffix
---
function util.ends_with(s, suffix)
  return suffix == "" or string.sub(s, -#suffix) == suffix
end

---
--- \return the keys of \p t as list, in no particular order
---
function util.keys(t)
  local keys = {}
  for k in pairs(t) do
    keys[#keys + 1] = k
  end
  return keys
end

---
--- \return true if a value of \p t equals \p value
---
function util.contains(t, value)
  for _, v in pairs(t) do
    if v == value then
      return true
    end
  end
  return false
end

---
--- \return the list of \p f applied to every element of the list \p t
---
function util.map(t, f)
  local result = {}
  for i, v in ipairs(t) do
    result[i] = f(v)
  end
  return result
end

---
--- \return the elements of the list \p t for which \p predicate is true, in their order
---
function util.filter(t, predicate)
  local result = {}
  for _, v in ipairs(t) do
    if predicate(v) then
      result[#result + 1] = v
    end
  end
  return result
end

---
--- \return a deep copy of \p value, tables referenced more than once stay shared in the copy and cycles are kept
---
function util.copy(value)
  local copies = {}
  local function copy(v)
    if type(v) ~= "table" then
      return v
    end
    if copies[v] then
      return copies[v]
    end
    local result = {}
    copies[v] = result
    for k, item in pairs(v) do
      result[copy(k)] = copy(item)
    end
    -- getmetatable hands out the __metatable field instead, if the metatable sets one
    local metatable = getmetatable(v)
    return type(metatable) == "table" and setmetatable(result, metatable) or result
  end
  return copy(value)
end

---
--- \return readable text of \p value, tables are expanded with sorted keys
---
function util.dump(value)
  local open = {}
  local function dump(v, indent)
    if type(v) == "string" then
      return string.format("%q", v)
    end
    if type(v) ~= "table" then
      return tostring(v)
    end
    if open[v] then
      return "<cycle>"
    end
    local keys = util.keys(v)
    if #keys == 0 then
      return "{}"
    end
    table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
    open[v] = true
    local lines = {}
    for _, k in ipairs(keys) do
      lines[#lines + 1] = indent .. "  [" .. dump(k, "") .. "] = " .. dump(v[k], indent .. "  ")
    end
    open[v] = nil
    return "{\n" .. table.concat(lines, ",\n") .. "\n" .. indent .. "}"
  end
  return dump(value, "")
end

---
--- \return the value at \p path of sections separated by "." in what scripts see, nil if a section is missing
---
function util.at(path)
  local node = root.interface
  for section in string.gmatch(path, "[^.]+") do
    if type(node) ~= "table" then
      return nil
    end
    node = node[section]
  end
  return node
end

local xml_entities = { amp = "&", lt = "<", gt = ">", quot = '"', apos = "'" }

---
--- \return \p s with its entities and character references replaced, nil if one is unknown
---
local function xml_unescape(s)
  if not string.find(s, "&", 1, true) then
    return s
  end
  -- An ampersand outside of a reference
  if string.find((string.gsub(s, "&[^;&%s]*;", "")), "&", 1, true) then
    return nil
  end
  local known = true
  local result = string.gsub(s, "&([^;&%s]*);", function(reference)
    local hex, decimal = string.match(reference, "^#[xX](%x+)$"), string.match(reference, "^#(%d+)$")
    local code = hex and tonumber(hex, 16) or decimal and tonumber(decimal)
    if xml_entities[reference] then
      return xml_entities[reference]
    elseif code and code <= 0x10FFFF then
      return utf8.char(code)
    end
    known = false
  end)
  return known and result or nil
end

---
--- \return the element tree of the XML document \p text, or nil and the reason if it is not well-formed. An element
--- is a table { name, attributes, children }, its attributes by name, its children elements and texts in their order.
--- Comments, processing instructions and the document type are left out, CDATA sections are text.
---
function util.parse_xml(text)
  assert(type(text) == "string", "text must be a string")
  -- Without the byte order mark "\239\187\191", lines end in "\n" only, as XML parsers read them
  text = string.gsub(string.gsub(string.gsub(text, "^\239\187\191", ""), "\r\n", "\n"), "\r", "\n")
  local document = { children = {} }
  local open, current = { document }, document
  local position = 1
  local function fail(reason)
    return nil, string.format("%s at byte %d", reason, position)
  end
  local function add_text(s)
    local children = current.children
    if current == document then
      return
    end
    local last = #children
    if type(children[last]) == "string" then
      children[last] = children[last] .. s
    elseif s ~= "" then
      children[last + 1] = s
    end
  end
  while position <= #text do
    local tag = string.find(text, "<", position, true) or #text + 1
    if tag > position then
      local s = xml_unescape(string.sub(text, position, tag - 1))
      if not s then
        return fail("unknown reference")
      end
      add_text(s)
      position = tag
    end
    if tag > #text then
      break
    end
    -- The character after "<" tells the kind of the tag
    local kind = string.sub(text, tag + 1, tag + 1)
    if kind == "!" and string.find(text, "^<!%-%-", tag) then
      -- A comment
      position = (select(2, string.find(text, "-->", tag + #"<!--", true)) or #text) + 1
    elseif kind == "?" then
      -- A processing instruction
      position = (select(2, string.find(text, "?>", tag + #"<?", true)) or #text) + 1
    elseif kind == "!" and string.find(text, "^<!%[CDATA%[", tag) then
      local start = tag + #"<![CDATA["
      local finish = string.find(text, "]]>", start, true)
      if not finish then
        return fail("unclosed CDATA section")
      end
      add_text(string.sub(text, start, finish - 1))
      position = finish + #"]]>"
    elseif kind == "!" then
      -- The document type, its internal subset ends with "]>"
      local subset = string.find(text, "^<![^>[]*%[", tag)
      position = (select(2, string.find(text, subset and "]%s*>" or ">", tag + #"<!")) or #text) + 1
    elseif kind == "/" then
      -- An end tag
      local _, finish, name = string.find(text, "^</([^%s>]+)%s*>", tag)
      if not name or name ~= current.name then
        return fail("unexpected end tag")
      end
      open[#open] = nil
      current = open[#open]
      position = finish + 1
    else
      -- A start tag
      local _, finish, name = string.find(text, "^<([^%s/>]+)", tag)
      if not name then
        return fail("invalid tag")
      end
      local element = { name = name, attributes = {}, children = {} }
      position = finish + 1
      while true do
        local _, attribute_end, key, quote = string.find(text, "^%s+([^%s=/>]+)%s*=%s*([\"'])", position)
        if not key then
          break
        end
        local value_end = string.find(text, quote, attribute_end + 1, true)
        if not value_end then
          return fail("unclosed attribute value")
        end
        -- Attribute values have their white space normalized to spaces
        local value = string.sub(text, attribute_end + 1, value_end - 1)
        if string.find(value, "[\t\n]") then
          value = string.gsub(value, "[\t\n]", " ")
        end
        value = xml_unescape(value)
        if not value then
          return fail("unknown reference")
        end
        element.attributes[key] = value
        position = value_end + 1
      end
      local _, tag_end, empty = string.find(text, "^%s*(/?)>", position)
      if not tag_end then
        return fail("invalid tag")
      end
      if current == document and document.root then
        return fail("second root element")
      end
      document.root = document.root or element
      local children = current.children
      children[#children + 1] = element
      if empty == "" then
        open[#open + 1] = element
        current = element
      end
      position = tag_end + 1
    end
  end
  if current ~= document then
    return fail("unclosed element")
  end
  if not document.root then
    return fail("no root element")
  end
  return document.root
end

---
--- \return the text of the element \p element of util.parse_xml and its descendants, with
--- \p without_layout the texts of white space only left out, which lay a document out
---
function util.xml_text(element, without_layout)
  local parts = {}
  local function add(node)
    for _, child in ipairs(node.children) do
      if type(child) == "table" then
        add(child)
      elseif not (without_layout and string.find(child, "^[ \t\n]*$")) then
        parts[#parts + 1] = child
      end
    end
  end
  add(element)
  return table.concat(parts)
end

root.interface.util = util
