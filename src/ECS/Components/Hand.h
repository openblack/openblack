/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include <entt/entt.hpp>

#include "3D/L3DMesh.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"

namespace openblack::ecs::components
{

struct Hand
{
	enum class RenderType
	{
		Model,
		Symbol
	};

	static constexpr entt::id_type k_MeshId = entt::hashed_string("hand");

	bool rightHanded;
	RenderType renderType;
};
} // namespace openblack::ecs::components
