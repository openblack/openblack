/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <chrono>
#include <optional>

#include "3D/FieldCrop.h"

namespace openblack::ecs::components
{
struct Field;
}

namespace openblack::ecs::systems
{

/// The crops growing in the towns' fields (components::Field)
class FieldSystemInterface
{
public:
	virtual ~FieldSystemInterface() = default;

	/// Once a game turn: each field's crop grows on its own turn in every ten, by the land's alignment and the weather
	virtual void ProcessTurn(uint32_t turn) = 0;
	/// Once a frame: the crops that show ease towards the height their food puts them at
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// How a field's crop looks, or nothing while it is too small to show
	[[nodiscard]] virtual std::optional<field_crop::Look> GetLook(const components::Field& field) const = 0;
};

} // namespace openblack::ecs::systems
