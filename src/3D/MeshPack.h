#pragma once
#ifndef OPENBLACK_MESHPACK_H
#define OPENBLACK_MESHPACK_H

#include <Common/OSFile.h>
#include <3D/Mesh.h>

namespace OpenBlack
{
	class MeshPack
	{
	public:
		MeshPack(OSFile* allMeshes);

		Mesh** Meshes;
	};
}

#endif // OPENBLACK_MESHPACK_H
