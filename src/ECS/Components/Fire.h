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

#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

#include "Enums.h"
#include "Fire/FireGraphic.h"
#include "Fire/FireModel.h"

namespace openblack::ecs::components
{

/// Something hotter than the air: its temperature, how charred it is, who set it alight and the blaze it is part of. The
/// fire system gives it to anything that heats up and takes it away once it has cooled and nothing is charred.
struct Fire
{
	fire::State state;
	/// What heated it, which it never heats back while that is still there
	entt::entity source {entt::null};
	/// The player whose doing it is, if anybody's
	PlayerNames player {PlayerNames::NEUTRAL};
	bool hasPlayer {false};
	/// The fire first in its blaze, which keeps the blaze's list (FireGroup); itself when it is first
	entt::entity root {entt::null};
	/// The turn it was made on, and whether it was made after that turn's burn point, when it waits for the next turn
	/// before it burns
	uint32_t createdTurn {0};
	bool waits {false};
	/// The reaction the living have to it while it is hot, 0 for none
	uint32_t reaction {0};
	/// The temperature the steam last started at, so that it starts again only when hotter
	float lastSteam {0.0f};
};

/// The first fire of a blaze keeps the blaze: its fires, itself first, and the villagers fighting it
struct FireGroup
{
	std::vector<entt::entity> members;
	std::vector<entt::entity> firemen;
};

/// How a burning object with a model looks: its flames, steam and smoke
struct FireLook
{
	fire::graphic::Graphic graphic;
	/// Whether its fire lights the land round it: a large building's does
	bool lightsLand {false};
};

/// How a script has an object take fire: it may be kept from ever catching, or catch without being hurt
struct FireProofing
{
	bool cannotBeSetOnFire {false};
	bool notHurtByFire {false};
};

/// A villager about a fire: the object whose fire it beats or runs from, the reaction to a fire it has taken up and the
/// object behind it, and where it was walking before the fire turned it aside
struct VillagerFireState
{
	entt::entity fire {entt::null};
	entt::entity reactionTarget {entt::null};
	/// The reaction it reacts to, 0 for none
	uint32_t reaction {0};
	glm::vec2 walkTarget {0.0f};
};

/// The life of an object that isn't living, 0 to 1: a tree, a building, a rock. Without it an object has all its life.
struct ObjectLife
{
	float life {1.0f};
};

} // namespace openblack::ecs::components
