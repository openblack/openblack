/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMindFileBody.h"

#include <cmath>

#include <algorithm>

#include <MindFile.h>

#include "Creature/CreatureMorph.h"

using namespace openblack;
using namespace openblack::creature_mind_body;

std::optional<CreatureType> creature_mind_body::SpeciesFromRow(uint32_t row)
{
	if (row == 0)
	{
		return CreatureType::GiantApe;
	}
	if (row < creaturemind::k_SpeciesRows)
	{
		return static_cast<CreatureType>(row);
	}
	return std::nullopt;
}

std::optional<Body> creature_mind_body::FromMindFile(const creaturemind::MindFileData& file)
{
	const auto species = SpeciesFromRow(file.speciesRow);
	if (!species.has_value())
	{
		return std::nullopt;
	}
	Body body {.species = *species, .name = file.name};
	const auto finite = [](float value) { return std::isfinite(value); };
	if (file.alignment.has_value() && finite(*file.alignment))
	{
		body.alignment = std::clamp(*file.alignment, -1.0f, 1.0f);
	}
	if (finite(file.physique.strength))
	{
		body.strength = std::clamp(file.physique.strength, 0.0f, 1.0f);
	}
	if (file.physique.size.has_value() && finite(*file.physique.size) && *file.physique.size > 0.0f)
	{
		body.size = std::clamp(*file.physique.size, creature_morph::k_MinScale, creature_morph::k_MaxScale);
	}
	if (file.tattooHeader.has_value())
	{
		creature_tattoo::Slots slots {};
		std::ranges::transform(*file.tattooHeader, slots.begin(), creature_tattoo::FromWord);
		body.tattoos = slots;
	}
	return body;
}
