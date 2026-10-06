/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <string_view>

#include "Windowing/WindowingInterface.h"

namespace openblack
{

enum class GraphicsBackend : uint8_t
{
	Noop,
	Direct3D12,
	Metal,
	Vulkan,
};

static const std::map<std::string_view, GraphicsBackend> k_GraphicsBackendStringLookup {
    std::pair {"Noop", GraphicsBackend::Noop},
    std::pair {"Direct3D12", GraphicsBackend::Direct3D12},
    std::pair {"Metal", GraphicsBackend::Metal},
    std::pair {"Vulkan", GraphicsBackend::Vulkan},
};

struct EngineConfig
{
	bool wireframe {false};
	/// The villagers' names over their heads, as the Show Villager Names key toggles
	bool showVillagerNames {false};
	/// What the villagers are doing, their age, health and hunger, as the Show Villager Details key toggles
	bool showVillagerDetails {false};
	bool debugVillagerNames {false};
	bool debugVillagerStates {false};
	/// Each miracle dispenser is labelled with its miracle
	bool showDispenserNames {true};

	bool viewDetailOverlay {false};
	bool drawSky {true};
	bool drawWater {true};
	bool drawIsland {true};
	bool drawEntities {true};
	bool drawVegetation {true};
	bool drawSprites {true};
	bool drawBoundingBoxes {false};
	bool drawFootpaths {false};
	bool drawStreams {false};
	/// The hand mesh is a left hand, which Black & White mirrors into a right hand unless the player profile asks for
	/// a left hand
	bool rightHandedHand {true};
	/// Creatures' vertices at the seams between their bones are blended towards their partners, as the game draws them
	bool blendCreatureSeams {true};
	/// Creatures cast their shadows onto the land and what stands on it
	bool drawCreatureShadows {true};

	bool vsync {false};
	bool running {false};

	float timeOfDay {12.0f};
	float smallBumpMapStrength {1.0f};

	float cameraXFov {70.0f};
	float cameraNearClip {1.0f};
	/// The game's graphics detail level, 0 to 6 (see graphics::detail_level)
	uint8_t detailLevel {4};
	float cameraFarClip {static_cast<float>(0x10000)};

	float guiScale {1.0f};

	GraphicsBackend graphicsBackend {GraphicsBackend::Noop};
	glm::u16vec2 resolution {256, 256};
	windowing::DisplayMode displayMode {windowing::DisplayMode::Windowed};

	uint32_t numFramesToSimulate {0};
	/// Log frame time statistics every so many frames, never when 0
	uint32_t frameStatsInterval {0};
	/// With the frame statistics, the GPU time of each render view
	bool frameStatsViews {false};
};
} // namespace openblack
