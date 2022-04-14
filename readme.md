# openblack

[![VCPKG CI](https://github.com/openblack/openblack/actions/workflows/ci-vcpkg.yml/badge.svg?branch=master&event=push)](https://github.com/openblack/openblack/actions/workflows/ci-vcpkg.yml?query=branch%3Amaster+event%3Apush)
[![System Deps CI](https://github.com/openblack/openblack/actions/workflows/ci-system-deps.yml/badge.svg?branch=master&event=push)](https://github.com/openblack/openblack/actions/workflows/ci-system-deps.yml?query=branch%3Amaster+event%3Apush)
[![GitHub Stars](https://img.shields.io/github/stars/openblack/openblack?logo=github)](https://github.com/openblack/openblack/stargazers)
[![Discord chat](https://img.shields.io/discord/608729286513262622?logo=discord&logoColor=white)](https://discord.gg/5QTexBU)
[![License](https://img.shields.io/github/license/openblack/openblack)](LICENSE.md)

openblack is an open source reimplementation of [Black & White (2001)](https://en.wikipedia.org/wiki/Black_&_White_(video_game)) written in modern C++ and modern rendering engines (OpenGL, Vulkan).

You still need to have the original game assets in order to use this. See further below for an explanation on how to extract them.

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

### XCode
* Install [XCode](https://apps.apple.com/ca/app/xcode)
* Install the homebrew depenedencies using [homebrew](https://brew.sh/):  `brew install cmake ninja pkg-config`
* Run `sudo xcode-select --reset` after installing cmake
* You need to configure using the cmake preset `xcode-vcpkg` using cmake in either the GUI or using the command line argument `--preset xcode-vcpkg`

### Android Studio
* Install [Android Studio](https://developer.android.com/studio)
* Install a recent SDK and an NDK of at least 23
* Open openblack from the `android` directory
* Set the SDK and NDK in the project settings
* Do gradle configure which will run cmake configuration and compile the vcpkg dependencies
* Build the project for either a virtual device or a connected device
* Currently requires user to upload game assets to `/data/local/tmp/bw` and `chmod -R a+wrx /data/local/tmp/bw` with adb

### Command Line
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

# Extracting asset files from original game

As mentioned before, the original game assets are required in order to run
openblack. Since these data are copyrighted, **we won't provide them**, so please
don't ask. Here is a guide for extracting them from the original game.

## Prerequisites

You will need the following:

- The original installer (usually on a CD)
- Official patch 1.10
- Official patch 1.2
- Not needed (but will not compromise the extraction): Unofficial patch 1.42

Patches can be found on fan sites such as
[Black & White REALM](https://www.bwrealm.com/official-files-patches-1)

## Installation

### On Windows

1. Install the game from the installer, remember the installation directory
(`C:\\Program Files (x86)\\Lionhead Studios\\Black & White\\` is the default)
2. Install the official patch 1.10 (in the same directory)
3. Install the official patch 1.2 (in the same directory)

### On Linux and macOS

If you run on Linux or macOS, you can install the game with Wine
([App Page on WineHQ](https://appdb.winehq.org/objectManager.php?sClass=application&iId=156)).

1. Install `wine`
2. Run `wine Setup.exe` from the directory containing the installer, don't
   change the install path
3. Run `wine Black_White_Patch_v1.100.exe` from the directory containing the
   first patch (adapt the command in case the filename isn't exactly that)
4. Run `wine black_white_patch_v1_20.exe` from the directory containing the
   second patch (adapt too)
5. In case you would like to install the unofficial patch, it requires .NET 2.0,
   that can be installed with `winetricks dotnet20sp2` (you should install
   `winetricks first`)

All the files should be installed in the following directory:
`~/.wine/drive_c/Program Files (x86)/Lionhead Studios Ltd/Black & White`

## Getting the assets

The folders you're interested in are the `Data` and `Scripts` folders in the
installation directory. You can use them from there or make a copy to the
location of your choice.

### Using them in place

If you want to use them from there, remember the installation directory chosen
in the installation step, unless you changed it, it should be either:

- either`C:\\Program Files (x86)\\Lionhead Studios\\Black & White\\`
- or `~/.wine/drive_c/Program Files (x86)/Lionhead Studios Ltd/Black & White`

This is the path you will give to openblack (using the `-g` flag).

If you prefer to copy the data (e.g. if you want to uninstall the original
game), just copy the `Data` and `Scripts` folders to the desired new location,
and give the path to this location to openblack (using the `-g` flag).

# License

openblack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [license](./license) file in the project root for the full license text.
