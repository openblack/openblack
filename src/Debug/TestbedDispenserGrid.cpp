/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TestbedDispenserGrid.h"

#include <glm/common.hpp>

#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"

using namespace openblack;

std::span<const MagicType> testbed_dispensers::GridMagicTypes()
{
	return magic::DispensableMiracles();
}

std::vector<glm::vec2> testbed_dispensers::GridOffsets(size_t count)
{
	std::vector<glm::vec2> offsets;
	offsets.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		const auto column = static_cast<float>(i % k_GridColumns);
		const auto row = static_cast<float>(i / k_GridColumns);
		offsets.emplace_back(k_GridOrigin + glm::vec2(column, row) * k_GridSpacing);
	}
	return offsets;
}

void testbed_dispensers::PlaceGrid(glm::vec2 middle)
{
	if (!Locator::magicSystem::has_value())
	{
		return;
	}
	auto& magic = Locator::magicSystem::value();
	auto& registry = Locator::entitiesRegistry::value();
	const auto types = GridMagicTypes();
	const auto offsets = GridOffsets(types.size());
	for (size_t i = 0; i < types.size(); ++i)
	{
		// The map's y is the world's z
		const auto point = middle + offsets[i];
		const auto dispenser = magic.CreateDispenser({point.x, 0.0f, point.y}, types[i], k_GridYawRadians);
		if (dispenser != entt::null)
		{
			registry.Assign<ecs::components::TestbedDispenser>(dispenser);
			// The testbed's grid starts with every globe there to take
			magic.ChargeDispenser(dispenser);
		}
	}
}

void testbed_dispensers::RemoveGrid()
{
	if (!Locator::magicSystem::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> gone;
	registry.Each<const ecs::components::TestbedDispenser, const ecs::components::SpellDispenser>(
	    [&](entt::entity entity, const ecs::components::SpellDispenser& /*dispenser*/) { gone.push_back(entity); });
	// Each with its bubble and swirl
	for (const auto entity : gone)
	{
		Locator::magicSystem::value().Remove(entity);
	}
}

bool testbed_dispensers::InGridArea(glm::vec2 offset)
{
	const auto offsets = GridOffsets(GridMagicTypes().size());
	glm::vec2 low = offsets.front();
	glm::vec2 high = offsets.front();
	for (const auto& point : offsets)
	{
		low = glm::min(low, point);
		high = glm::max(high, point);
	}
	low -= glm::vec2(k_GridClearance);
	high += glm::vec2(k_GridClearance);
	return offset.x >= low.x && offset.x <= high.x && offset.y >= low.y && offset.y <= high.y;
}
