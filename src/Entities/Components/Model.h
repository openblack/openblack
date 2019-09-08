#include <AllMeshes.h>
#include <cstdint>

struct Model
{
	openblack::MeshId meshId;
	std::vector<int> submeshIds;
	float xOffset;
	float yOffset;
	float zOffset;
};
