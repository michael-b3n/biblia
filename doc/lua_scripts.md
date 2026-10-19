# Lua scripts

A script is a `*.lua` file returning a table: its `name`, shown to the user and kept in the settings, and the `functions` it offers, each named after a manifest of [lua_api.md](lua_api.md). The file name plays no role.

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

[example.lua](../bibstd/lua/examples/example.lua) shows a whole script. [lua_api.md](lua_api.md) lists the manifests and what scripts call, it is generated from the comments tagged `\lua` in the sources.

## Loading

- Bundled scripts ([bibstd/lua/bundled](../bibstd/lua/bundled), and `bibstd/lua/private` where a build has it) load first, then the `*.lua` files of `%LOCALAPPDATA%\verselens\scripts` in the order of their names, unless `Scripts > Enabled` is off.
- Scripts load after the start and with `Load scripts` in the scripts tab. The top level of a file runs once while loading, the work belongs into the functions.
- Files of the same `name` are one script. A function offered twice stays with the file loaded first, so a script of yours adds to a bundled one and replaces nothing.
- A file without `name` or `functions` is rejected, the log tells which.

## Values

- Input and output hold a string, an integer, a number, a boolean, a sequence or a table by key for each key of the manifest. An optional one may be nil.
- An integer is a Lua integer: `4 // 2` is one, `4 / 2` is the float `2.0`.
- An output of a wrong type, without a value it must have or with an unknown key is rejected, the log names the function.

## Sandbox

- Each script has its own globals. Available are `base`, `coroutine`, `math`, `string`, `table` and `utf8`, without `dofile`, `loadfile`, `require`, `rawset`, `collectgarbage`, `_G` and `print`. There is no `io`, `os`, `package` or `debug`, so scripts reach no files and no processes. `load` only takes text.
- `util`, `workflow` and the libraries are read-only, `util.copy` makes a copy of the script's own. `next` sees them empty.
- `setmetatable` takes no `__gc` finalizer.
- The functions run one at a time outside of the user interface, without a time limit: other scripts wait meanwhile. On exit a running script fails at its next call.
