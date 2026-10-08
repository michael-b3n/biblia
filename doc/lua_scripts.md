# Lua scripts

Lua scripts offer scriptures or the lookup of references in the browser.

| Scripts | Where | Loaded |
|---|---|---|
| Bundled | [bibstd/lua/bundled](../bibstd/lua/bundled), part of the app | Always, first |
| User | `*.lua` in `%LOCALAPPDATA%\verselens\scripts`, or in `Scripts > Folder` | Unless `Scripts > Enabled` is turned off, in the order of their file names |

Scripts load once after the start. A changed script or setting takes effect on the next start, or right away with `Load scripts` in the scripts tab, which lists the loaded scripts and their functions.

[example.lua](../bibstd/lua/examples/example.lua) is installed with the app in `share/scripts`. Copied into the script folder it is inactive until its `enabled` is set.

## A script

A script returns a table describing itself:

| Key | Value |
|---|---|
| `name` | What the app knows the script by, shown to the user and kept in the settings. A script using a web page takes the name of the page, e.g. `Bibleserver`. The file name plays no role. |
| `functions` | The functions the script offers, each named after a manifest of the app, which defines its input and output. Other entries are left out. |

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

- A script returning no table is loaded but offers nothing. Without a `name` or `functions` it is rejected, the log tells its file.
- Scripts of the same name are one script, e.g. the lookup and the scriptures of a web page in two files. A function both offer is the one of the script loaded first, the log names the file left out. So a script of yours adds functions to a bundled one, it replaces none.
- The code of the file runs once while loading. Keep it to definitions, the work belongs into the functions.
- The functions run outside of the user interface, one at a time.

Input and output have a value for each key of the manifest:

- A string, an integer, a number, a boolean, a sequence like `{ "a", "b" }` or a table by key like `{ a = 1 }`, with any key but nil.
- An integer is a Lua integer: `4 // 2` is one, `4 / 2` is the float `2.0`.
- An optional value may be nil or left out. `nil` as output is a table without keys.
- An output with a value of another type, without a value it must have or with a key the manifest does not have is rejected, the log names the function. A failing function gives no output.

## Scriptures

A script offering the four functions below offers scriptures. Several scripts may. Their scriptures join those of `Scripture > Name`, named after their script, e.g. `LUT (Bibleserver)`, and use the fallback versification.

| Manifest | Input | Output |
|---|---|---|
| `scripture.names` | | `names`: the scriptures the script offers, e.g. `{ "ABC" }` |
| `scripture.information` | `name` | `abbreviation`, `language`, `copyright`: strings, the copyright is shown below the verses |
| `scripture.book` | `name`, `book` | `abbreviation`, `short_name`, `long_name`: strings. The short name is shown, without one the identifier |
| `scripture.passage` | `name`, `book`, `chapter`, `verse` | `text`: the plain text of the verse. `error`: why there is none |

- `name` is one of the names the script offers, `book` the identifier of the app, e.g. `john`, `chapter` and `verse` are integers.
- Markup of a page is shown as text.
- A verse without `text` shows the `error` in its place, without one `verse not found`.
- `scripture.passage` runs for every verse shown and the app keeps nothing. So the script asks `workflow.cache` first, and a script reading web pages fetches the whole chapter once and keeps its verses there, see `example.lua`.
- Mind the terms of use of the pages a script reads and the copyright of the translations.

## Lookup

A script offering the two functions below opens references in the browser, `Lookup > Scripts` chooses one by its name. Until the user chooses, it is the script offering the most translations. Bundled are [lookup_bibleserver.lua](../bibstd/lua/bundled/lookup_bibleserver.lua) and [lookup_bible_com.lua](../bibstd/lua/bundled/lookup_bible_com.lua).

| Manifest | Input | Output |
|---|---|---|
| `lookup.translations` | `language` | `names`: the translations to choose from in `Lookup > Translations`, may be empty. `defaults`: the ones chosen as long as the user chose none, optional |
| `lookup.url` | `translations`, `book`, `chapter`, `verse_begin`, `verse_end` | `url`: the web page showing these verses, or none |

- `language` is the language of the user, `english` or `german`.
- `names` are shown as they are. The bundled scripts offer full names, e.g. `Lutherbibel`, and know the abbreviations of the urls themselves.
- `translations` are the chosen names in the order chosen, `book` is the identifier of the app, the others are integers.
- A reference over several chapters asks for a url per chapter, each is opened in a tab of its own.
- Only a valid `http` or `https` url is opened, so encode what is not ASCII.

## Functions of the app

| Function | Description |
|---|---|
| `workflow.web.fetch(url)` | The content of the web page at `url` (http or https), or `nil` and the reason: `invalid_url`, `not_found` (HTTP 404 or 410), `timeout` (30 s), `init_failed`, else `request_failed`. The script waits for the page and other scripts wait meanwhile. A url that failed is not requested again for the time of `Web > Pause after a failure`, 60 s unless changed, the reason is answered from memory. |
| `workflow.cache.get(name, key)`, `workflow.cache.set(name, key, value)` | Text values kept beyond the run of the app. `name` is of letters, digits, `_` and `-`, a script names its own cache. `value = nil` removes the key. Each cache is the SQLite file `cache/<name>.sqlite` of the script folder, other programs may read and change it meanwhile. A failing cache is logged and holds nothing. |
| `util` | Strings: `split`, `trim`, `starts_with`, `ends_with`. Tables: `keys`, `contains`, `map`, `filter`, `copy`. Inspection: `dump`, `at`. Log of the app: `log_debug`, `log_info`, `log_warning`, `log_error`, each taking values like `print` does. |

A function called the wrong way, e.g. with a value of another type, raises an error like the functions of Lua do, `pcall` catches it. A failure the script can not prevent, e.g. a missing web page, is no error: the function returns `nil` and the reason, like `io.open`.

## Sandbox

- Every script has an environment of its own, its globals do not reach other scripts.
- Available: `base`, `coroutine`, `math`, `string`, `table` and `utf8`, without `dofile`, `loadfile`, `require`, `rawset`, `collectgarbage` and `_G`.
- Not available: `io`, `os`, `package` and `debug`. Scripts reach no files and no processes, and there is no `print`.
- `load` only takes text. Lua does not verify bytecode, a damaged one would crash the app.
- `util`, `workflow` and the libraries are read-only: writing to them fails with an error, `util.copy` makes a copy of the script's own. They work with `pairs`, `ipairs` and `#`, `next` sees them empty.
- `setmetatable` takes no `__gc` finalizer, the app could not stop one.
- There is no time limit. A script runs until it returns and other scripts wait meanwhile, one looping endlessly while it loads keeps the scripts after it from loading.
- On exit the app stops the scripts: a running one fails with an error at its next call, catching it does not keep the script running.
