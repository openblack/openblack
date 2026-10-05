/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "FieldSystem.h"

#include "ECS/Components/Field.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{

field_crop::Type TypeOf(FieldTypeInfo type)
{
	const auto& info = Locator::infoConstants::value().fieldType.at(static_cast<size_t>(type));
	return {
	    .ageGrowth = info.ageGrowth,
	    .ageRipe = info.ageRecolt,
	    .timesToSow = info.timesToSow,
	    .totalFood = info.totalFoodInField,
	    .sunWhenGrowing = info.effectSunWhenGrowing,
	    .sunWhenRipening = info.effectSunWhenRipening,
	    .rainWhenGrowing = info.effectRainWhenGrowing,
	    .rainWhenRipening = info.effectRainWhenRipening,
	};
}

/// The land's alignment at a place: each player's influence there times their alignment
float LandAlignment(const glm::vec3& position)
{
	if (!Locator::influenceSystem::has_value() || !Locator::alignmentSystem::has_value())
	{
		return 0.0f;
	}
	const auto& influence = Locator::influenceSystem::value();
	const auto& alignment = Locator::alignmentSystem::value();
	float sum = 0.0f;
	for (size_t i = 0; i < static_cast<size_t>(PlayerNames::_COUNT); ++i)
	{
		const auto player = static_cast<PlayerNames>(i);
		sum += alignment.GetPlayerAlignment(player) * influence.PlayerInfluence(player, position);
	}
	return field_crop::ClampAlignment(sum);
}

bool IsRaining(const glm::vec3& position)
{
	return Locator::weatherSystem::has_value() && Locator::weatherSystem::value().GetWeather(position).rain > 0;
}

} // namespace

void FieldSystem::ProcessTurn(uint32_t turn)
{
	Locator::entitiesRegistry::value().Each<Field, const Transform>([turn](Field& field, const Transform& transform) {
		if ((turn + field.growthTurn) % field_crop::k_TurnsPerGrowth != 0)
		{
			return;
		}
		field_crop::Grow(field.crop, TypeOf(field.type), LandAlignment(transform.position), IsRaining(transform.position));
	});
}

void FieldSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	const float seconds = std::chrono::duration<float>(gameTime).count();
	Locator::entitiesRegistry::value().Each<Field>([this, seconds](Field& field) {
		if (const auto look = GetLook(field); look.has_value())
		{
			field.height = field_crop::Advance(field.height, look->height, seconds);
		}
	});
}

std::optional<field_crop::Look> FieldSystem::GetLook(const Field& field) const
{
	const auto leastFood =
	    Locator::infoConstants::value().pot.at(static_cast<size_t>(PotInfo::HandFood)).amountPickedUpInitially;
	return field_crop::LookOf(field.crop, TypeOf(field.type), leastFood);
}
