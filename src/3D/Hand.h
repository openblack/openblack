#pragma once
#ifndef OPENBLACK_3D_HAND_H
#define OPENBLACK_3D_HAND_H

#include <Graphics/Mesh.h>
#include <Graphics/Texture2D.h>

#include <3D/L3DModel.h>

#include <memory>

namespace OpenBlack
{
	class Hand
	{
	public:
		Hand();
		~Hand();

		void Init();
		void Draw();
	private:
		std::unique_ptr<L3DModel> m_HandModel;
	};
}

#endif
