# biblia

[![CI](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml)

Libraries that find bible references on the screen, and VerseLens, the Windows tray app built on them.

## Repository

| Folder | Content |
|---|---|
| `arca` | Submodule [arca](https://github.com/michael-b3n/arca): the general libraries `arcstd` and `arcqml` (QML module `ArcQml`), their tests and test helpers, the third party sources |
| `bibocr` | OCR engines (`txt`), the Windows OCR, its language tags and the screen capture (`system`), image and screen types (`data`), the bitmap export of captures (`io`), the languages of the app (`util`) |
| `bibstd` | The Bible library: references and their parsing (`bible`), the workflows that turn a hotkey into a lookup (`workflow`), the scripts shipped with the app (`lua/bundled`), letter tables of the languages (`txt`), the system layer of the app: clipboard, locale, browser, screen and tray (`system`) |
| `bibqml` | The Bible Qt layer, QML module `BibQml`: the bridges of the reference search, the scriptures and the application, the scripture model, the settings views and their display names, the icons of the app |
| `bibstd_test` | Catch2 tests of `bibocr` and `bibstd`. Their scripture zips are local only, see [bibstd_test/res/scripture](bibstd_test/res/scripture/README.md) |
| `verselens` | The app: window, tray, updater and the resources a release ships |
| `libs_external` | Velopack and traypp, used as they are |
| `tools` | CMake helpers, the clang-tidy runner, the Lua reference generator, the icon and MSIX packaging scripts, a VS Code launcher |

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). Clone with `--recurse-submodules`, or run `git submodule update --init` in a clone, for `arca`. In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
cmake --install build
```

- Presets: `clang-debug`, `clang-release`, `gcc-debug`, `gcc-release`. All build into `build`, `--fresh` replaces the configuration of the previous one.
- CI and releases use `gcc-release`.
- `cmake --install` fills `build/install`.
- Static analysis: `tools/run_clang_tidy.ps1`.

## VerseLens

Point the cursor at a reference like "Matthew 23, 10-11" in any window and press `ALT + f`. VerseLens reads the text around the cursor, recognizes the reference and opens it on [bibleserver.com](https://www.bibleserver.com). With the automatic search enabled, resting the cursor on a reference is enough.

- `Lookup > Scripts` opens references on [bible.com](https://www.bible.com) instead, `Lookup > Translations` chooses the translations shown.
- A reference over several chapters opens a browser tab per chapter.
- `Display > Click action` chooses what a click on the found reference does: open it, copy it to the clipboard or nothing. The icon on its right runs the other one.
- `Copy > Format` writes the copied reference, e.g. `"[content]" ([book] [#chapter_begin], [#verse_begin])`. Keys in `[ ]` are replaced, text in `{ }` drops out if all its keys are empty. Keys: `content`, `content_numbered`, `book`, `book_long`, `book_abbr`, `book_end`, `#chapter_begin`, `#verse_begin`, `#chapter_end`, `#verse_end`, `scripture`, `scripture_abbr`, `copyright`, and the characters `nbsp`, `newline` and `tab`.

### Install

Requires Windows 10 or later.

1. Uninstall versions 1.x ("Bible Assistant"), they do not update to 2.x.
2. Download `VerseLens-win-Setup.exe` from the latest [release](https://github.com/michael-b3n/biblia/releases/latest) and run it.

VerseLens installs for the current user and starts at sign-in, which can be turned off in the Task Manager under Startup apps. Updates are downloaded in the background and installed on the next start. The notifications tab installs them right away and checks for them on request, its bell rings once one is ready.

### Scriptures

VerseLens ships without scriptures. To read passages in the app, download USX bundles from the Digital Bible Library at [library.bible](https://library.bible/), or zip archives of USFM files, e.g. from [eBible.org](https://ebible.org/), and import them in the scripture tab. They are kept in `%LOCALAPPDATA%\verselens\scriptures`, read on their import and, after that, loaded from what was read when VerseLens starts. The reload button below the verses reads them anew.

### Scripts

Lua scripts build the urls of the lookup, read the scripture files and can offer scriptures too, e.g. by fetching the verses from a web page.

- The scripts for bibleserver.com and bible.com and the readers of USX and USFM files are part of the app, see [bibstd/lua/bundled](bibstd/lua/bundled).
- Scripts of your own are the `*.lua` files of `%LOCALAPPDATA%\verselens\scripts`, `Import scripts` in the scripts tab copies them there. They are loaded after the start and with `Load scripts` in the scripts tab, unless `Scripts > Enabled` is turned off.
- [example.lua](bibstd/lua/examples/example.lua) is installed with the app in `share/scripts`. Imported it is inactive until its `enabled` is set.
- Mind the terms of use of the pages a script reads and the copyright of the translations.
- Writing scripts: [doc/lua_scripts.md](doc/lua_scripts.md), what the app offers them: [doc/lua_api.md](doc/lua_api.md).

### Release

On the branch `release/verselens_v<major>`, set `APP_VERSION_MAJOR` and `APP_VERSION_MINOR` in `verselens/CMakeLists.txt`, then tag and push:

```
git tag verselens_vX.Y
git push origin verselens_vX.Y
```

The workflow `release_verselens.yml` checks the tag, builds, tests and publishes the release, packed from `build/install` with [Velopack](https://velopack.io). The configure step downloads its prebuilt library. Installed apps check for a release 3 minutes after their start and every 24 hours.

For the Microsoft Store the workflow also packs an MSIX and offers it as a build artifact, which is uploaded to Partner Center by hand.

- The store signs the package and delivers its updates. So it is packed from the preset `gcc-release-msix`, which builds into `build_msix` without the Velopack updater (`-DVERSELENS_VELOPACK=OFF`).
- `tools/make_msix_verselens.ps1` packs the same package locally from an installed `gcc-release-msix` build. It needs the Windows SDK for `makeappx`.

## License

[MIT](LICENSE). The libraries in `libs_external` and `arca/libs_external` keep their own licenses. Qt is used under the LGPLv3, Tesseract and its `tessdata` under the Apache License 2.0. Scriptures are not part of this repository or of a release.
