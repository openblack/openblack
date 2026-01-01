/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <bgfx/bgfx.h>

#include "Windowing/WindowingInterface.h"

namespace openblack
{

struct EngineConfig
{
	bool wireframe {false};
	bool showVillagerNames {false};
	bool debugVillagerNames {false};
	bool debugVillagerStates {false};

	bool viewDetailOverlay {false};
	bool drawSky {true};
	bool drawWater {true};
	bool drawIsland {true};
	bool drawEntities {true};
	bool drawVegetation {true};
	bool drawSprites {true};
	bool drawTestModel {true};
	bool drawDebugCross {true};
	bool drawBoundingBoxes {false};
	bool drawFootpaths {false};
	bool drawStreams {false};

	bool vsync {false};
	bool running {false};

	float timeOfDay {12.0f};
	float skyAlignment {0.0f};
	float bumpMapStrength {1.0f};
	float smallBumpMapStrength {1.0f};

	float cameraXFov {70.0f};
	float cameraNearClip {1.0f};
	float cameraFarClip {static_cast<float>(0x10000)};

	float guiScale {1.0f};

	bgfx::RendererType::Enum rendererType {bgfx::RendererType::Noop};
	glm::u16vec2 resolution {256, 256};
	windowing::DisplayMode displayMode {windowing::DisplayMode::Windowed};

	uint32_t numFramesToSimulate {0};
};
} // namespace openblack
