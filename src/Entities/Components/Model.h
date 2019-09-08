#include <AllMeshes.h>
#include <cstdint>

struct Model
{
	openblack::MeshId meshId;
	int submeshId;
	float xOffset;
	float yOffset;
	float zOffset;
};
