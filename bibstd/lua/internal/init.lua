-- Sets up the state in the global `root` the app created. root.interface is what scripts see as their globals,
-- read-only: the safe parts of Lua, the helpers and what the workflows register. root.system is kept for the app.
-- The app passes the file names of the other embedded scripts.
local files = ...

local interface, system = root.interface, root.system

-- Each adds itself to the tree, at the node it names
for _, file in ipairs(files) do
  assert(load(system.embedded(file), "@" .. file, "t"))()
end

-- What scripts may use of Lua. Not dofile, loadfile, io, os, package and debug, they reach files and processes.
-- Not print, there is no console. Not rawset, it would change a view. Not collectgarbage, the collector is shared.
for _, name in ipairs({
  "assert", "error", "getmetatable", "ipairs", "next", "pairs", "pcall", "rawequal", "rawget", "rawlen", "select",
  "tonumber", "tostring", "type", "xpcall", "_VERSION", "coroutine", "math", "string", "table", "utf8",
}) do
  interface[name] = _G[name]
end

local readonly = system.readonly
for _, library in ipairs({ coroutine, math, string, table, utf8 }) do
  -- Their content never changes, the view reads them directly
  readonly(library, true)
end

-- All strings share one metatable for their methods, getmetatable("") hands out this field instead from now on
getmetatable("").__metatable = "read only"

-- Read-only, so a script can not change what other scripts or the app use. The app runs the scripts on it.
local sandbox = readonly(interface)
system.sandbox = sandbox

---
--- setmetatable of scripts. A finalizer fails: Lua runs it without hooks, the shutdown could not stop it.
--- \return \p table
---
function interface.setmetatable(table, metatable)
  if type(metatable) == "table" and rawget(metatable, "__gc") ~= nil then
    error("finalizers are not available to scripts", 2)
  end
  return setmetatable(table, metatable)
end

---
--- load of scripts. Text only, damaged bytecode crashes the app. By default the chunk gets globals of its own.
--- \return the loaded chunk, or nil and the error
---
function interface.load(chunk, name, _, environment)
  return load(chunk, name, "t", environment or setmetatable({}, { __index = sandbox }))
end
