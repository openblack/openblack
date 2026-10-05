/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The temples' outsides (Citadel::Process): each turn every temple's look moves toward its player's alignment and share
/// of influence, and its mesh and texture are blended afresh for them as they change (fn_00882B10)
class TempleExteriorSystemInterface
{
public:
	virtual ~TempleExteriorSystemInterface() = default;

	virtual void UpdateTurn() = 0;
	/// The player whose temple's entrance a ray first meets, if it meets one (CitadelEntrance, picked under the cursor)
	[[nodiscard]] virtual std::optional<PlayerNames> EntranceAt(glm::vec3 origin, glm::vec3 direction) const = 0;
};

} // namespace openblack::ecs::systems
