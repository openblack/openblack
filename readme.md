# openblack

[![CI](https://github.com/openblack/openblack/actions/workflows/ci.yml/badge.svg)](https://github.com/openblack/openblack/actions/workflows/ci.yml)
[![GitHub Stars](https://img.shields.io/github/stars/openblack/openblack?logo=github)](https://github.com/openblack/openblack/stargazers)
[![Discord chat](https://img.shields.io/discord/608729286513262622?logo=discord&logoColor=white)](https://discord.gg/5QTexBU)
[![License](https://img.shields.io/github/license/openblack/openblack)](LICENSE.md)

openblack is an open source reimplementation of [Black & White (2001)](https://en.wikipedia.org/wiki/Black_&_White_(video_game)) written in modern C++ and modern rendering engines (OpenGL, Vulkan).

You still need to have the original game assets in order to use this, don't ask where to get these.

---

<a href="https://user-images.githubusercontent.com/1388267/67631321-93c85380-f88c-11e9-9103-804807844af2.png">
    <img src="https://user-images.githubusercontent.com/1388267/67631321-93c85380-f88c-11e9-9103-804807844af2.png" width="45%">
</a>
<a href="https://user-images.githubusercontent.com/1388267/63901712-bad5e680-c9fd-11e9-8000-9de22ad8054e.png">
    <img src="https://user-images.githubusercontent.com/1388267/63901712-bad5e680-c9fd-11e9-8000-9de22ad8054e.png" width="45%">
</a>

# Building

Clone the code using: `git clone --recursive https://github.com/openblack/openblack.git`

The simplest way to obtain all the required dependencies is through [vcpkg](https://github.com/Microsoft/vcpkg).

```bash
PS> .\vcpkg install --triplet x64-windows sdl2 spdlog glm entt cxxopts
Linux:~/$ ./vcpkg install sdl2 spdlog glm entt cxxopts
```

If you don't want to use vcpkg; CMake will fallback on installed system dependencies, or manually specified
package directories.

## Windows

* Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* Install [CMake](https://cmake.org/download/) (3.15+ for Windows)
* Install [bgfx.cmake](https://github.com/openblack/bgfx.cmake/releases/tag/latest)

You can either:

1. [Open the `openblack` folder directly in Visual Studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019).
2. Generate project files with the following:

```bash
cd openblack
cmake -S . -B build -Dbgfx_DIR=C:/Users/Matt/Development/bgfx-windows-x64/lib/cmake/bgfx
```

_Replace `C:/Users/Matt/Development/bgfx-windows-x64` with the folder you extracted bgfx to._

## Linux

*Note: The instructions are for Ubuntu can be easily applied to other distros. Alternatively, the commands of the Arch Linux PKGBUILD can be used as a reference.*

### Ubuntu

* Install [bgfx.cmake](https://github.com/openblack/bgfx.cmake)

Ensure you have dependencies first
```bash
sudo apt install cmake
cd openblack
cmake -S . -B build -DOPENBLACK_USE_VCPKG=true -DVCPKG_TARGET_TRIPLET=x64-linux
cmake --build build -j 5
```

### Arch Linux

Install [openblack-git](https://aur.archlinux.org/packages/openblack-git/) from the AUR which builds and installs directly for all Arch Linux derived distros such as Manjaro.

## Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

You can also join the [Discord for development discussion]((https://discord.gg/5QTexBU))
if you are unsure of anything.

## License
openblack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [license](./license) file in the project root for the full license text.
