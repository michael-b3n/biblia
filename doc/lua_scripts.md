# Lua scripts

A script is a `*.lua` file that returns a table: `name`, which the app shows and stores in the settings, `node`, under which the other scripts see the script, and its entries. An entry that is a function named after a manifest in [lua_api.md](lua_api.md) is one the app runs. Manifests are named with a dot, e.g. `scripture.read`, so the app takes every function whose key has a dot for one: name other functions without a dot. The file name does not matter.

```lua
local example = {}

example["scripture.passage"] = function(input)   -- the input of the manifest as a table
  return { text = "..." }                         -- its output as a table, or nil for none
end

example.name = "Example"
example.node = "example"                          -- the other scripts see scripts.example
return example
```

[example.lua](../bibstd/lua/examples/example.lua) is a complete script. [lua_api.md](lua_api.md) lists the manifests and everything a script can call. The build generates it from the documented code, file by file: the C++ comments tagged `\lua` and the documented functions of the Lua files.

## Loading

- The bundled scripts ([bibstd/lua/bundled](../bibstd/lua/bundled), and `bibstd/lua/private` if a build has it) load first. Then the `*.lua` files in `%LOCALAPPDATA%\verselens\scripts` load in the order of their names, unless `Scripts > Enabled` is off.
- Scripts load after the app starts and when you click `Load scripts` in the scripts tab. The top level of a file runs once while it loads, so put the work into the functions.
- Files with the same `name` form one script with one `node`. If two files offer the same entry, the file that loaded first wins. So your script can add entries to a bundled one, but not replace them.
- The app rejects a file without `name` or `node`, with a `node` that is no Lua name, or with the `node` of another script, and writes the file name to the log.
- The scripts tab lists the functions named after a manifest, the other entries are for scripts.

## Nodes

Every script sees the entries of the others, read-only, as `scripts` followed by their node, e.g. `scripts.usx.parse` or `scripts.usx["scripture.read"]`. This is how scripts share code: the bundled scripts `usx.lua` and `usfm.lua` read the scripture files of their format through `scripts.scripture_archive.read_scripture`, which names a scripture with `scripts.paratext`, `scripts.dbl` or `scripts.ebible`. A script can add its own node, e.g. for another format:

```lua
local zefania = {}

function zefania.parse(text)                  -- e.g. the element tree of a Zefania XML bible
  return util.parse_xml(text)
end

zefania.name = "Zefania"
zefania.node = "zefania"                      -- the other scripts call scripts.zefania.parse
return zefania
```

- Use the nodes of other scripts in your functions, not at the top level of the file: the scripts after yours are not loaded yet.
- [lua_api.md](lua_api.md) lists the functions of the bundled nodes.

## Values

- Input and output have one value for each key of the manifest: a string, an integer, a number, a boolean, a list or a table. An optional value may be nil.
- An integer must be a Lua integer: `4 // 2` is one, `4 / 2` is the float `2.0`.
- The app rejects an output with a wrong type, a missing value or an unknown key, and writes the function to the log.

## Sandbox

- Each script has its own globals. It can use `base`, `coroutine`, `math`, `string`, `table` and `utf8`, but not `dofile`, `loadfile`, `require`, `rawset`, `collectgarbage`, `_G` or `print`. There is no `io`, `os`, `package` or `debug`, so scripts cannot reach files or processes. `load` takes only text.
- `util`, `scripts`, `workflow` and the libraries are read-only. `util.copy` makes a copy the script can change. `next` sees them as empty.
- `setmetatable` does not accept a `__gc` finalizer.
- Functions run one at a time, outside of the user interface and without a time limit, so other scripts wait. When the app closes, a running script fails at its next call.
