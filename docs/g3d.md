# Black & White G3D Archive

A G3D file is an archive containing meshes and textures used by the game.
The only file with this format is the `AllMeshes.g3d` file in the game's `Data` folder.

The file has a magic header `LiOnHeAd` which can be used to identify it from other files.

Each mesh in this file can be described with the subsequent `AllMeshes.h` file which
contains ordered enums for each mesh in the file, these can be read from the `MESHES`
offsets.

The file contains sections, some of which are prefixed for the game to find information
easily:

* High Resolution Textures - These are first and have no identifying header, but can be
read instantly from the start of the file (after the magic header).
* Info Section - Identified by the header `INFO` this describes how many textures there
are, their ID and their type (DXT1/DXT3).
* Meshes Section - Identified by the header `MESHES` this has offsets and data for each
mesh in game.
* Low Resolution Textures - Identified by the header `LOW`, these are lower resolution
textures used by the game, in place of the high resolution ones.

#### High Resolution Textures

High-res textures are first, loop through starting high to low until currentTexture != 1.

```cpp
struct Texture {
    char textureNumberHex[32];
    uint32_t fileEof1;
    uint32_t fileEof2;
    uint32_t currentTexture;
    uint32_t texInfo; // 1 = DXT1 2 = DXT3 (alpha)
    uint32_t fileSize;

    uint8_t textureData[fileSize - 4]; // Without the `DDS ` header.
}
```

#### Info Section

`char infoHeader[32] = "INFO"`
`uint32_t unk = 4100`
`uint32_t numTextures`

foreach:
{
`uint32_t textureID`
`uint32_t textureType` (DXT1/DXT3)
}

148 whitespace bytes
24 bytes of ???
3044 UNKNOWN (filler?) could be the array above is meant to extend down here.

#### Meshes Section

`char meshesHeader[32] = "MESHES"`
`26, 53, 106, 0`
`MKJC`

Offsets first
Then data

#### Low Resolution Textures

`char lowHeader[32] = "LOW" ltoa(numTextures)`

For each texture:

`uint32_t size + 12`
`uint8_t whitespace[12]`
`uint32_t size`
`uint8_t data[size]`