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

The simplest way to obtain all the required dependencies is through [vcpkg](https://github.com/Microsoft/vcpkg) which is included with a [manifest file](https://github.com/microsoft/vcpkg/blob/master/docs/users/manifests.md).

If you don't want to use vcpkg; CMake will use system dependencies, or manually specified package directories.

## Windows

* Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* Install [CMake](https://cmake.org/download/) (3.15+ for Windows)

The easiest way to get started on Windows is to allow CMake and vcpkg to handle all dependencies
for you, this is the default on Windows.

You can simply [open the `openblack` folder directly in Visual Studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019).

Alternatively you can generate your project files using CMake GUI or with CMake directly:

```bash
cd openblack
cmake -S . -B build
```

## Linux

Using vcpkg for dependency management you can apply these build instructions to other distros,
these are just tested on Ubuntu for simplicity though:

```bash
sudo apt install build-essential cmake # Ubuntu / Debian
cd openblack
cmake -S . -B build -DOPENBLACK_USE_VCPKG=true -DVCPKG_TARGET_TRIPLET=x64-linux -DOPENBLACK_USE_BUNDLED_BGFX=true
cmake --build build -j 5
```

### System Dependencies

Alternative to vcpkg you can use system dependencies by setting `OPENBLACK_USE_VCPKG=false` (default: false)
CMake will find them as long as they provide a [proper config file](https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html#libraries-providing-config-file-packages).

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
