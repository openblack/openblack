/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
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

namespace openblack::ecs::components
{

struct Hand
{
	enum class RenderType
	{
		Model,
		Symbol
	};

	static constexpr int meshId = 999;

	bool rightHanded;
	RenderType renderType;
};
} // namespace openblack::ecs::components
