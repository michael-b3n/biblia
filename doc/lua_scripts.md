# Lua scripts

A script is a `*.lua` file that returns a table with:

- `name`: shown in the app and stored in the settings.
- `description`: a short text below the name in the scripts tab, may be empty.
- `node`: the name under which the other scripts see the script.
- its entries: functions and values.

The app runs an entry that is a function named after a manifest in [lua_api.md](lua_api.md). Manifests have a dot in their name, e.g. `scripture.read`, so give your other functions names without a dot. The file name does not matter.

```lua
local example = {}

example["scripture.passage"] = function(input)   -- the input of the manifest as a table
  return { text = "..." }                         -- its output as a table, or nil for none
end

example.name = "Example"
example.description = "Offers a scripture."       -- shown below the name, may be ""
example.node = "example"                          -- the other scripts see scripts.example
return example
```

[example.lua](../bibstd/lua/examples/example.lua) is a complete script. [lua_api.md](lua_api.md) lists the manifests and everything a script can call, with their input and output. The build generates it from the documented code: the C++ comments tagged `\lua` and the documented functions of the Lua files.

## Loading

- The bundled scripts ([bibstd/lua/bundled](../bibstd/lua/bundled), and `bibstd/lua/private` if a build has it) load first. Then the `*.lua` files in `%LOCALAPPDATA%\verselens\scripts` load in the order of their names, unless `Scripts > Enabled` is off.
- Scripts load after the app starts and when you click `Load scripts` in the scripts tab. The top level of a file runs once while it loads, so put the work into the functions.
- Files with the same `name` form one script with one `node`. The first file gives the `description`, and of two files offering the same entry the first one wins. So your script can add entries to a bundled one, but not replace them.
- The app rejects a file without `name`, `description` or `node`, with a `node` that is no Lua name, or with the `node` of another script, and writes the file name to the log.
- The scripts tab shows the name, the description and the functions named after a manifest. The other entries are for scripts.

## Nodes

Every script sees the entries of the others, read-only, as `scripts.<node>`, e.g. `scripts.usx.parse` or `scripts.usx["scripture.read"]`. This is how scripts share code: the bundled `usx.lua` and `usfm.lua` read the files of their format through `scripts.scripture_archive.read_scripture`, which takes the name of a scripture from `scripts.paratext`, `scripts.dbl` or `scripts.ebible`. A script can add its own node, e.g. for another format:

```lua
local zefania = {}

function zefania.parse(text)                  -- e.g. the element tree of a Zefania XML bible
  return util.parse_xml(text)
end

zefania.name = "Zefania"
zefania.description = "Reads Zefania XML bibles."
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
