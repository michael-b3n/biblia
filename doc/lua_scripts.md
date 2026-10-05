# Lua scripts

VerseLens loads the `*.lua` files of the script folder once after the start, in the order of their names, if `Scripts > Enabled` is set. The folder is `%LOCALAPPDATA%\verselens\scripts` unless `Scripts > Folder` names another one. It is created empty on first use. An example of a script, [example.lua](../bibstd/lua/examples/example.lua), is installed with the app in `share/scripts`. Copied into the script folder it is inactive until its `enabled` is set. Changes to scripts or to these settings take effect on the next start, or right away with `Load scripts` in the scripts tab, which lists the loaded scripts and their functions.

The scripts of [bibstd/lua/bundled](../bibstd/lua/bundled) are part of the app, e.g. the lookup on bibleserver.com. They are written like the scripts of the folder and loaded before them, also without `Scripts > Enabled`, so their ids are taken.

## Functions of a script

A script returns a table describing itself:

| Key | Value |
|---|---|
| `id` | What the app knows the script by, of letters, digits, `_` and `-`, e.g. `example`. Other characters are replaced by `_`. The name of the file plays no role. Of two scripts with the same id the first one loaded is kept. |
| `name` | The name shown to the user, e.g. `Example`. |
| `functions` | The functions the script offers. Each is named after a manifest of the app, which defines its input and output. Entries that are no named function are left out. |

A script returning no table is loaded but offers nothing. One without a valid `id`, a `name` or `functions` is rejected, the log tells its file.

The code of the file runs once while loading, that is how Lua defines the functions. Keep it to definitions, the work belongs into the functions, which run each time the app needs their output.

```lua
return {
  id = "example",
  name = "Example",
  functions = {
    ["scripture.passage"] = function(input)   -- called by the app with a table of the input
      return { text = "..." }                 -- a table of the output, or nil for none
    end,
  },
}
```

Input and output are tables of the keys of the manifest, each with a value of its type: a string, an integer, a number, a boolean, a table of values in sequence like `{ "a", "b" }` or a table of values by key like `{ a = 1 }`, whose keys may be of any type but nil. An integer is a Lua integer, `4 // 2` is one, `4 / 2` is the float `2.0`. An optional value may be nil or left out, `nil` returned for the output is a table without keys. An output with a value of another type, without a value it must have or with a key the manifest does not have is rejected, the log names the function. A failing function gives no output.

The functions run outside of the user interface, one at a time, and may read web pages with `workflow.web.fetch`.

## Scriptures

A script offering the four functions below offers scriptures, several scripts may. Their scriptures join those of `Scripture > Name`, named after the id of their script, e.g. `LUT (Bibleserver)`, and they use the fallback versification.

| Manifest | Input | Output |
|---|---|---|
| `scripture.names` | | `names`: string_table of the scriptures the script offers, e.g. `{ "ABC" }` |
| `scripture.information` | `name` | `abbreviation`, `language`, `copyright`: strings, the copyright is shown below the verses |
| `scripture.book` | `name`, `book` | `abbreviation`, `short_name`, `long_name`: strings, the short name is shown, missing ones show the identifier |
| `scripture.passage` | `name`, `book`, `chapter`, `verse` | `text`: the plain text of the verse, markup of a page is shown as text. Without it the verse shows "...". |

`name` is one of the names the script offers, `book` the identifier of the app, e.g. `john`, `chapter` and `verse` are integers. `scripture.passage` runs whenever a verse is shown, keeping fetched verses in `workflow.cache` is up to the script, see `example.lua`. Mind the terms of use of the pages a script reads.

## Lookup

A script offering the two functions below can open references in the browser. `Lookup > Scripts` chooses one of them. [lookup_bibleserver.lua](../bibstd/lua/bundled/lookup_bibleserver.lua), the default, and [lookup_bible_com.lua](../bibstd/lua/bundled/lookup_bible_com.lua) are bundled with the app. A script is named after what it offers and the page it uses, a script offering the scriptures of bibleserver.com would be `scripture_bibleserver`.

| Manifest | Input | Output |
|---|---|---|
| `lookup.translations` | `language` | `names`: string_table of the translations to choose from in `Lookup > Translations`, may be empty. `defaults`: the ones chosen as long as the user chose none, optional. |
| `lookup.url` | `translations`, `book`, `chapter`, `verse_begin`, `verse_end` | `url`: the web page showing these verses, or none |

`language` is the language of the user, `english` or `german`. `translations` are the chosen names in the order chosen, `book` is the identifier of the app, e.g. `john`, the others are integers. A reference over several chapters asks for a url per chapter, each is opened in a tab of its own. Only a valid `http` or `https` url is opened, so encode what is not ASCII.

## Sandbox

Every script has an environment of its own, its globals do not reach other scripts. The libraries `base`, `coroutine`, `math`, `string`, `table` and `utf8` are available, without `dofile`, `loadfile`, `require`, `rawset` and `_G`. `io`, `os`, `package` and `debug` are not, scripts reach no files and no processes. `load` only takes text, Lua does not verify bytecode and a damaged one would crash the app instead of failing with an error. There is no console and so no `print`, `util.log_info` writes to the log of the app.

What scripts share is read-only for them: the libraries, `util` and `workflow`. Writing to them fails with an error, a copy made with `util.copy` is the script's own. `setmetatable` takes no `__gc` finalizer, the app could not stop one, and there is no `collectgarbage`, the collector is shared by all scripts. Read-only tables work with `pairs`, `ipairs` and `#`, `next` sees them empty.

A script runs until it returns, there is no time limit, and other scripts wait meanwhile. On exit the app stops the scripts: a running one fails with an error, catching it does not keep the script running. The stop is checked between calls, a single library call like a pattern search on a large text is stopped once it returns. A script looping endlessly while it loads keeps the scripts after it from loading.

## Functions of the app

The workflows of the app offer their functions below `workflow`, the helpers of `util` come with every state.

A function of the app called the wrong way, e.g. with a value of another type, raises an error like the functions of Lua do, `pcall` catches it. A failure the script can not prevent, e.g. a missing web page, is no error: the function returns `nil` and the reason, like `io.open`.

| Function | Description |
|---|---|
| `workflow.web.fetch(url)` | Content of the web page at `url` (http or https), or `nil` and the reason it failed. The script waits for the page, other scripts wait meanwhile, so keep what you need in `workflow.cache`. |
| `workflow.cache.get(name, key)` / `workflow.cache.set(name, key, value)` | Text values kept beyond the run of the app, in the cache `name` of letters, digits, `_` and `-`. A script names its own cache, e.g. after its id. `value = nil` removes the key. Each cache is the SQLite file `cache/<name>.sqlite` of the script folder, other programs may read and change it meanwhile. Setting values one by one is fast, a failing cache is logged and holds nothing. |
| `util` | Helpers for strings (`split`, `trim`, `starts_with`, `ends_with`), tables (`keys`, `contains`, `map`, `filter`, `copy`), inspection (`dump`, `at`) and the log (`log_debug`, `log_info`, `log_warning`, `log_error`, each taking values like `print` does). |
