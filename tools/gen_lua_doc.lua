---
--- Writes the reference of what the app offers scripts and what scripts offer the app: the manifests, the functions
--- and settings of the app, the helpers and the nodes of the bundled scripts, each with its input and output.
--- Documented are the C++ comments tagged \lua, and the Lua comments above a function of a table or tagged \lua.
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
table.sort(sources)

---
--- \return the content of the file at \p path with "\n" line endings
---
local function read(path)
  -- Binary, text mode would convert line endings on Windows only
  local file = assert(io.open(path, "rb"), path)
  local content = file:read("a"):gsub("\r\n", "\n")
  file:close()
  return content
end

---
--- \return the comment text \p text in Markdown: \p and \see as words
---
local function inline(text)
  -- A full stop after a name ends the sentence
  text = text:gsub("\\p%s+([%w_][%w_.]*)", function(name)
    local bare, stops = name:match("^(.-)(%.*)$")
    return "`" .. bare .. "`" .. stops
  end)
  return (text:gsub("\\see%s+", "see "))
end

---
--- \return the parts of the C++ code \p text separated by commas outside of brackets, trimmed
---
local function split(text)
  local result, depth, start = {}, 0, 1
  for position, character in text:gmatch("()([<>%(%){},])") do
    if character == "," then
      if depth == 0 then
        result[#result + 1] = text:sub(start, position - 1):match("^%s*(.-)%s*$")
        start = position + 1
      end
    else
      depth = depth + (character:match("[<%({]") and 1 or -1)
    end
  end
  local last = text:sub(start):match("^%s*(.-)%s*$")
  if last ~= "" then
    result[#result + 1] = last
  end
  return result
end

---
--- \return the Lua type of the C++ parameter or field type \p cpp: `{ T }` for a list, `{ [K] = V }` for a table
---
local function lua_type(cpp, source)
  cpp = cpp:gsub("%s+", " "):match("^ ?(.-)[ &]*$"):gsub("^const ", "")
  local optional = cpp:match("^std::optional<(.*)>$")
  if optional then
    return lua_type(optional, source) .. " or nil"
  end
  local element = cpp:match("^std::vector<(.*)>$")
  if element then
    return "{ " .. lua_type(element, source) .. " }"
  end
  local arguments = cpp:match("^std::map<(.*)>$")
  if arguments then
    local key, value = table.unpack(split(arguments))
    return "{ [" .. lua_type(key, source) .. "] = " .. lua_type(value, source) .. " }"
  end
  if cpp == "std::string" then
    return "string"
  elseif cpp == "int" or cpp:match("^std::u?int%d+_t$") then
    return "integer"
  elseif cpp == "bool" then
    return "boolean"
  elseif cpp == "double" then
    return "number"
  end
  error(source .. ": no Lua type for " .. cpp)
end

---
--- \return the fields of the `lua::script_table` \p declaration, each with its name and Lua type
---
local function fields(declaration, source)
  local result = {}
  for _, field in ipairs(split(declaration:match("^lua::script_table<(.*)>$") or "")) do
    local name, type = field:match('^lua::field<"([%w_]+)"%s*,(.*)>$')
    result[#result + 1] = { name = name, type = lua_type(type, source) }
  end
  return result
end

---
--- \return the parameters of the lambda that the C++ \p code registers, each with its name and Lua type
---
local function lambda_parameters(code, source)
  local list = assert(code:match('register_function%(%s*"[^"]+"%s*,%s*%[[^%]]*%]%s*(%b())'), source)
  local result = {}
  for _, parameter in ipairs(split(list:sub(2, -2))) do
    local type, name = parameter:match("^(.-)([%w_]+)$")
    result[#result + 1] = { name = name, type = lua_type(type, source) }
  end
  return result
end

---
--- \return the C++ statement or definition starting at line \p i of \p lines, up to where its brackets close,
--- and the index of the line after it
---
local function statement(lines, i, source)
  local code, depth, opened = {}, 0, false
  while not opened or depth > 0 do
    local line = assert(lines[i], source .. ": the code below a \\lua comment does not end")
    code[#code + 1] = line
    i = i + 1
    -- Brackets in literals and comments do not count
    local bare = line:gsub("\\.", ""):gsub('"[^"]*"', ""):gsub("'[^']*'", ""):gsub("//.*", "")
    for bracket in bare:gmatch("[%(%)%[%]{}]") do
      depth = depth + (bracket:match("[%(%[{]") and 1 or -1)
      opened = true
    end
  end
  return table.concat(code, "\n"), i
end

---
--- \return the documented parts of the file text \p text with the comment \p prefix, each with its comment and the
--- code below it. A part is a comment tagged \lua, or in \p lua one above a function of a table.
---
local function parts(text, prefix, lua, source)
  local result, comment = {}, nil
  local lines = {}
  for line in (text .. "\n"):gmatch("([^\n]*)\n") do
    lines[#lines + 1] = line
  end
  local i = 1
  while i <= #lines do
    local content = lines[i]:match("^%s*" .. prefix .. " ?(.*)$")
    if content then
      comment = comment or {}
      -- The empty first and last lines of a block, e.g. "///"
      if content ~= "" or #comment > 0 then
        comment[#comment + 1] = content
      end
      i = i + 1
    else
      local tagged = comment and table.concat(comment, "\n"):find("\\lua")
      local is_function = lua and lines[i]:match("^function%s+[%w_]+[.:]")
      if comment and (tagged or is_function) then
        while comment[#comment] == "" do
          comment[#comment] = nil
        end
        local code = lines[i]
        if not lua then
          code, i = statement(lines, i, source)
        end
        result[#result + 1] = { comment = comment, code = code }
      else
        i = i + 1
      end
      comment = nil
    end
  end
  return result
end

---
--- \return the comment lines \p lines split into the function names of its \lua lines, its description and its
--- \param and \return items, each item with its lines
---
local function parse(lines, source)
  local result = { signatures = {}, description = {}, items = {} }
  local item
  for _, line in ipairs(lines) do
    local signature = line:match("^\\lua%s+([%w_.<>]+%b())%s*$")
    line = line:gsub("^\\lua%s*", "")
    local name, rest = line:match("^\\param%s+(%S+)%s*(.*)$")
    if signature then
      result.signatures[#result.signatures + 1] = signature
    elseif name then
      item = { name = name, lines = { inline(rest) } }
      result.items[#result.items + 1] = item
    elseif line:match("^\\return") then
      item = { returns = true, lines = { inline(line:match("^\\return%s*(.*)$")) } }
      result.items[#result.items + 1] = item
    elseif item and line ~= "" then
      item.lines[#item.lines + 1] = inline(line)
    elseif line ~= "" or #result.description > 0 then
      assert(#result.items == 0, source .. ": text after the \\param and \\return lines")
      result.description[#result.description + 1] = inline(line)
    end
  end
  return result
end

---
--- \return the list item of \p head followed by the lines \p lines
---
local function list_item(head, lines)
  local result = { "- " .. head .. lines[1] }
  for i = 2, #lines do
    result[#result + 1] = "  " .. lines[i]
  end
  return table.concat(result, "\n")
end

---
--- \return the items of \p comment as list items, the parameters typed and in the order of \p parameters if given
---
local function items(comment, parameters, source)
  local result = {}
  if parameters then
    local documented = {}
    for _, item in ipairs(comment.items) do
      if item.name then
        documented[item.name] = item
      end
    end
    for _, parameter in ipairs(parameters) do
      local item = documented[parameter.name]
      documented[parameter.name] = nil
      local head = string.format("`%s` (`%s`)", parameter.name, parameter.type)
      result[#result + 1] = item and list_item(head .. ": ", item.lines) or "- " .. head
    end
    assert(next(documented) == nil, source .. ": " .. tostring(next(documented)) .. " is no parameter")
  end
  for _, item in ipairs(comment.items) do
    if item.returns then
      result[#result + 1] = list_item("Returns ", item.lines)
    elseif not parameters then
      result[#result + 1] = list_item("`" .. item.name .. "`: ", item.lines)
    end
  end
  return result
end

---
--- \return the input and the output of the manifest \p code as lists, with the \param and \return items of
--- \p comment, a \return item names its field first
---
local function manifest_lists(code, comment, source)
  local input, output = {}, {}
  for _, item in ipairs(comment.items) do
    if item.returns then
      local name, rest = item.lines[1]:match("^([%w_]+)%s*(.*)$")
      output[#output + 1] = { name = name, lines = { rest, table.unpack(item.lines, 2) } }
    else
      input[#input + 1] = item
    end
  end
  local input_fields = fields(code:match("using input%s*=%s*(.-);"), source)
  local output_fields = fields(code:match("using output%s*=%s*(.-);"), source)
  return {
    { title = "Input", items = items({ items = input }, input_fields, source) },
    { title = "Output", items = items({ items = output }, output_fields, source) },
  }
end

---
--- \return the path scripts see the setting registered in \p code at, from the create_setting in \p text, or nil
---
local function setting_path(code, text, source)
  local name = code:match("register_setting%(%*settings%(%)%.([%w_]+)%)")
  if name then
    local path = text:match(name .. "%s*{%s*[%w_]+%->create_setting%(%s*\"([^\"]+)\"")
    return "workflow.setting." .. assert(path, source .. ": no create_setting of " .. name)
  end
end

---
--- \return the names under which scripts see the tables of the Lua file \p text, by table, e.g. "scripts.usx."
---
local function lua_prefixes(text)
  local result = {}
  for table_name, node in text:gmatch('([%w_]+)%.node%s*=%s*"([%w_]+)"') do
    result[table_name] = "scripts." .. node .. "."
  end
  for node, table_name in text:gmatch("root%.interface%.([%w_]+)%s*=%s*([%w_]+)") do
    result[table_name] = node .. "."
  end
  -- A local for root.interface itself holds the globals of scripts
  for names, values in text:gmatch("local ([%w_, ]-) = ([%w_., ]-)\n") do
    local values_list = split(values)
    for index, name in ipairs(split(names)) do
      if values_list[index] == "root.interface" then
        result[name] = ""
      end
    end
  end
  return result
end

---
--- \return the entry of the documented C++ part \p part of the file text \p text
---
local function cpp_entry(part, text, source)
  local comment = parse(part.comment, source)
  local entry = { description = comment.description, lists = {} }
  local function_name = part.code:match('register_function%(%s*"([^"]+)"')
  local manifest = part.code:match('util::path%s+id%s*{%s*"([^"]+)"')
  local setting = setting_path(part.code, text, source)
  if function_name then
    local parameters = lambda_parameters(part.code, source)
    local names = {}
    for _, parameter in ipairs(parameters) do
      names[#names + 1] = parameter.name
    end
    entry.names = { string.format("workflow.%s(%s)", function_name, table.concat(names, ", ")) }
    entry.lists = { { items = items(comment, parameters, source) } }
  elseif manifest then
    entry.manifest = true
    entry.names = { manifest }
    entry.lists = manifest_lists(part.code, comment, source)
  elseif setting then
    entry.names = { setting }
  else
    assert(#comment.signatures > 0, source .. ": a \\lua comment names no function")
    entry.names = comment.signatures
    entry.lists = { { items = items(comment, nil, source) } }
  end
  return entry
end

---
--- \return the entry of the documented Lua part \p part, the names of its functions with the \p prefixes
---
local function lua_entry(part, prefixes, source)
  local comment = parse(part.comment, source)
  local signatures = comment.signatures
  if #signatures == 0 then
    signatures = { part.code:match("^function%s+([%w_]+[.:][%w_]+%s*%b())") }
  end
  local names = {}
  for _, signature in ipairs(signatures) do
    local table_name, rest = signature:match("^([%w_]+)[.:](.*)$")
    local prefix = assert(prefixes[table_name], source .. ": scripts do not see " .. signature)
    names[#names + 1] = prefix .. rest
  end
  return { names = names, description = comment.description, lists = { { items = items(comment, nil, source) } } }
end

local groups = {
  {
    title = "Manifests",
    intro = "A script offers a function named after a manifest, e.g. `scripture.read`. "
      .. "The app calls it with the input as a table and takes the output as a table.",
  },
  { title = "Functions", intro = "What the app offers at `workflow`." },
  {
    title = "Settings",
    intro = "Settings of the app at `workflow.setting`, each with the functions at the end, "
      .. "`<setting>` stands for one of them.",
  },
  { title = "Helpers", intro = "What the app offers at `util`." },
  { title = "Bundled scripts", intro = "The nodes of the bundled scripts at `scripts`." },
  { title = "Lua", intro = "The functions of Lua that work differently for scripts." },
}
for _, group in ipairs(groups) do
  group.entries = {}
end

---
--- \return the group of \p entry
---
local function group_of(entry)
  local name = entry.names[1]
  if entry.manifest then
    return groups[1]
  elseif name:match("^workflow%.setting%.") or name:match("^<setting>") then
    return groups[3]
  elseif name:match("^workflow%.") then
    return groups[2]
  elseif name:match("^util%.") then
    return groups[4]
  elseif name:match("^scripts%.") then
    return groups[5]
  end
  return groups[6]
end

for _, source in ipairs(sources) do
  local lua = source:match("%.lua$")
  -- Scripts do not see them
  if not (lua and source:match("[/\\]system_[^/\\]*$")) then
    local text = read(source)
    local prefixes = lua and lua_prefixes(text)
    for _, part in ipairs(parts(text, lua and "%-%-%-" or "///", lua, source)) do
      local entry = lua and lua_entry(part, prefixes, source) or cpp_entry(part, text, source)
      local entries = group_of(entry).entries
      entries[#entries + 1] = entry
    end
  end
end

---
--- \return \p entry in Markdown
---
local function render(entry)
  local names = {}
  for _, name in ipairs(entry.names) do
    names[#names + 1] = "`" .. name .. "`"
  end
  local result = { "### " .. table.concat(names, ", ") }
  if #entry.description > 0 then
    result[#result + 1] = table.concat(entry.description, "\n")
  end
  for _, list in ipairs(entry.lists) do
    if #list.items > 0 then
      if list.title then
        result[#result + 1] = list.title .. ":"
      end
      result[#result + 1] = table.concat(list.items, "\n")
    end
  end
  return table.concat(result, "\n\n")
end

local doc = {
  "# Lua API",
  "<!-- Generated by tools/gen_lua_doc.lua from the documented code, do not edit -->",
  "What the app offers scripts and what scripts offer the app. How scripts work: [lua_scripts.md](lua_scripts.md).",
  "Types: `{ T }` is a list, `{ [K] = V }` a table with keys of type `K`, `or nil` marks a value that may be nil. "
    .. "An integer is a Lua integer.",
}
for _, group in ipairs(groups) do
  if #group.entries > 0 then
    doc[#doc + 1] = "## " .. group.title
    doc[#doc + 1] = group.intro
    -- The functions every setting offers after the settings
    for _, last in ipairs({ false, true }) do
      for _, entry in ipairs(group.entries) do
        if (entry.names[1]:match("^<") ~= nil) == last then
          doc[#doc + 1] = render(entry)
        end
      end
    end
  end
end

doc = table.concat(doc, "\n\n") .. "\n"

if check then
  local file = io.open(output, "rb")
  local current = file and file:read("a"):gsub("\r\n", "\n")
  if file then
    file:close()
  end
  if current ~= doc then
    io.stderr:write(output, " is out of date, a build regenerates it\n")
    os.exit(1)
  end
else
  local file = assert(io.open(output, "wb"))
  file:write(doc)
  file:close()
end
