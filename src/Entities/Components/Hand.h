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
#include "Common/MeshLookup.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"

#include <memory>

namespace openblack::entities::components
{

struct Hand
{
	enum class RenderType
	{
		Model,
		Symbol
	};

	static constexpr MeshId meshId = 999;

	void Init();

	RenderType renderType;
};
} // namespace openblack::entities::components
