# Setup IDE (Windows)

1.  download and install Visual Studio Code [here](https://code.visualstudio.com/)
2.  download and install MSYS2 [here](https://www.msys2.org/)
3.  open `MSYS2 MSYS` (`msys2_shell.cmd`)
4.  run `pacman -Syu`
5.  run `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`
6.  run `pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja`
7.  run `pacman -S mingw-w64-ucrt-x86_64-clang`
8.  run `pacman -S mingw-w64-ucrt-x86_64-lld`
9.  open Visual Studio Code and install atleast
    - C/C++ Extension Pack [here](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
    - Clang-Format [here](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
    - Code Spell Checker

## Add mingw dependencies (Windows)

### Required

- **Boost** run `pacman -S mingw-w64-ucrt-x86_64-boost` in `msys2_shell.cmd`
- **libzip** run `pacman -S mingw-w64-ucrt-x86_64-libzip` in `msys2_shell.cmd`
- **Lua 5.4** run `pacman -S mingw-w64-ucrt-x86_64-lua54` in `msys2_shell.cmd`
- **Catch2** run `pacman -S mingw-w64-ucrt-x86_64-catch` in `msys2_shell.cmd`
- **Curl** run `pacman -S mingw-w64-ucrt-x86_64-curl` in `msys2_shell.cmd`
- **pugixml** run `pacman -S mingw-w64-ucrt-x86_64-pugixml` in `msys2_shell.cmd`
- **sol2** run `pacman -S mingw-w64-ucrt-x86_64-sol2` in `msys2_shell.cmd`
- **Qt6 SVG** run `pacman -S mingw-w64-ucrt-x86_64-qt6-svg` in `msys2_shell.cmd`
- **Qt6** run `pacman -S mingw-w64-ucrt-x86_64-qt6-declarative` in `msys2_shell.cmd`
- **Spdlog** run `pacman -S mingw-w64-ucrt-x86_64-spdlog` in `msys2_shell.cmd`
- **SQLite** run `pacman -S mingw-w64-ucrt-x86_64-sqlite3` in `msys2_shell.cmd`
- **Tesseract Data (deu)** run `pacman -S mingw-w64-ucrt-x86_64-tesseract-data-deu` in `msys2_shell.cmd`
- **Tesseract** run `pacman -S mingw-w64-ucrt-x86_64-tesseract-ocr` in `msys2_shell.cmd`
- **WinRT** run `pacman -S mingw-w64-ucrt-x86_64-cppwinrt` in `msys2_shell.cmd` (Windows only)

### Optional

- **Clang Tools Extra** run `pacman -S mingw-w64-ucrt-x86_64-clang-tools-extra` in `msys2_shell.cmd`

## Deployment

Releases are packed by the release workflow, see the Release section of the README. Packing locally is only needed to test an installer:

- **.NET SDK 10** download and install
- **vpk** run `dotnet tool install --global vpk --version 1.2.0`, the version has to match `libs_external/velopack`

## Build

Clang and GCC are both supported. `CMakePresets.json` holds a preset for each combination of compiler and build type: `clang-debug`, `clang-release`, `gcc-debug` and `gcc-release`, all building into `build`. CI and releases use `gcc-release`.

The presets expect the compilers and Ninja on the PATH, as in the `MSYS2 UCRT64` shell:

1.  cd to root directory
2.  run `cmake --preset clang-debug --fresh`, `--fresh` replaces the configuration of the previous preset
3.  run `cmake --build --preset clang-debug`
4.  run `ctest --preset clang-debug`

In Visual Studio Code, CMake Tools can build with kits and variants instead, which put the compiler folder on the PATH. Set `"cmake.useCMakePresets": "never"` in `.vscode/settings.json` to keep them when the presets file is present.
