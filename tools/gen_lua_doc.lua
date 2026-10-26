---
--- Writes the reference of what the app offers scripts: file by file in the order of their
--- paths, each documented part with its comment and the code below it. Documented are the C++
--- comments tagged \lua, and the Lua comments above a function of a table or tagged \lua.
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
--- \return the comment line \p line in Markdown: \p, \param, \return and \see as words. A \return
--- starting with one of the \p fields of a manifest names that field.
---
local function markdown(line, fields)
  line = line:gsub("\\lua%s*", "")
  line = line:gsub("^\\param%s+(%S+)", "- `%1`:")
  local field = line:match("^\\return%s+([%w_]+)")
  if fields[field] then
    line = line:gsub("^\\return%s+[%w_]+%s*", "- Returns `" .. field .. "`: ")
  else
    line = line:gsub("^\\return%s+", "- Returns ")
  end
  -- A full stop after a name ends the sentence
  line = line:gsub("\\p%s+([%w_][%w_.]*)", function(name)
    local bare, stops = name:match("^(.-)(%.*)$")
    return "`" .. bare .. "`" .. stops
  end)
  return (line:gsub("\\see%s+", "see "))
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
  return code, i
end

---
--- \return the documented parts of the file \p source with the comment \p prefix, each with its comment and code.
--- A part is a comment tagged \lua, or in \p lua one above a function of a table. In \p lua the code of a
--- tagged comment is the functions its \lua lines name.
---
local function parts(source, prefix, lua)
  local result, comment = {}, nil
  local lines = {}
  for line in (read(source) .. "\n"):gmatch("([^\n]*)\n") do
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
        local code = {}
        if lua then
          local described = {}
          for _, line in ipairs(comment) do
            local signature = line:match("^\\lua%s+(.+)$")
            if signature then
              code[#code + 1] = "function " .. signature
            else
              described[#described + 1] = line
            end
          end
          comment = described
          if #code == 0 then
            code[1] = assert(is_function and lines[i], source .. ": a \\lua comment names no function")
          end
        else
          code, i = statement(lines, i, source)
        end
        while comment[#comment] == "" do
          comment[#comment] = nil
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
--- \return the path scripts see the setting registered in \p code at, from the create_setting in \p text, or nil
---
local function setting_path(code, text, source)
  local name = code:match("register_setting%(%*settings%(%)%.([%w_]+)%)")
  if name then
    local path = text:match(name .. "%s*{%s*[%w_]+%->create_setting%(%s*\"([^\"]+)\"")
    return "workflow.setting." .. assert(path, source .. ": no create_setting of " .. name)
  end
end

local doc = {
  "# Lua API",
  "<!-- Generated by tools/gen_lua_doc.lua from the documented code, do not edit -->",
  "What the app offers scripts, file by file. How scripts work: [lua_scripts.md](lua_scripts.md).",
  table.concat({
    "- C++: a comment tagged `\\lua` documents the code below it.",
    "  - `register_function(\"web.fetch\", …)`: scripts call `workflow.web.fetch`, the parameters of the lambda are "
      .. "theirs. `std::optional` is a value or nil, `std::vector` a list, `std::map` a table.",
    "  - `register_setting(*settings().name)`: scripts see the setting at the path above its comment, with the "
      .. "setting functions of `lua::state`, e.g. `workflow.setting.web.retry_pause.get()`.",
    "  - A manifest struct with the `id` \"scripture.read\": a script offers the function \"scripture.read\", its "
      .. "input and output are tables of the keys of the `lua::field`s.",
    "- Lua: a `---` comment documents the function below it. In `interface_util.lua`, `util.split` is `util.split` "
      .. "for scripts, in `usx.lua` `usx.parse` is `scripts.usx.parse`, in `init.lua` "
      .. "`interface.load` is `load`. Files named `system_*` are left out, scripts do not see them.",
  }, "\n"),
}

for _, source in ipairs(sources) do
  local lua = source:match("%.lua$")
  if not (lua and source:match("[/\\]system_[^/\\]*$")) then
    local found = parts(source, lua and "%-%-%-" or "///", lua)
    if #found > 0 then
      doc[#doc + 1] = string.format("## %s", source)
    end
    for _, part in ipairs(found) do
      local code = table.concat(part.code, "\n")
      local fields = {}
      for field in code:gmatch('lua::field<"([%w_]+)"') do
        fields[field] = true
      end
      local text, in_list = {}, false
      local path = not lua and setting_path(code, read(source), source)
      if path then
        text = { "`" .. path .. "`", "" }
      end
      for _, line in ipairs(part.comment) do
        line = markdown(line, fields)
        if line:match("^%- ") then
          -- A list after text is a paragraph of its own
          if not in_list and #text > 0 and text[#text] ~= "" then
            text[#text + 1] = ""
          end
          in_list = true
        elseif line == "" then
          in_list = false
        elseif in_list then
          -- The continuation of a list item
          line = "  " .. line
        end
        if line ~= "" or #text > 0 then
          text[#text + 1] = line
        end
      end
      while text[#text] == "" do
        text[#text] = nil
      end
      doc[#doc + 1] = string.format("%s\n\n```%s\n%s\n```", table.concat(text, "\n"), lua and "lua" or "cpp", code)
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
