/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

namespace openblack
{

enum class TempleRoom
{
	Challenge,
	CreatureCave,
	Credits,
	Main,
	Multi,
	Options,
	SaveGame,
	Unknown
};

class TempleInteriorInterface
{
public:
	[[nodiscard]] virtual bool Active() const = 0;
	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
	virtual void Activate() = 0;
	virtual void Activate(TempleRoom room) = 0;
	virtual void Deactivate() = 0;
};
} // namespace openblack
