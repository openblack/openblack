/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureRig.h"

using namespace openblack;
using namespace openblack::creature;

const skeletal_animation::Animation* CreatureRig::GetAnimation(Mesh mesh, size_t index) const
{
	const auto slot = static_cast<size_t>(mesh);
	if (slot < animations.size() && index < animations.at(slot).size() && animations.at(slot)[index].has_value())
	{
		return &*animations.at(slot)[index];
	}
	const auto& base = animations.front();
	return index < base.size() && base[index].has_value() ? &*base[index] : nullptr;
}
