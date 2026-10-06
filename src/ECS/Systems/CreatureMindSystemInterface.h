/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <entt/entity/fwd.hpp>

namespace openblack::ecs::systems
{

/// The creatures' minds: once a game turn their desires grow and fade, and each decides what its body does while it
/// has nothing better to do, and what it looks at (see components::CreatureMindState). Creatures can also be told to
/// do things, which the debug tools use.
class CreatureMindSystemInterface
{
public:
	virtual ~CreatureMindSystemInterface() = default;

	virtual void ProcessTurn() = 0;

	/// Plays an action once, unless the creature's body already plays one
	virtual bool PlayAction(entt::entity creature, size_t animation) = 0;
	/// Plays a gesture on top of the body, unless one already plays
	virtual bool PlayGesture(entt::entity creature, size_t animation) = 0;
	/// Pulls a face for a few seconds
	virtual void PullFace(entt::entity creature, size_t animation) = 0;
	/// Sits down for a while, unless the body plays an action
	virtual bool SitDown(entt::entity creature) = 0;
	/// Gets up from sitting
	virtual void StandUp(entt::entity creature) = 0;
	/// As if the player stroked or slapped the creature: it shows its pleasure or sorrow next
	virtual void Feedback(entt::entity creature, bool stroke) = 0;
};

} // namespace openblack::ecs::systems
