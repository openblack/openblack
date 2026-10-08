/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SnowDust.h"

#include <algorithm>

#include <glm/vec2.hpp>

#include "3D/LandLightTable.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "Locator.h"
#include "Physics/TurnRules.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;

std::optional<uint32_t> snow_dust::CurrentLandColour()
{
	if (!Locator::resources::has_value() || !Locator::skySystem::has_value())
	{
		return std::nullopt;
	}
	const auto& palettes = Locator::resources::value().GetLandLightPalettes();
	if (!palettes.Contains(LandLightPalette::k_Id.value()))
	{
		return std::nullopt;
	}
	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();
	const auto alignment = Locator::alignmentSystem::has_value() ? Locator::alignmentSystem::value().GetSkyAlignment() : 0.0f;
	float overcast = 0.0f;
	if (Locator::weatherSystem::has_value() && Locator::camera::has_value())
	{
		overcast = Locator::weatherSystem::value().GetOvercast(Locator::camera::value().GetOrigin());
	}
	return LandLightTable::GetLandColour(*palettes.Handle(LandLightPalette::k_Id.value()), skyType, alignment, overcast);
}

int32_t snow_dust::SnowAt(glm::vec3 point)
{
	if (!Locator::snowSystem::has_value())
	{
		return 0;
	}
	// The depth is cut to a whole number before it is kept within a byte
	const auto depth = static_cast<int32_t>(Locator::snowSystem::value().GetDepth(glm::vec2(point.x, point.z)));
	return std::clamp(depth, 0, 255);
}

uint32_t snow_dust::Tint(uint32_t argb, int32_t snow)
{
	if (snow == 0)
	{
		return argb;
	}
	const auto land = CurrentLandColour();
	return land.has_value() ? physics::turn::BlendColour(argb, *land, snow) : argb;
}
