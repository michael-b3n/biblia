# Development setup (Windows)

## Tools

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [MSYS2](https://www.msys2.org/).
2. In the `MSYS2 MSYS` shell (`msys2_shell.cmd`), update and install the toolchain:

   ```
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-{toolchain,cmake,ninja,clang,lld}
   ```

3. Install the libraries the project builds against:

   ```
   pacman -S --needed mingw-w64-ucrt-x86_64-{boost,catch,cppwinrt,curl,libzip,lua54,pugixml,qt6-declarative,qt6-svg,sol2,spdlog,sqlite3,tesseract-ocr,tesseract-data-deu}
   ```

4. Optional, for clang-tidy and clangd: `pacman -S mingw-w64-ucrt-x86_64-clang-tools-extra`.
5. In Visual Studio Code install at least the [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack), [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) and Code Spell Checker.

## Build

`CMakePresets.json` holds a preset per compiler and build type: `clang-debug`, `clang-release`, `gcc-debug` and `gcc-release`. All build into `build`. CI and releases use `gcc-release`.

The presets expect the compilers and Ninja on the PATH, as in the `MSYS2 UCRT64` shell. In the root directory:

```
cmake --preset clang-debug --fresh
cmake --build --preset clang-debug
ctest --preset clang-debug
```

`--fresh` replaces the configuration of the previous preset.

In Visual Studio Code, CMake Tools can build with kits and variants instead, which put the compiler folder on the PATH. Set `"cmake.useCMakePresets": "never"` in `.vscode/settings.json` to keep them while the presets file is present.

## Packing an installer

The release workflow packs the releases, see the Release section of the [README](../README.md). Packing locally is only needed to test an installer:

1. Install the .NET SDK 10.
2. Run `dotnet tool install --global vpk --version 1.2.0`. The version has to match `libs_external/velopack`.
