#pragma once
#ifndef OPENBLACK_MESHVIEWER_H
#define OPENBLACK_MESHVIEWER_H

#include <3D/MeshPack.h>

namespace OpenBlack
{
	class MeshViewer {
	public:
		MeshViewer();
		~MeshViewer();

		void LoadPack(std::string filePath);

		void GUI();
		void Render();
	private:
		MeshPack* m_meshPack;
		int m_currentMesh;

		L3DModel* GetCurrentModel();
	};
}

#endif
