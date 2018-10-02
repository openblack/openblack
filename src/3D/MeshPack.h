#pragma once
#ifndef OPENBLACK_MESHPACK_H
#define OPENBLACK_MESHPACK_H

#include <Common/OSFile.h>
#include <3D/L3DModel.h>
#include <Graphics/Texture2DArray.h>

namespace OpenBlack
{
	class MeshPack
	{
	public:
		MeshPack(OSFile* allMeshes);

		L3DModel** Models;
		GLuint* Textures;

		int GetMeshCount();
	private:
		int m_meshCount;
	};
}

#endif // OPENBLACK_MESHPACK_H
