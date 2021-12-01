# openblack

[![VCPKG CI](https://github.com/openblack/openblack/actions/workflows/ci-vcpkg.yml/badge.svg)](https://github.com/openblack/openblack/actions/workflows/ci-vcpkg.yml)
[![System Deps CI](https://github.com/openblack/openblack/actions/workflows/ci-system-deps.yml/badge.svg)](https://github.com/openblack/openblack/actions/workflows/ci-system-deps.yml)
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

## Configuration for using vcpkg (Recommended for new users)

The simplest way to obtain all the required dependencies is through [vcpkg](https://github.com/Microsoft/vcpkg) which is included with a [manifest file](https://github.com/microsoft/vcpkg/blob/master/docs/users/manifests.md).

The easiest way to get started on any platform is to allow CMake and vcpkg to handle all dependencies and configuration
for you. To do so, you will be selecting the `"ninja-multi-vcpkg"   - Ninja Multi-Config (vcpkg)` preset. Other presets are available for more advanced users.

### Visual Studio Code
 * Install [Visual Studio Code](https://code.visualstudio.com/Download)
 * Install the [C++ Extension](vscode:extension/ms-vscode.cpptools)
    * The extension might not be required on Linux
    * Install a compiler depending on your platform
        * GNU or Clang compilers on Linux
        * Visual Studio's MSVC on Windows (See Visual Studio)
        * Clang compiler on Mac
 * Install the [CMake Tools Extension](vscode:extension/ms-vscode.cmake-tools)
     * Install [CMake version 3.20+](https://cmake.org/download/)
 * You can simply [open the `openblack` folder directly in Visual Studio Code and select a preset](https://devblogs.microsoft.com/cppblog/cmake-presets-integration-in-visual-studio-and-visual-studio-code/).

### Visual Studio 2019 / 2022
* Install [Visual Studio](https://visualstudio.microsoft.com/downloads/)
    * Select an appropriate MSVC C++ Component for your version
    * Select an appropriate Windows SDK Component for your version
    * Select the C++ CMake tools for Windows Component
* You can simply [open the `openblack` folder directly in Visual Studio and select a preset](https://devblogs.microsoft.com/cppblog/cmake-presets-integration-in-visual-studio-and-visual-studio-code/).

### Clion
* Install [Clion](https://www.jetbrains.com/clion/download/)
* You can simply [open the `openblack` folder directly in CLion and select a preset](https://www.jetbrains.com/help/clion/cmake-presets.html).

### Commandline
* Your usual build tool-chain.
    * Ubuntu / Debian: `# apt install build-essential cmake ninja-build`
    * Arch Linux / Manjaro: `# pacman -S base-devel cmake ninja`
* You can generate the cmake build preset using `cmake --preset` and you can list the presets using `cmake --list-presets`

## Configuration for using System Dependencies (Recommended for packagers)

If you don't want to use vcpkg; CMake will use system libraries, or manually specified package directories.

CMake will find the required libraries as long as they provide a [proper config file](https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html#libraries-providing-config-file-packages).

### Arch Linux

Install [openblack-git](https://aur.archlinux.org/packages/openblack-git/) from the AUR which builds and installs directly for all Arch Linux derived distros such as Manjaro. This will require pacman to install all required dependencies.

The prefix to use is `"linux-system-deps"   - Linux (System Deps)`

## Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

You can also join the [Discord for development discussion]((https://discord.gg/5QTexBU))
if you are unsure of anything.

## License
openblack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [license](./license) file in the project root for the full license text.
