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

-- util.log_debug, util.log_info, util.log_warning and util.log_error write their values to the log of the app at
-- that level. Scripts have no console, so there is no print.
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

root.interface.util = util
