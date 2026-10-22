# Lua scripts

A script is a `*.lua` file that returns a table with two keys: `name`, which the app shows and stores in the settings, and `functions`, the functions the script offers. Each function is named after a manifest in [lua_api.md](lua_api.md). The file name does not matter.

```lua
return {
  name = "Example",
  functions = {
    ["scripture.passage"] = function(input)   -- the input of the manifest as a table
      return { text = "..." }                 -- its output as a table, or nil for none
    end,
  },
}
```

[example.lua](../bibstd/lua/examples/example.lua) is a complete script. [lua_api.md](lua_api.md) lists the manifests and everything a script can call. The build generates it from the documented code, file by file: the C++ comments tagged `\lua` and the documented functions of the Lua files.

## Loading

- The bundled scripts ([bibstd/lua/bundled](../bibstd/lua/bundled), and `bibstd/lua/private` if a build has it) load first. Then the `*.lua` files in `%LOCALAPPDATA%\verselens\scripts` load in the order of their names, unless `Scripts > Enabled` is off.
- Scripts load after the app starts and when you click `Load scripts` in the scripts tab. The top level of a file runs once while it loads, so put the work into the functions.
- Files with the same `name` form one script. If two files offer the same function, the file that loaded first wins. So your script can add functions to a bundled one, but not replace them.
- The app rejects a file without `name` or `functions` and writes the file name to the log.

## Values

- Input and output have one value for each key of the manifest: a string, an integer, a number, a boolean, a list or a table. An optional value may be nil.
- An integer must be a Lua integer: `4 // 2` is one, `4 / 2` is the float `2.0`.
- The app rejects an output with a wrong type, a missing value or an unknown key, and writes the function to the log.

## Sandbox

- Each script has its own globals. It can use `base`, `coroutine`, `math`, `string`, `table` and `utf8`, but not `dofile`, `loadfile`, `require`, `rawset`, `collectgarbage`, `_G` or `print`. There is no `io`, `os`, `package` or `debug`, so scripts cannot reach files or processes. `load` takes only text.
- `util`, `workflow` and the libraries are read-only. `util.copy` makes a copy the script can change. `next` sees them as empty.
- `setmetatable` does not accept a `__gc` finalizer.
- Functions run one at a time, outside of the user interface and without a time limit, so other scripts wait. When the app closes, a running script fails at its next call.
