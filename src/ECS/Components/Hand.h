/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>

namespace openblack::ecs::components
{

struct Hand
{
	enum class RenderType : uint8_t
	{
		Model,
		Symbol
	};

	static constexpr entt::id_type k_MeshId = entt::hashed_string("hand");

	bool rightHanded;
	RenderType renderType;
};
} // namespace openblack::ecs::components
