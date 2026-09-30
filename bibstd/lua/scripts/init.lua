-- Sets up the state: fills the tree the app created as global `root`.
-- root.interface is what scripts see as their globals, read-only: the safe functions and libraries of Lua, the helpers
-- and what the workflows register. root.system is kept for the app.
-- The app passes the file names of the other embedded scripts and gave root.system.embedded and the log functions.
local files = ...

local interface, system = root.interface, root.system

-- Each adds itself to the tree, at the node it names
for _, file in ipairs(files) do
  assert(load(system.embedded(file), "@" .. file, "t"))()
end

-- The functions and libraries of Lua scripts may use. Not dofile, loadfile and the io, os, package and debug libraries,
-- they would reach files and processes. Not print, there is no console, \see util.log_info. Not rawset, a view is a
-- table of its own, rawset would change it for all scripts. Not collectgarbage, the collector is shared.
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

-- All strings share one metatable, it gives them their methods, e.g. ("a"):upper(). getmetatable("") hands out this
-- field from now on instead of the metatable, so a script can not change the methods for all.
getmetatable("").__metatable = "read only"

-- Read-only, so a script can not change what other scripts or the app use. The app runs the scripts on it.
local sandbox = readonly(interface)
system.sandbox = sandbox

---
--- setmetatable of scripts, a metatable with a finalizer fails: Lua runs finalizers without hooks, so the shutdown
--- could not stop one
--- \return \p table
---
function interface.setmetatable(table, metatable)
  if type(metatable) == "table" and rawget(metatable, "__gc") ~= nil then
    error("finalizers are not available to scripts", 2)
  end
  return setmetatable(table, metatable)
end

---
--- load of scripts, text only: Lua does not verify bytecode, a damaged one corrupts memory instead of failing with
--- an error that could be caught. By default the chunk gets globals of its own on the sandbox.
--- \return the loaded chunk, or nil and the error
---
function interface.load(chunk, name, _, environment)
  return load(chunk, name, "t", environment or setmetatable({}, { __index = sandbox }))
end
