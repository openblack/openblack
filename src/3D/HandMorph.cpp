/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandMorph.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include "Creature/CreatureSkin.h"

using namespace openblack;

float hand_morph::Target(float playerAlignment)
{
	// Anything not above -1 is -1, and anything not below 1 is 1
	if (!(playerAlignment > -1.0f))
	{
		return -1.0f;
	}
	return playerAlignment < 1.0f ? playerAlignment : 1.0f;
}

std::optional<float> hand_morph::Refresh(float drawn, float target)
{
	if (!(std::abs(target - drawn) >= k_RefreshThreshold))
	{
		return std::nullopt;
	}
	return target;
}

hand_morph::Change hand_morph::Advance(State& state, float playerAlignment, std::optional<bool> inInfluence)
{
	Change change;
	if (inInfluence.has_value() && *inInfluence != state.inInfluence)
	{
		state.inInfluence = *inInfluence;
		change.skin = state.target;
	}
	state.target = Target(playerAlignment);
	if (const auto drawn = Refresh(state.drawn, state.target))
	{
		state.drawn = *drawn;
		change.skin = *drawn;
		change.shape = true;
	}
	return change;
}

map_coords::MapCoords hand_morph::NextPoint(const map_coords::MapCoords& last, std::optional<map_coords::MapCoords> picked,
                                            bool held)
{
	if (picked.has_value())
	{
		return *picked;
	}
	return held ? last : map_coords::MapCoords {};
}

hand_morph::Look hand_morph::LookOf(float drawn)
{
	return drawn < 0.0f ? Look::Evil : Look::Good;
}

float hand_morph::Weight(float drawn)
{
	return std::abs(drawn);
}

void hand_morph::BlendSkin(std::span<const uint16_t> base, std::span<const uint16_t> look, float drawn,
                           std::span<uint16_t> blended)
{
	assert(base.size() == look.size() && look.size() == blended.size());
	// The same steps as a creature's skin: the weight truncated from 256 steps and capped at 255
	const auto weight = creature_skin::BlendWeight(drawn);
	std::ranges::transform(base, look, blended.begin(),
	                       [weight](uint16_t from, uint16_t to) { return creature_skin::BlendTexel(from, to, weight); });
}
