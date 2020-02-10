/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/L3DMesh.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"

#include <memory>

namespace openblack
{
enum class HandRenderType
{
	Model,
	Symbol
};

class Hand
{
public:
	void Init();
	void Draw();

private:
	HandRenderType handType;
	std::unique_ptr<L3DMesh> m_HandModel;
};
} // namespace openblack
