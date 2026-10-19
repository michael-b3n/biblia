---
--- Writes the reference of what the app offers scripts. It reads the comments tagged \lua: a /// block above a
--- register_function, a register_setting, a setting function of lua::state or a manifest struct, and the --- blocks
--- above the functions of util. Names, parameters and types come from the code. The comment adds the description
--- and a \param or \return line for each parameter, input and output.
--- Usage: lua gen_lua_doc.lua [--check] <output> <source>..., where @<file> stands for the sources listed in a file.
---

local check = arg[1] == "--check"
local output = arg[check and 2 or 1]
local sources = {}
for _, argument in ipairs({ table.unpack(arg, check and 3 or 2) }) do
  if argument:sub(1, 1) == "@" then
    for line in io.lines(argument:sub(2)) do
      local source = line:match("^%s*(.-)%s*$")
      if source ~= "" then
        sources[#sources + 1] = source
      end
    end
  else
    sources[#sources + 1] = argument
  end
end
local errors = {}

---
--- Record an error in \p file, \p message formatted with the values \p ...
---
local function fail(file, message, ...)
  errors[#errors + 1] = file .. ": " .. string.format(message, ...)
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
--- \return \p s without white space at its start and end
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
--- \return the comment blocks of \p text with the line prefix \p prefix, each with its lines and the position of the
--- code after it in \p text
---
local function blocks(text, prefix)
  local result, current = {}, nil
  for position, line in (text .. "\n"):gmatch("()([^\n]*)\n") do
    local content = line:match("^%s*" .. prefix .. " ?(.*)$")
    if content and not content:match("^[/%-]") then
      current = current or { lines = {}, text = text }
      current.lines[#current.lines + 1] = content
    elseif current then
      current.position = position
      result[#result + 1] = current
      current = nil
    end
  end
  return result
end

---
--- \return the captures of \p pattern, anchored with ^, matched against the code after \p block
---
local function after(block, pattern)
  return block.text:match(pattern, block.position)
end

---
--- Fail for each entry of \p documented that names none of \p names, and for each of \p names without one if
--- \p complete
---
local function check_names(file, what, tag, documented, names, complete)
  local allowed, seen = set_of(names), {}
  for _, entry in ipairs(documented) do
    if not allowed[entry.name] then
      fail(file, "%s: \\%s %s names nothing in the code", what, tag, entry.name)
    end
    seen[entry.name] = true
  end
  for _, name in ipairs(complete and names or {}) do
    if not seen[name] then
      fail(file, "%s: no \\%s for %s", what, tag, name)
    end
  end
end

---
--- \return the comment \p lines of \p what: its description, params and returns. A line starting with \param (or \p)
--- documents one of \p inputs, one starting with \return the result, or one of \p outputs if given. Each must be
--- documented if \p complete.
---
local function describe(file, what, lines, inputs, outputs, complete)
  local doc, current = { description = {}, params = {}, returns = {} }, nil
  for _, line in ipairs(lines) do
    line = trim(line)
    local tag, rest = line:match("^\\(%a+)%s+(.*)$")
    if line == "" then
      current = nil
    elseif tag == "param" or tag == "p" or (tag == "return" and outputs) then
      local name, text = rest:match("^(%S+)%s*(.*)$")
      current = { name = name, text = text }
      table.insert(tag == "return" and doc.returns or doc.params, current)
    elseif tag == "return" then
      current = { text = rest }
      table.insert(doc.returns, current)
    elseif current then
      current.text = current.text .. " " .. line
    else
      current = { text = line }
      table.insert(doc.description, current)
    end
  end
  if #doc.description == 0 and #doc.returns == 0 then
    fail(file, "%s has no description", what)
  end
  check_names(file, what, "param", doc.params, inputs, complete)
  if outputs then
    check_names(file, what, "return", doc.returns, outputs, complete)
  elseif #doc.returns > 1 then
    fail(file, "%s has more than one \\return", what)
  end

  -- A \p in the text names a parameter, an input or an output
  local names = set_of(inputs)
  for _, name in ipairs(outputs or {}) do
    names[name] = true
  end
  local function inline(text)
    local function name(n)
      if not names[n] then
        fail(file, "%s: \\p %s names nothing in the code", what, n)
      end
      return "`" .. n .. "`"
    end
    return (text:gsub("\\p%s+(%.%.%.)", name):gsub("\\p%s+([%a_][%w_]*)", name):gsub("\\see%s+", "see "))
  end
  for _, list in ipairs({ doc.description, doc.params, doc.returns }) do
    for _, entry in ipairs(list) do
      entry.text = inline(entry.text)
    end
  end
  return doc
end

---
--- \return the texts of \p entries
---
local function texts(entries)
  local result = {}
  for i, entry in ipairs(entries) do
    result[i] = entry.text
  end
  return result
end

---
--- \return \p doc as Markdown paragraphs, the inputs and outputs of a manifest if \p manifest
---
local function markdown(doc, manifest)
  local parts = { table.concat(texts(doc.description), "\n\n") }
  local function list(title, entries)
    local items = {}
    for i, entry in ipairs(entries) do
      items[i] = string.format("- `%s`: %s", entry.name, entry.text)
    end
    parts[#parts + 1] = #items > 0 and string.format("**%s**\n\n%s", title, table.concat(items, "\n")) or nil
  end
  list(manifest and "Input" or "Parameters", doc.params)
  if manifest then
    list("Output", doc.returns)
  elseif doc.returns[1] then
    parts[#parts + 1] = "**Returns** " .. doc.returns[1].text
  end
  return trim(table.concat(parts, "\n\n"))
end

---
--- \return \p doc as one line, for a list item
---
local function one_line(doc)
  local function sentence(text)
    return text:match("%.$") and text or text .. "."
  end
  local parts = texts(doc.description)
  for _, param in ipairs(doc.params) do
    parts[#parts + 1] = sentence(string.format("`%s`: %s", param.name, param.text))
  end
  if doc.returns[1] then
    parts[#parts + 1] = sentence("Returns " .. doc.returns[1].text)
  end
  return table.concat(parts, " ")
end

local cpp_sources, texts, all_cpp = {}, {}, {}
for _, source in ipairs(sources) do
  if source:match("%.[ch]pp$") then
    texts[source] = assert(read(source), source)
    cpp_sources[#cpp_sources + 1] = { path = source, text = texts[source] }
    all_cpp[#all_cpp + 1] = texts[source]
  end
end
all_cpp = table.concat(all_cpp, "\n")

-- The nodes workflow_script registers at, e.g. workflow.web.fetch and workflow.setting.web.retry_pause
local lua_node = all_cpp:match("lua_node%s*=%s*\"([^\"]+)\"")
local setting_node = all_cpp:match("setting_node%s*=%s*\"([^\"]+)\"")
if not lua_node or not setting_node then
  io.stderr:write("lua_node or setting_node of workflow_script not found\n")
  os.exit(1)
end

local workflow, setting_functions, manifests, utils = {}, {}, {}, {}

---
--- \return the names of the lambda \p parameters of \p what, and the parameters with the types scripts see
---
local function typed_parameters(file, what, parameters)
  local names, typed = {}, {}
  for i, parameter in ipairs(split_top(parameters:sub(2, -2))) do
    local cpp, name = parameter:match("^(.-)%s*([%a_][%w_]*)$")
    local type = cpp and lua_type(cpp)
    if not type then
      fail(file, "%s: no Lua type for the parameter \"%s\"", what, parameter)
    end
    names[i] = name or parameter
    typed[i] = string.format("%s: %s", name, type)
  end
  return names, table.concat(typed, ", ")
end

---
--- Add the entry of the C++ comment \p block of \p file, tagged \lua, and mark what it documents in \p documented
---
local function add_cpp(file, block, documented)
  local lines = {}
  for i, line in ipairs(block.lines) do
    lines[i] = line:gsub("\\lua%s*", "", 1)
  end
  local registration = "^%s*[%w_]*%s*<<%s*[%w_]+%->register_"

  local path, parameters = after(block, registration .. "function%(%s*\"([^\"]+)\"%s*,%s*%b[]%s*(%b())")
  if path then
    local name = lua_node .. "." .. path
    local names, typed = typed_parameters(file, name, parameters)
    documented[path] = true
    workflow[#workflow + 1] = {
      name = name,
      group = path:match("^[^.]+"),
      file = file,
      signature = string.format("%s(%s)", name, typed),
      text = markdown(describe(file, name, lines, names, nil, true)),
    }
    return
  end

  local member = after(block, registration .. "setting%(%s*%*settings%(%)%.([%w_]+)%s*%)")
  if member then
    -- The settings of a workflow are in its own source and header, other workflows may use the same member name
    local own = (texts[file] or "") .. "\n" .. (texts[(file:gsub("%.cpp$", ".hpp"))] or "")
    local setting_path = own:match("%f[%w_]" .. member .. "%s*{%s*workflow_settings_%->create_setting%(%s*\"([^\"]+)\"")
    local cpp = own:match("setting_type(%b<>)%s+" .. member .. "%s*;")
    local type = cpp and lua_type(cpp:sub(2, -2), true)
    if not setting_path or not type then
      fail(file, "no path or no Lua type found for the setting %s", member)
      return
    end
    local name = string.format("%s.%s.%s", lua_node, setting_node, setting_path)
    documented[member] = true
    workflow[#workflow + 1] = {
      name = name,
      group = setting_path:match("^[^.]+"),
      setting_type = type,
      file = file,
      signature = string.format("%s: %s", name, type),
      text = markdown(describe(file, name, lines, {}, nil, true)),
    }
    return
  end

  local constant, setting_parameters =
    after(block, "^%s*node%.set_function%(%s*names::([%w_]+)%s*,%s*function_cast%(%s*%b[]%s*(%b())")
  if constant then
    local name = all_cpp:match(constant .. "%s*=%s*\"([^\"]+)\"")
    if not name then
      fail(file, "no value found for names::%s", constant)
      return
    end
    local names, typed = typed_parameters(file, name, setting_parameters)
    documented[constant] = true
    setting_functions[#setting_functions + 1] = {
      name = name,
      typed = typed,
      file = file,
      text = one_line(describe(file, name, lines, names, nil, true)),
    }
    return
  end

  local struct, body = after(block, "^%s*struct%s+([%w_]+)%s+final%s*(%b{})")
  local id = body and body:match("util::path%s+id%s*{%s*\"([^\"]+)\"%s*}")
  if id then
    local names, tables = { input = {}, output = {} }, {}
    for _, side in ipairs({ "input", "output" }) do
      local fields = body:match("using%s+" .. side .. "%s*=%s*lua::script_table(%b<>)")
      if not fields then
        fail(file, "%s: no lua::script_table as %s", id, side)
      end
      local typed = {}
      for field in (fields or ""):gmatch("lua::field(%b<>)") do
        local key, cpp = field:sub(2, -2):match("^%s*\"([^\"]+)\"%s*,%s*(.-)%s*$")
        local type = cpp and lua_type(cpp)
        if not type then
          fail(file, "%s: no Lua type for the field %s", id, field)
        end
        table.insert(names[side], key or field)
        typed[#typed + 1] = string.format("%s: %s", key, type)
      end
      tables[#tables + 1] = string.format("%-7s { %s }", side .. ":", table.concat(typed, ", "))
    end
    documented[struct] = true
    manifests[#manifests + 1] = {
      name = id,
      file = file,
      signature = table.concat(tables, "\n"):gsub("{  }", "{}"),
      text = markdown(describe(file, id, lines, names.input, names.output, true), true),
    }
    return
  end

  fail(file, "a \\lua comment is above no register_function, register_setting, setting function or manifest")
end

for _, source in ipairs(cpp_sources) do
  local documented = {}
  for _, block in ipairs(blocks(source.text, "///")) do
    if table.concat(block.lines, "\n"):match("\\lua") then
      add_cpp(source.path, block, documented)
    end
  end
  -- Everything a script sees needs a comment, commented out code does not count
  local code = source.text:gsub("[^\n]*", function(line)
    return line:match("^%s*//") and "" or line
  end)
  for call, rest in code:gmatch("%->register_(%a+)%(%s*([^\n]*)") do
    local key = call == "function" and rest:match("^\"([^\"]+)\"") or call == "setting" and rest:match("^%*settings%(%)%.([%w_]+)")
    if (call == "function" or call == "setting") and not documented[key or ""] then
      fail(source.path, "register_%s of %s has no \\lua comment above it", call, key or rest)
    end
  end
  for constant in code:gmatch("node%.set_function%(%s*names::([%w_]+)") do
    if not documented[constant] then
      fail(source.path, "the setting function names::%s has no \\lua comment above it", constant)
    end
  end
  for struct, body in code:gmatch("struct%s+([%w_]+)%s+final%s*(%b{})") do
    if body:match("util::path%s+id") and not documented[struct] then
      fail(source.path, "the manifest %s has no \\lua comment above it", struct)
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
      local name, parameters = after(block, "^function%s+(util%.[%w_]+)%s*(%b())")
      if name then
        signatures[#signatures + 1] = name .. parameters
      end
      if #signatures > 0 then
        local names, seen = {}, {}
        for _, signature in ipairs(signatures) do
          documented[signature:match("^[%w_%.]+")] = true
          for _, parameter in ipairs(split_top(signature:match("%b()"):sub(2, -2))) do
            names[#names + 1] = not seen[parameter] and parameter or nil
            seen[parameter] = true
          end
        end
        utils[#utils + 1] = {
          signatures = signatures,
          file = source,
          text = one_line(describe(source, signatures[1], lines, names, nil, tagged)),
        }
      end
    end
    for name in text:gmatch("\nfunction%s+(util%.[%w_]+)") do
      if not documented[name] then
        fail(source, "%s has no --- comment above it", name)
      end
    end
  end
end

-- The state refuses a registration at a path that another one takes or lies below
for i, entry in ipairs(workflow) do
  for j = 1, i - 1 do
    local a, b = entry.name, workflow[j].name
    if a == b then
      fail(entry.file, "%s is registered twice, the state refuses one of them", a)
    elseif a:find(b .. ".", 1, true) == 1 or b:find(a .. ".", 1, true) == 1 then
      fail(entry.file, "%s and %s overlap, the state refuses one of them", a, b)
    end
  end
end

if #errors > 0 then
  io.stderr:write(table.concat(errors, "\n"), "\n")
  os.exit(1)
end

local root = string.rep("../", select(2, output:gsub("/", "")))

---
--- \return the line linking the files of \p entries
---
local function source_line(entries)
  local links, seen = {}, {}
  for _, entry in ipairs(entries) do
    if not seen[entry.file] then
      seen[entry.file] = true
      links[#links + 1] = string.format("[%s](%s%s)", entry.file:match("[^/]+$"), root, entry.file)
    end
  end
  return "Source: " .. table.concat(links, ", ")
end

---
--- \return \p entry below a heading of \p level, with a code block of its signature
---
local function entry_markdown(entry, level)
  return string.format("%s %s\n\n```\n%s\n```\n\n%s", string.rep("#", level), entry.name, entry.signature, entry.text)
end

local doc = {
  "# Lua API",
  "<!-- Generated by tools/gen_lua_doc.lua from the comments tagged \\lua, do not edit -->",
  "What the app asks of scripts and what it offers them. How scripts work: [lua_scripts.md](lua_scripts.md).",
  "## Manifests",
  "A script offers functions named after manifests. A manifest defines the input and output of its function.",
}
for _, manifest in ipairs(manifests) do
  doc[#doc + 1] = entry_markdown(manifest, 3) .. "\n\n" .. source_line({ manifest })
end

doc[#doc + 1] = "## " .. lua_node
doc[#doc + 1] = string.format(
  "What the app offers scripts, by workflow. Calling a function with wrong arguments raises an error, like the "
    .. "functions of Lua do. `pcall` catches it. A failure the script cannot prevent, e.g. a missing web page, is not "
    .. "an error: the function returns `nil` and the reason, like `io.open`. Settings are at `%s.%s` followed by "
    .. "their path. Each setting has the [setting functions](#setting-functions).",
  lua_node,
  setting_node
)
local groups, group_names = {}, {}
-- Settings first, then the functions, each in the order of their registration
for _, settings in ipairs({ true, false }) do
  for _, entry in ipairs(workflow) do
    if (entry.setting_type ~= nil) == settings then
      if not groups[entry.group] then
        groups[entry.group] = {}
        group_names[#group_names + 1] = entry.group
      end
      table.insert(groups[entry.group], entry)
    end
  end
end
table.sort(group_names)
for _, group in ipairs(group_names) do
  doc[#doc + 1] = string.format("### %s.%s\n\n%s", lua_node, group, source_line(groups[group]))
  for _, entry in ipairs(groups[group]) do
    local text = entry_markdown(entry, 4)
    if entry.setting_type then
      -- The setting functions with the type of this setting
      local calls = {}
      for i, setting_function in ipairs(setting_functions) do
        local typed = setting_function.typed:gsub("%f[%w_]T%f[^%w_]", entry.setting_type)
        calls[i] = string.format("`%s(%s)`", setting_function.name, typed)
      end
      text = text .. "\n\n[Setting functions](#setting-functions): " .. table.concat(calls, ", ")
    end
    doc[#doc + 1] = text
  end
end

doc[#doc + 1] = "## Setting functions"
local items = { "Each setting has these functions. `T` is the type of the setting.", "" }
for _, setting_function in ipairs(setting_functions) do
  items[#items + 1] = string.format("- `%s(%s)`: %s", setting_function.name, setting_function.typed, setting_function.text)
end
doc[#doc + 1] = table.concat(items, "\n") .. "\n\n" .. source_line(setting_functions)

doc[#doc + 1] = "## util"
items = { "Helpers written in Lua.", "" }
for _, util in ipairs(utils) do
  items[#items + 1] = string.format("- `%s`: %s", table.concat(util.signatures, "`, `"), util.text)
end
doc[#doc + 1] = table.concat(items, "\n") .. "\n\n" .. source_line(utils)

doc = table.concat(doc, "\n\n") .. "\n"

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
