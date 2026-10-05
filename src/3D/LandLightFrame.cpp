/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandLightFrame.h"

#include "3D/LandLightTable.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

namespace openblack
{

LandLightInputs FrameLandLightInputs()
{
	LandLightInputs inputs;
	if (Locator::skySystem::has_value())
	{
		inputs.skyType = Locator::skySystem::value().GetCurrentSkyType();
	}
	if (Locator::alignmentSystem::has_value())
	{
		inputs.alignment = Locator::alignmentSystem::value().GetSkyAlignment();
	}
	if (Locator::weatherSystem::has_value() && Locator::camera::has_value())
	{
		const auto camera = Locator::camera::value().GetOrigin();
		inputs.overcast = Locator::weatherSystem::value().GetOvercast(camera);
		inputs.flash = Locator::weatherSystem::value().GetLightningFlash(camera);
	}
	return inputs;
}

uint32_t FrameLandLight(uint8_t level)
{
	const auto& palettes = Locator::resources::value().GetLandLightPalettes();
	if (!palettes.Contains(LandLightPalette::k_Id.value()))
	{
		return 0xFFFFFFu;
	}
	const auto inputs = FrameLandLightInputs();
	LandLightTable table;
	table.Build(*palettes.Handle(LandLightPalette::k_Id.value()), inputs.skyType, inputs.alignment, inputs.overcast,
	            inputs.flash);
	// The texels are red, green, blue and alpha bytes
	const auto texel = table.GetTexels().at(level);
	return ((texel & 0xFFu) << 16u) | (texel & 0xFF00u) | ((texel >> 16u) & 0xFFu);
}

} // namespace openblack
