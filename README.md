# biblia

[![CI](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml)

Libraries that find bible references on the screen, and VerseLens, the Windows tray app built on them.

## Repository

| Folder | Content |
|---|---|
| `bibstd` | The library and most of the logic: references and their parsing (`bible`), the workflows that turn a hotkey into a lookup (`workflow`), OCR engines and text scripts (`txt`), scripture store, cache and web fetch (`core`), Lua scripting (`lua`, the scripts shipped with the app in `lua/bundled`), threading and settings (`framework`), the system layer (`system`, Windows in `system/windows`) |
| `bibqml` | The Qt layer: bridge between QML and the library, models, shared controls |
| `bibstd_test` | Catch2 tests of `bibstd`. Their scripture zips are local only, see [bibstd_test/res/scripture](bibstd_test/res/scripture/README.md) |
| `verselens` | The app: window, tray, updater and the resources a release ships |
| `libs_external` | Third party sources, used as they are |
| `tools` | CMake helpers, the clang-tidy runner, the MSIX packaging script |

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). In the MSYS2 UCRT64 shell:

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

### Install

Requires Windows 10 or later.

1. Uninstall versions 1.x ("Bible Assistant"), they do not update to 2.x.
2. Download `VerseLens-win-Setup.exe` from the latest [release](https://github.com/michael-b3n/biblia/releases/latest) and run it.

VerseLens installs for the current user and starts at sign-in, which can be turned off in the Task Manager under Startup apps. Updates are downloaded in the background and installed on the next start. The notifications tab installs them right away and checks for them on request, its bell rings once one is ready.

### Scriptures

VerseLens ships without scriptures. To read passages in the app, download USX bundles from the Digital Bible Library at [library.bible](https://library.bible/) and put the zip files into `%LOCALAPPDATA%\verselens\scriptures`. An install from the Microsoft Store keeps this folder inside its package data, `Scripture > Folder` shows where. They are loaded on start. `Scripture > Folder` names another folder.

### Scripts

Lua scripts build the urls of the lookup and can offer scriptures too, e.g. by fetching the verses from a web page.

- The scripts for bibleserver.com and bible.com are part of the app, see [bibstd/lua/bundled](bibstd/lua/bundled).
- Scripts of your own are the `*.lua` files of `%LOCALAPPDATA%\verselens\scripts`, `Scripts > Folder` names another folder. They are loaded after the start and with `Load scripts` in the scripts tab, unless `Scripts > Enabled` is turned off.
- [example.lua](bibstd/lua/examples/example.lua) is installed with the app in `share/scripts`. Copied into the script folder it is inactive until its `enabled` is set.
- Mind the terms of use of the pages a script reads and the copyright of the translations.
- Writing scripts: [doc/lua_scripts.md](doc/lua_scripts.md).

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

[MIT](LICENSE). The libraries in `libs_external` keep their own licenses. Qt is used under the LGPLv3, Tesseract and its `tessdata` under the Apache License 2.0. Scriptures are not part of this repository or of a release.
