<p align="center">
  <img src="https://github.com/openblack.png"/><br>
  <i>OpenBlack</i><br>
  A reimplementation of the Black & White game engine
</p>

## What's this?
OpenBlack aims to be an open-source reimplementation of [Black & White (2001)](https://en.wikipedia.org/wiki/Black_&_White_(video_game)), an award winning god game by [Lionhead Studios](https://en.wikipedia.org/wiki/Lionhead_Studios).

You still need to have the original game assets in order to use this.

## Features
* Multi-platform: Windows and Linux support.
* Graphics rendering in modern OpenGL.

## Building

You will need the following to build OpenBlack:

* [CMake 3.12+](https://cmake.org)
* [SDL2](https://www.libsdl.org)
* [GLEW]()
* [GLM]()

### Windows

**Visual Studio 2017 or higher is required.**

#### Generate project files with CMake

You will need to download the above dependencies manually and configure their paths in CMake by defining:

* `SDL2_ROOT_DIR`
* `GLEW_ROOT_DIR`
* `GLM_ROOT_DIR`

You can then generate project files and compile the project.

#### Open project with VS2017+ using Ninja

You should be able to open the project folder if you have CMake support installed on VS2017+.

### Linux

Building has been tested on Ubuntu 19.04 and works fine, however **GCC 9.1.0+ is required** due to bugs in earlier versions with C++17 std.

Dependencies can be installed simply from apt:

```
sudo apt install cmake libglew-dev libsdl2-dev libglm-dev
```

Running `cmake .` in the project root directory will configure a Makefile with all the required dependencies, `make` should then build the project.

## Documentation
Documentation on various different file formats used throughout Black and White can all be
found in the `docs` folder in the project root directory.

## Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

## License
OpenBlack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html)
license, see the [LICENSE.md](./LICENSE.md) file in the project root for the full license text.
