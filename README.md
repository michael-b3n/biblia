# biblia

[![CI](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/biblia/actions/workflows/ci.yml)

Libraries that find bible references on the screen, and VerseLens, the app built on them.

## Repository

The app is the smaller part of this repository, the library below it holds the logic.

- `bibstd` — the library: bible references and their parsing (`bible`), the workflows that turn a hotkey into a lookup (`workflow`), OCR engines and text scripts (`txt`), scripture store, cache and web fetch (`core`), the Lua scripting and the scripts bundled with the app (`lua`), threading and settings (`framework`), and a system layer whose Windows implementations sit in `system/windows`.
- `bibqml` — the Qt layer: the bridge between QML and the library, models and shared controls.
- `bibstd_test` — Catch2 tests of `bibstd`. Their scripture zips are local only, see `bibstd_test/res/scripture`.
- `verselens` — the app: window, tray, updater and the resources a release ships.
- `libs_external` — third party sources, used as they are.
- `tools` — CMake helpers, the clang-tidy runner and the MSIX packaging script.

## Development

In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
cmake --install build
```

The presets `clang-debug`, `clang-release`, `gcc-debug` and `gcc-release` all build into `build`, `--fresh` replaces the configuration of the previous preset. CI and releases use `gcc-release`, `cmake --install` fills `build/install`. Static analysis: `tools/run_clang_tidy.ps1`.

## VerseLens

VerseLens is a Windows tray app that finds bible references on the screen. Point the cursor at a reference like "Matthew 23, 10-11" in any window and press `ALT + f`: VerseLens reads the text around the cursor, recognizes the reference and opens it on [bibleserver.com](https://www.bibleserver.com). With the automatic search enabled, resting the cursor on a reference is enough.

### Install

Download `VerseLens-win-Setup.exe` from the latest [release](https://github.com/michael-b3n/biblia/releases/latest) and run it. VerseLens installs for the current user and starts at sign-in. New versions are downloaded in the background and installed on the next start, or right away from the notifications tab, whose bell rings once an update is ready. The tab also checks for updates on request. Start at sign-in can be turned off in the Task Manager under Startup apps.

Uninstall versions 1.x ("Bible Assistant") first, they do not update to 2.x.

Requires Windows 10 or later.

### Scriptures

VerseLens ships without scriptures. To read passages in VerseLens, download USX bundles from the Digital Bible Library at [library.bible](https://library.bible/) and put the zip files into `%LOCALAPPDATA%\verselens\scriptures`. They are loaded on start, the folder can be changed in the settings.

### Scripts

Lua scripts can provide scriptures too, e.g. fetching the verses from a web page. With `Scripts > Enabled` set in the settings, VerseLens loads the scripts of `%LOCALAPPDATA%\verselens\scripts` after the start and offers the scriptures of the scripts next to the others. The folder is created on first use. An example of a script, [example.lua](bibstd/lua/examples/example.lua), is installed with the app in `share/scripts`. Copied into the script folder it is inactive until its `enabled` is set. Mind the terms of use of the pages a script reads and the copyright of the translations. Writing scripts: [doc/lua_scripts.md](doc/lua_scripts.md).

### Release

On the branch `release/verselens_v<major>`, set `APP_VERSION_MAJOR` and `APP_VERSION_MINOR` in `verselens/CMakeLists.txt`, then tag and push:

```
git tag verselens_vX.Y
git push origin verselens_vX.Y
```

The release workflow `release_verselens.yml` checks the tag, builds, tests and publishes the release, packed from `build/install` with [Velopack](https://velopack.io), whose prebuilt library the configure step downloads. Installed apps pick it up within a day.

The workflow also packs an MSIX for the Microsoft Store and offers it as a build artifact, which is uploaded to Partner Center by hand. The store signs the package and delivers its updates, so it is packed from the preset `gcc-release-msix`, which builds into `build_msix` without the Velopack updater (`-DVERSELENS_VELOPACK=OFF`). `tools/make_msix_verselens.ps1` packs the same package from an installed `gcc-release-msix` build locally, it needs the Windows SDK for `makeappx`.

## License

[MIT](LICENSE). The libraries in `libs_external` keep their own licenses, Qt is used under the LGPLv3, Tesseract and its `tessdata` under the Apache License 2.0. Scriptures are not part of this repository or of a release.
