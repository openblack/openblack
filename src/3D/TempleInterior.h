/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include "3D/TempleInteriorInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{

class TempleInterior final: public TempleInteriorInterface
{
public:
	[[nodiscard]] bool Active() const override { return _active; }
	[[nodiscard]] glm::vec3 GetPosition() const override { return _templePosition; }
	void Activate() override;
	void Deactivate() override;
	void ChangeRoom(TempleRoom nextRoom) override;
	[[nodiscard]] TempleRoom GetCurrentRoom() const override;

private:
	bool _active;
	glm::vec3 _templePosition;
	glm::vec3 _templeRotation;
	glm::vec3 _playerPositionOutside;
	glm::vec3 _playerRotationOutside;
	TempleRoom _currentRoom;
};
} // namespace openblack
