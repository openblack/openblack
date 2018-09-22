# Black & White Landscape Files

Black & White's Landscape Files are stored in the `Data/Landscape` folder and have the file extension `.lnd`.

Landscape Files are all that's needed to fully construct and render the land. These files contain multiple sections consisting of:

* Low Resolution Textures - DXT3 compressed textures to be applied directly onto blocks. These are preblended country textures w/ map details on top in an incredibly low resolution.
* Blocks - A variable amount of `Cell[17*17]` which contain data about the structural layout of the landscape.
* Countries - These define the type of terrain and what map materials to use and blend at certain altitudes. Each cell has a country identifier on it.
* Materials - All the textures used by the landscape's *countries* - these are stored as `uint16_t[256*256]` in the `RGB5_A1` format.
* Noisemap - A `uint8_t[256*256]` texture.
* Bumpmap - A `uint8_t[256*256]` texture.

## Format

Landscape files have no identifiable magic header, thus can't be easily checked for file validity.
They follow a constant order and can be read easily by cycling through the following sections:

### Header

```cpp
struct LH3DLandHeader {
    uint32_t blockCount; // The number of Block structs.
    uint8_t  indexBlock;
    uint32_t materialCount; // The number of Material structs.
    uint32_t countryCount; // The number of Country structs.
    uint32_t blockSize; // The size of the Block struct.
    uint32_t materialSize; // The size of the Material struct.
    uint32_t countrySize; // The size of the Country struct.
    uint32_t lowresTextureCount; // The number of low resolution textures.
}
```

### Low Resolution Textures

For each `LH3DLandHeader.lowresTextureCount`:

```cpp
struct LNDLowresTexture
{
	uint32_t textureSize;
	uint8_t texture[]; // of textureSize
};
```

### Land Blocks

`LH3DLandHeader.blockCount` appears to always be `+1` of the actual amount of blocks to loop through.
A huge portion of the LandBlock struct appears to be runtime data and never used nor required whilst loading.

```cpp
struct LH3DLandCell
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t l;

	uint8_t altitude;
	uint8_t savecolor;

    // Bit flags defining country, cell split, cell properties, sound properties.
	uint16_t flags;
};

struct LH3DLandBlock
{
	LH3DLandCell cells[289]; // 17*17
	uint32_t index;
	float mapY;
	float mapX;
	uint32_t blockY;
	uint32_t blockX;
	uint32_t clipped; // Runtime
	uint32_t frameVisibility; // Runtime
	uint32_t highestAltitude; // Runtime
	uint32_t useSmallBump; // Runtime
	uint32_t forceLowResTex; // Runtime
	uint32_t meshLOD;// Runtime
	uint32_t meshBlending; // Runtime
	uint32_t textureBlend; // Runtime
	uint32_t meshLODType; // Runtime
	uint32_t fog; // Runtime
	uint32_t texPointer; // Runtime
	uint32_t matPointer; // Runtime
	uint32_t drawSomething; // Runtime
	uint32_t specMatBeforePtr; // Runtime
	uint32_t specMatAfterPtr; // Runtime
	float transformUVBefore[3][4];
	float transformUVAfter[3][4];
	uint32_t nextSortingPtr;
	float valueSorting;
	float lowResTexture;
	float fu_lrs; // (iu_lrs / 256)
	float fv_lrs; // (iv_lrs / 256)
	float iu_lrs; // lowrestex x
	float iv_lrs; // lowrestex y
	uint32_t smallTextUpdated;
}
```

### Countries

For each `LH3DLandHeader.countryCount`:

```cpp
struct LNDMapMaterial
{
	uint32_t FirstMaterialIndex;
	uint32_t SecondMaterialIndex;
	uint32_t Coeficient;
}

struct LNDCountry
{
	uint32_t TerrainType; // Enum
	LNDMapMaterial MapMaterial[256]; // altitude 0-255
}
```

### Materials

For each `LH3DLandHeader.materialCount`:

```cpp
struct LNDMaterial
{
	uint16_t MaterialType;
	uint16_t texture[256*256];
};
```

### Bumpmap

`uint8_t[65536]`

### Noisemap

`uint8_t[65536]`

## Notes

* The game defines a global height unit for the terrain at `0.67f` - this means at max altitude (255) the game's height coordinate is `170.85f`.