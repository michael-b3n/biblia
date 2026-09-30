-- Read-only views of shared tables, so a script can not change what other scripts or the app use.
-- A view is an empty table reading through to the real one, nested tables are viewed as well.
-- Lua has no read-only tables and the environment of a script only gives it globals of its own: what it reaches through
-- them, e.g. string or util, are the shared tables. Such a proxy is the usual way to protect them.
local error, next, setmetatable, tostring, type = error, next, setmetatable, tostring, type

-- One view per table, built once, so views stay comparable. Never freed, what is viewed lives as long as the state.
local views = {}

---
--- \return the read-only view of \p value, values other than tables as they are. A \p flat table holds no tables and
--- never changes, its view reads it directly without a call per key.
---
local function readonly(value, flat)
  if type(value) ~= "table" then
    return value
  end
  local view = views[value]
  if view then
    return view
  end
  view = setmetatable({}, {
    __index = flat and value or function(_, key)
      return readonly(value[key])
    end,
    __newindex = function(_, key)
      error(tostring(key) .. " is read only", 2)
    end,
    __pairs = function()
      return function(_, key)
        local next_key, next_value = next(value, key)
        return next_key, readonly(next_value)
      end, view, nil
    end,
    __len = function()
      return #value
    end,
    -- Protected, else setmetatable would lift the view
    __metatable = "read only",
  })
  views[value] = view
  -- A view is its own view
  views[view] = view
  return view
end

root.system.readonly = readonly
