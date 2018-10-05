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

		void GUI();
		void Render();
	private:
		int m_currentMesh;

		L3DModel* GetCurrentModel();
	};
}

#endif
