---
--- Writes the reference of what the app offers scripts, from the comments tagged \lua: a /// block above a
--- register_function, a register_setting, a setting function of lua::state or a manifest struct, and every ---
--- block above a function of util. Names, parameters and types come from the code, the comment gives the
--- description and a \param or \return line for each parameter, input or output.
--- Usage: lua gen_lua_doc.lua [--check] <output> <source>...
---

local check = arg[1] == "--check"
local first = check and 2 or 1
local output = arg[first]
local sources = table.move(arg, first + 1, #arg, 1, {})

local errors = {}

---
--- Record the error \p message of \p file, nothing is written once there is one
---
local function fail(file, message)
  errors[#errors + 1] = string.format("%s: %s", file, message)
end

---
--- \return the content of the file at \p path with "\n" line endings, nil if there is none
---
local function read(path)
  -- Binary, text mode would convert line endings on Windows only
  local file = io.open(path, "rb")
  if not file then
    return nil
  end
  local content = file:read("a"):gsub("\r\n", "\n")
  file:close()
  return content
end

---
--- \return \p s without the white space at its start and end
---
local function trim(s)
  return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

---
--- \return the parts of \p s between its commas outside of brackets
---
local function split_top(s)
  local parts, depth, start = {}, 0, 1
  for i = 1, #s do
    local c = s:sub(i, i)
    if c:match("[<%(%[{]") then
      depth = depth + 1
    elseif c:match("[>%)%]}]") then
      depth = depth - 1
    elseif c == "," and depth == 0 then
      parts[#parts + 1] = trim(s:sub(start, i - 1))
      start = i + 1
    end
  end
  if s:sub(start):match("%S") then
    parts[#parts + 1] = trim(s:sub(start))
  end
  return parts
end

---
--- \return the type a script sees for the C++ type \p cpp, nil if value_cast converts none like it. With \p erased,
--- \p cpp is the type of a setting, which scripts see type erased.
---
local function lua_type(cpp, erased)
  cpp = trim(cpp):gsub("^const%s+", ""):gsub("[%s&]+$", "")
  local wrapper, inner = cpp:match("^std::(%a+)(%b<>)$")
  if wrapper then
    local types = {}
    for i, part in ipairs(split_top(inner:sub(2, -2))) do
      types[i] = lua_type(part, erased)
      if not types[i] then
        return nil
      end
    end
    if wrapper == "optional" and #types == 1 then
      return types[1] .. "?"
    elseif wrapper == "vector" and #types == 1 then
      return types[1] .. "[]"
    elseif wrapper == "map" and #types == 2 then
      return string.format("table<%s, %s>", types[1], types[2])
    end
    return nil
  end
  if cpp == "bool" then
    return "boolean"
  elseif cpp == "std::string" or cpp == "std::filesystem::path" then
    return "string"
  elseif cpp == "int" or cpp == "std::size_t" or cpp:match("^std::u?int%d+_t$") or cpp:match("^std::chrono::%a+$") then
    return "integer"
  elseif cpp == "float" or cpp == "double" then
    return "number"
  elseif cpp == "erased_type" then
    return "T"
  elseif erased and cpp:match("^[%w_:]+$") then
    -- A setting holds the basic types above or an enum, which it erases to its name
    return "string"
  end
  return nil
end

---
--- \return the set of the values of \p list
---
local function set_of(list)
  local set = {}
  for _, v in ipairs(list) do
    set[v] = true
  end
  return set
end

---
--- Fail for each entry of \p documented naming none of \p names, and each of \p names without one if \p complete
---
local function check_names(file, what, tag, documented, names, complete)
  local allowed, seen = set_of(names), {}
  for _, entry in ipairs(documented) do
    if not allowed[entry.name] then
      fail(file, string.format("%s: \\%s %s names nothing of the code", what, tag, entry.name))
    end
    seen[entry.name] = true
  end
  for _, name in ipairs(complete and names or {}) do
    if not seen[name] then
      fail(file, string.format("%s: no \\%s for %s", what, tag, name))
    end
  end
end

---
--- \return the comment \p lines of \p what as Markdown. A line starting with \param (or \p) documents one of
--- \p inputs, one starting with \return the result, or one of \p outputs if given. Each must be documented if
--- \p complete.
---
local function describe(file, what, lines, inputs, outputs, complete)
  local chunks, params, returns, current = {}, {}, {}, nil
  for _, line in ipairs(lines) do
    line = trim(line)
    local tag, rest = line:match("^\\(%a+)%s+(.*)$")
    if line == "" then
      current = nil
    elseif tag == "param" or tag == "p" then
      local name, text = rest:match("^(%S+)%s*(.*)$")
      current = { name = name, text = text }
      params[#params + 1] = current
    elseif tag == "return" then
      local name, text = rest:match("^(%S+)%s*(.*)$")
      current = outputs and { name = name, text = text } or { text = rest }
      returns[#returns + 1] = current
    elseif line:match("^%- ") or not current then
      current = { text = line, item = line:match("^%- ") ~= nil }
      chunks[#chunks + 1] = current
    else
      current.text = current.text .. " " .. line
    end
  end
  if #chunks == 0 and #returns == 0 then
    fail(file, what .. " has no description")
  end
  check_names(file, what, "param", params, inputs, complete)
  if outputs then
    check_names(file, what, "return", returns, outputs, complete)
  elseif #returns > 1 then
    fail(file, what .. " has more than one \\return")
  end

  local parts, text = {}, ""
  for i, chunk in ipairs(chunks) do
    text = text .. (i == 1 and "" or (chunk.item and chunks[i - 1].item) and "\n" or "\n\n") .. chunk.text
  end
  parts[#parts + 1] = text ~= "" and text or nil
  local function list(title, entries)
    local items = {}
    for i, entry in ipairs(entries) do
      items[i] = string.format("- `%s`: %s", entry.name, entry.text)
    end
    parts[#parts + 1] = #items > 0 and string.format("**%s**\n\n%s", title, table.concat(items, "\n")) or nil
  end
  list(outputs and "Input" or "Parameters", params)
  if outputs then
    list("Output", returns)
  elseif returns[1] then
    parts[#parts + 1] = "**Returns** " .. returns[1].text
  end

  local names = set_of(inputs)
  for _, output in ipairs(outputs or {}) do
    names[output] = true
  end
  local function name(n)
    if not names[n] then
      fail(file, string.format("%s: \\p %s names nothing of the code", what, n))
    end
    return "`" .. n .. "`"
  end
  text = table.concat(parts, "\n\n")
  return (text:gsub("\\p%s+(%.%.%.)", name):gsub("\\p%s+([%a_][%w_]*)", name):gsub("\\see%s+", "see "))
end

---
--- \return the comment blocks of \p text, each with its lines without \p prefix and the text after it
---
local function blocks(text, prefix)
  local result, current = {}, nil
  local position = 1
  for line, next_position in text:gmatch("([^\n]*)\n?()") do
    local content = line:match("^%s*" .. prefix .. " ?(.*)$")
    if content and not content:match("^[/%-]") then
      current = current or { lines = {} }
      current.lines[#current.lines + 1] = content
    elseif current then
      current.rest = text:sub(position)
      result[#result + 1] = current
      current = nil
    end
    position = next_position
    if position > #text then
      break
    end
  end
  return result
end

local cpp_sources = {}
for _, source in ipairs(sources) do
  if source:match("%.[ch]pp$") then
    cpp_sources[#cpp_sources + 1] = { path = source, text = assert(read(source), source) }
  end
end
local all_cpp = {}
for i, source in ipairs(cpp_sources) do
  all_cpp[i] = source.text
end
all_cpp = table.concat(all_cpp, "\n")

local functions, settings, setting_functions, manifests, utils = {}, {}, {}, {}, {}

---
--- \return the names of the lambda \p parameters of \p what and the parameters typed for scripts
---
local function typed_parameters(file, what, parameters)
  local names, typed = {}, {}
  for i, parameter in ipairs(split_top(parameters:sub(2, -2))) do
    local cpp, name = parameter:match("^(.-)%s*([%a_][%w_]*)$")
    local type = cpp and lua_type(cpp)
    if not type then
      fail(file, string.format("%s: no Lua type for the parameter \"%s\"", what, parameter))
    end
    names[i] = name or parameter
    typed[i] = string.format("%s: %s", name, type)
  end
  return names, table.concat(typed, ", ")
end

---
--- Add the entry of the C++ comment \p block of \p file, tagged \lua
---
local function add_cpp(file, block, documented)
  local lines = {}
  for i, line in ipairs(block.lines) do
    lines[i] = line:gsub("\\lua%s*", "", 1)
  end
  local link = "<<%s*[%w_]+%->register_"
  local path, parameters = block.rest:match("^%s*[%w_]*%s*" .. link .. "function%(%s*\"([^\"]+)\"%s*,%s*%b[]%s*(%b())")
  if path then
    local what = "workflow." .. path
    local names, typed = typed_parameters(file, what, parameters)
    documented[what] = true
    functions[#functions + 1] = {
      name = what,
      file = file,
      signature = string.format("%s(%s)", what, typed),
      text = describe(file, what, lines, names, nil, true),
    }
    return
  end
  local member = block.rest:match("^%s*[%w_]*%s*" .. link .. "setting%(%s*%*settings%(%)%.([%w_]+)%s*%)")
  if member then
    local setting_path = all_cpp:match(member .. "%s*{%s*workflow_settings_%->create_setting%(%s*\"([^\"]+)\"")
    local cpp = all_cpp:match("setting_type(%b<>)%s+" .. member .. "%s*;")
    local type = cpp and lua_type(cpp:sub(2, -2), true)
    if not setting_path or not type then
      fail(file, string.format("no path or no Lua type found for the setting %s", member))
      return
    end
    local what = "workflow." .. setting_path
    documented[member] = true
    settings[#settings + 1] = {
      name = what,
      file = file,
      signature = string.format("%s: %s", what, type),
      text = describe(file, what, lines, {}, nil, true),
    }
    return
  end
  local constant, setting_parameters =
    block.rest:match("^%s*node%.set_function%(%s*names::([%w_]+)%s*,%s*function_cast%(%s*%b[]%s*(%b())")
  if constant then
    local name = all_cpp:match(constant .. "%s*=%s*\"([^\"]+)\"")
    if not name then
      fail(file, string.format("no value found for names::%s", constant))
      return
    end
    local names, typed = typed_parameters(file, name, setting_parameters)
    documented[constant] = true
    setting_functions[#setting_functions + 1] = {
      name = name,
      file = file,
      signature = string.format("setting.%s(%s)", name, typed),
      text = describe(file, name, lines, names, nil, true),
    }
    return
  end
  local struct, body = block.rest:match("^%s*struct%s+([%w_]+)%s+final%s*(%b{})")
  local id = body and body:match("util::path%s+id%s*{%s*\"([^\"]+)\"%s*}")
  if id then
    local names, tables = { input = {}, output = {} }, {}
    for _, side in ipairs({ "input", "output" }) do
      local fields = body:match("using%s+" .. side .. "%s*=%s*lua::script_table(%b<>)")
      local typed = {}
      for field in (fields or ""):gmatch("lua::field(%b<>)") do
        local key, cpp = field:sub(2, -2):match("^%s*\"([^\"]+)\"%s*,%s*(.-)%s*$")
        local type = cpp and lua_type(cpp)
        if not type then
          fail(file, string.format("%s: no Lua type for the field %s", id, field))
        end
        table.insert(names[side], key or field)
        typed[#typed + 1] = string.format("%s: %s", key, type)
      end
      if not fields then
        fail(file, string.format("%s: no lua::script_table as %s", id, side))
      end
      tables[#tables + 1] = string.format(#typed > 0 and "%-7s { %s }" or "%-7s {}", side .. ":", table.concat(typed, ", "))
    end
    documented[struct] = true
    manifests[#manifests + 1] = {
      name = id,
      file = file,
      signature = table.concat(tables, "\n"),
      text = describe(file, id, lines, names.input, names.output, true),
    }
    return
  end
  fail(file, "a \\lua comment is above no register_function, register_setting, setting function or manifest")
end

for _, source in ipairs(cpp_sources) do
  local documented = {}
  for _, block in ipairs(blocks(source.text, "///")) do
    if block.rest and table.concat(block.lines, "\n"):match("\\lua") then
      add_cpp(source.path, block, documented)
    end
  end
  local code = source.text:gsub("[^\n]*", function(line)
    return line:match("^%s*//") and "" or line
  end)
  for call, rest in code:gmatch("%->register_(%a+)%(%s*([^\n]*)") do
    local key = call == "function" and rest:match("^\"([^\"]+)\"") and ("workflow." .. rest:match("^\"([^\"]+)\""))
      or call == "setting" and rest:match("^%*settings%(%)%.([%w_]+)")
    if not key or not documented[key] then
      fail(source.path, string.format("register_%s of %s has no \\lua comment above it", call, key or rest))
    end
  end
  for constant in code:gmatch("node%.set_function%(%s*names::([%w_]+)") do
    if not documented[constant] then
      fail(source.path, string.format("the setting function names::%s has no \\lua comment above it", constant))
    end
  end
  for struct, body in code:gmatch("struct%s+([%w_]+)%s+final%s*(%b{})") do
    if body:match("util::path%s+id") and not documented[struct] then
      fail(source.path, string.format("the manifest %s has no \\lua comment above it", struct))
    end
  end
end

for _, source in ipairs(sources) do
  if source:match("%.lua$") then
    local text = assert(read(source), source)
    local documented = {}
    for _, block in ipairs(blocks(text, "%-%-%-")) do
      local signatures, lines = {}, {}
      for _, line in ipairs(block.lines) do
        local signature = line:match("^\\lua%s+(.+)$")
        if signature then
          signatures[#signatures + 1] = trim(signature)
        else
          lines[#lines + 1] = line
        end
      end
      -- Only a block tagged \lua has a line per parameter, the others are comments over declarations
      local tagged = #signatures > 0
      local name, parameters = (block.rest or ""):match("^function%s+(util%.[%w_]+)%s*(%b())")
      if name then
        signatures[#signatures + 1] = name .. parameters
      end
      if #signatures > 0 then
        local names, seen, heading = {}, {}, {}
        for i, signature in ipairs(signatures) do
          heading[i] = signature:match("^[%w_%.]+")
          documented[heading[i]] = true
          for _, parameter in ipairs(split_top(signature:match("%b()"):sub(2, -2))) do
            names[#names + 1] = not seen[parameter] and parameter or nil
            seen[parameter] = true
          end
        end
        utils[#utils + 1] = {
          name = table.concat(heading, ", "),
          file = source,
          signature = table.concat(signatures, "\n"),
          text = describe(source, heading[1], lines, names, nil, tagged),
        }
      end
    end
    for name in text:gmatch("\nfunction%s+(util%.[%w_]+)") do
      if not documented[name] then
        fail(source, string.format("%s has no --- comment above it", name))
      end
    end
  end
end

-- The state refuses a registration at a path taken by another or below it, the script would find the first one
local registered = {}
for _, list in ipairs({ settings, functions }) do
  for _, entry in ipairs(list) do
    for _, other in ipairs(registered) do
      local a, b = entry.name, other.name
      if a == b then
        fail(entry.file, string.format("%s is registered twice, the state refuses one of them", a))
      elseif a:find(b .. ".", 1, true) == 1 or b:find(a .. ".", 1, true) == 1 then
        fail(entry.file, string.format("%s and %s overlap, the state refuses one of them", a, b))
      end
    end
    registered[#registered + 1] = entry
  end
end

if #errors > 0 then
  io.stderr:write(table.concat(errors, "\n"), "\n")
  os.exit(1)
end

local root = output:gsub("[^/]+", ".."):gsub("/?%.%.$", "")
root = root == "" and "" or (root .. "/")

---
--- \return the Markdown section \p title of \p entries
---
local function section(title, intro, entries)
  local parts = { "## " .. title, intro }
  for _, entry in ipairs(entries) do
    local file = entry.file:match("[^/]+$")
    parts[#parts + 1] = string.format(
      "### %s\n\n```\n%s\n```\n\n%s\n\nSource: [%s](%s%s)",
      entry.name,
      entry.signature,
      entry.text,
      file,
      root,
      entry.file
    )
  end
  return table.concat(parts, "\n\n")
end

local doc = table.concat({
  "# Lua API",
  "<!-- Generated by tools/gen_lua_doc.lua from the comments tagged \\lua, do not edit -->",
  "What the app asks of scripts and what it offers them. How scripts work: [lua_scripts.md](lua_scripts.md).",
  section(
    "Manifests",
    "The functions a script offers, each named after a manifest, which defines its input and output.",
    manifests
  ),
  section(
    "Functions of the app",
    "A function called the wrong way, e.g. with a value of another type, raises an error like the functions of Lua do, "
      .. "`pcall` catches it. A failure the script can not prevent, e.g. a missing web page, is no error: the function "
      .. "returns `nil` and the reason, like `io.open`.",
    functions
  ),
  section(
    "Settings",
    "Settings of the app scripts read and change, each a table of the [setting functions](#setting-functions).",
    settings
  ),
  section("Setting functions", "The functions of each setting, `T` is the type of the setting.", setting_functions),
  section("util", "Helpers written in Lua.", utils),
}, "\n\n") .. "\n"

if check then
  if read(output) ~= doc then
    io.stderr:write(output, " is out of date, a build regenerates it\n")
    os.exit(1)
  end
else
  local file = assert(io.open(output, "wb"))
  file:write(doc)
  file:close()
end
