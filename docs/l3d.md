## Black & White L3D Model

```cpp
struct L3DHeader {
    char magicHeader[4] = 'L3D0';
    uint32_t modelSize;
    uint32_t skinOffset;
    uint32_t numMeshes;
    uint32_t meshListOffset; // L3D_Mesh
    uint32_t unknown_1;
    uint32_t unknown_2;
    uint32_t unknown_3;
    uint32_t unknown_4;
    uint32_t unknown_5;
    uint32_t unknown_6;
    uint32_t unknown_7;
    uint32_t unknown_8;
    uint32_t unknown_9;
    uint32_t numSkins;
    uint32_t skinListOffset;
    uint32_t unknown_10;
    uint32_t unknown_11;
    uint32_t pSkinName;
};

struct L3D_Mesh {
    uint8_t unknown_1;
    uint8_t unknown_2; // 20: nodraw
    uint8_t unknown_3;
    uint8_t unknown_4; // e0: transparent

    uint32_t numSubMeshes;
    uint32_t subMeshOffset; // L3D_SubMesh
    uint32_t numBones;
    uint32_t bonesOffset;
};

struct L3D_SubMesh {
    uint32_t unknown_1;
    uint32_t unknown_2;
    uint32_t skinID;
    uint32_t unknown_3;

    uint32_t numVerticies;
    uint32_t verticiesOffset;
    uint32_t numTriangles;
    uint32_t trianglesOffset;
    uint32_t boneVertLUTSize;
    uint32_t boneVertLUTOffset;
    uint32_t numVertexBlends;
    uint32_t vertexBlendsOffset;
};

struct L3D_VertexBlend {
    uint16_t index1;
    uint16_t index2;
    float weight;
};

struct L3D_Vertex {
    Vec3 position;
    Vec2 texCoords;
    Vec3 normal;
};

struct L3D_Triangle {
    uint16_t indices[3];
}
```
