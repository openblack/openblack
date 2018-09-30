#pragma once
#ifndef OPENBLACK_MESHPACK_H
#define OPENBLACK_MESHPACK_H

#include <Common/OSFile.h>
#include <3D/Mesh.h>
#include <Graphics/Texture2DArray.h>

namespace OpenBlack
{
	class MeshPack
	{
	public:
		MeshPack(OSFile* allMeshes);

		Mesh** Meshes;
		Texture2DArray* TextureArray;
	};
}

#endif // OPENBLACK_MESHPACK_H
