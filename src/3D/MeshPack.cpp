#include "MeshPack.h"

#include <stdint.h>
#include <stdexcept>

#include <3D/Mesh.h>

using namespace OpenBlack;

// Just enough for us to parse
struct L3DSMiniHeader {
	char magic[4];
	uint32_t unknown;
	uint32_t l3dSize;
};

MeshPack::MeshPack(OSFile* file)
{
	LHSegment meshesSegment;
	file->GetSegment("MESHES", &meshesSegment, true);

	printf("Meshes Segment %s of %d bytes\n", meshesSegment.Name, meshesSegment.SegmentSize);

	uint8_t* data = (uint8_t*)meshesSegment.SegmentData;
	uint32_t* meshCount = (uint32_t*)(data + 4);
	uint32_t* meshOffsets = (uint32_t*)(data + 8);

	printf("%d meshes\n", *meshCount);

	Mesh* meshes = new Mesh[*meshCount];

	Meshes = new Mesh*[*meshCount];

	for (int i = 0; i < *meshCount; i++)
	{
		L3DSMiniHeader* header = (L3DSMiniHeader*)(data + meshOffsets[i]);

		Mesh* mesh = new Mesh();
		mesh->LoadFromL3D(data + meshOffsets[i], header->l3dSize);

		Meshes[i] = mesh;
	}
}
