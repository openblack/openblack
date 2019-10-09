# openblack

[![Travis Build](https://img.shields.io/travis/com/openblack/openblack/master?logo=travis)](https://travis-ci.com/openblack/openblack)
[![Appveyor Build](https://img.shields.io/appveyor/ci/handsomematt/openblack/master?logo=appveyor)](https://ci.appveyor.com/project/handsomematt/openblack)
[![GitHub Stars](https://img.shields.io/github/stars/openblack/openblack?logo=github)](https://github.com/openblack/openblack/stargazers)
[![Discord chat](https://img.shields.io/discord/608729286513262622?logo=discord&logoColor=white)](https://discord.gg/5QTexBU)
[![License](https://img.shields.io/github/license/openblack/openblack)](LICENSE.md)

openblack is an open source reimplementation of [Black & White (2001)](https://en.wikipedia.org/wiki/Black_&_White_(video_game)) written in modern C++ and modern OpenGL.

You still need to have the original game assets in order to use this.

---

<a href="https://user-images.githubusercontent.com/1388267/63901709-b8738c80-c9fd-11e9-9d1e-da728515ac0e.png">
    <img src="https://user-images.githubusercontent.com/1388267/63901709-b8738c80-c9fd-11e9-9d1e-da728515ac0e.png" width="45%">
</a>
<a href="https://user-images.githubusercontent.com/1388267/63901712-bad5e680-c9fd-11e9-8000-9de22ad8054e.png">
    <img src="https://user-images.githubusercontent.com/1388267/63901712-bad5e680-c9fd-11e9-8000-9de22ad8054e.png" width="45%">
</a>

# Building

Clone the code using: `git clone --recursive https://github.com/openblack/openblack.git`

The simplest way to get going is to install the required dependencies using vcpkg:
`vcpkg install sdl2 spdlog glm entt`

If you don't want to use vcpkg, CMake will fallback on installed system dependencies,
or manually specified package directories.

## Windows

* Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* Install [CMake](https://cmake.org/download/)
* Open CMake GUI and point it to `openblack` folder, set build directory to `openblack/build`
* Ensure `HUNTER_ENABLED=1` which will download all dependencies automatically for you.
* Press Configure, Generate, Open Project and then build.

## Linux

Dependencies can be installed simply from your package manager, compiled from source
or you can enable Hunter to manage dependencies for you.

```bash
sudo apt install cmake libsdl2-dev libglm-dev libspdlog-dev
git clone --recursive https://github.com/openblack/openblack.git
cd openblack
cmake -S . -B build
cmake --build build
```

## Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

You can also join the [Discord for development discussion]((https://discord.gg/5QTexBU))
if you are unsure of anything.

## License
openblack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [LICENSE.md](./LICENSE.md) file in the project root for the full license text.
