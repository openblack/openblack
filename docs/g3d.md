A G3D is an archive.

`LiOnHeAd` is the magic header.

High-res textures are first, loop through starting high to low until currentTexture != 1.

```cpp
struct Texture {
    char textureNumberHex[4];
    char whitespace[28];  // appears to be entirely null bytes. (null terminated 32 byte string)
    uint32_t fileEof1;
    uint32_t fileEof2;
    uint32_t currentTexture;
    uint32_t texInfo; // 1 = DXT1 2 = DXT3 (alpha)
    uint32_t fileSize;

    uint8_t[fileSize - 4] textureData; // Without the `DDS ` header.
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

#### Low Res Textures

`char lowHeader[32] = "LOW" ltoa(numTextures)`

Todo...