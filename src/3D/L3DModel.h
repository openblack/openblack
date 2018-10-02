#pragma once
#ifndef OPENBLACK_L3DMODEL_H
#define OPENBLACK_L3DMODEL_H

#include <Graphics/Mesh.h>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	class L3DModel
	{
	public:
		L3DModel();
		~L3DModel();

		void LoadFromL3D(void* data, size_t size);
		void Draw();

		unsigned int GetSubMeshCount();
	protected:
		Mesh** m_subMeshes;
		unsigned int m_subMeshCount;
	};
}

#endif
