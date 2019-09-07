# openblack

[![Travis Build](https://img.shields.io/travis/com/openblack/openblack?logo=travis)](https://travis-ci.com/openblack/openblack)
[![Appveyor Build](https://img.shields.io/appveyor/ci/handsomematt/openblack?logo=appveyor)](https://ci.appveyor.com/project/handsomematt/openblack)
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

## Windows

* Install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* Install [CMake](https://cmake.org/download/)
* Open CMake GUI and point it to `openblack` folder, set build directory to `openblack/build`
* Configure the following variables:
  * `SDL2_ROOT_DIR` - [SDL2](https://www.libsdl.org)
  * `GLEW_ROOT_DIR`
  * `GLM_ROOT_DIR`
* Press Configure, Generate, Open Project and then build.

## Linux

Building has been tested on Ubuntu 19.04 and works fine, however GCC7 (e.g. Ubuntu 18.04) should suffice for building.

Dependencies can be installed simply from apt:

```
sudo apt install cmake libglew-dev libsdl2-dev libglm-dev libspdlog-dev
```

Then to build run CMake in a build directory:

```bash
cmake -S . -B build
cmake --build build
```

**Mesa drivers on Linux:** if you are trying to run with Mesa drivers and are getting issues with OpenGL context try messing with `MESA_GL_VERSION_OVERRIDE` when running like so: `MESA_GL_VERSION_OVERRIDE=4.3FC MESA_GLSL_VERSION_OVERRIDE=430 bin/donut`

## Documentation
Documentation on various different file formats used throughout Black and White can all be
found in the `docs` folder in the project root directory.

## Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

## License
openblack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [LICENSE.md](./LICENSE.md) file in the project root for the full license text.
