<p align="center">
  <img src="https://github.com/openblack.png"/><br>
  <i>OpenBlack</i><br>
  A reimplementation of the Black & White game engine
</p>

### Building

This needs to be documented further in depth, but it is made as simple as a possible with CMake.

Dependencies required:

* SDL2
* GLEW
* GLM

#### Windows

Starting with Visual Studio 2017, the project folder can simply be opened with Visual Studio and built.

#### Linux

I have tested this build personally on Ubuntu 19.04, if someone wants to elaborate
on these instructions further, please feel free.

Installing dependencies:

```
sudo apt install cmake libglew-dev libsdl2-dev libglm-dev
```

And then simply running `cmake && make` should suffice.

### Documentation
Documentation on various different file formats used throughout Black and White can all be
found in the `docs` folder in the project root directory.

### Contributing
Contributions are always welcome, whether it's modifying source code to add new
features or bug fixes, documenting new file formats or simply editing some
grammar.

## License
OpenBlack is released as open source software under the [GPL v3](https://opensource.org/licenses/gpl-3.0.html) 
license, see the [LICENSE.md](./LICENSE.md) file in the project root for the full license text.
