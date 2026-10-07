/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

/// What a drop of the water miracle's rain does to the crops and the trees it falls by
namespace openblack::magic
{

/// A field's crop as the water sees it
struct WaterCrop
{
	uint8_t timesSown {0};
	float age {0.0f};
	float food {0.0f};
};

/// The field's kind
struct WaterCropType
{
	float timesToSow {30.0f};
	/// The age it is ripe for picking at, the food it then holds, and how much each drop ages it
	float ageRipe {1200.0f};
	float totalFood {350.0f};
	float effectOfWater {2.0f};
};

/// A drop on a field not on fire: one not fully sown is sown at once; one sown and not yet ripe ages and fills with food
void WaterField(WaterCrop& crop, const WaterCropType& type);

/// A tree as the water sees it: its size now and the size it grows to
struct WaterTreeType
{
	float growthAmount {0.01f};
	float waterAccelerator {1.0f};
};
struct WaterTreeResult
{
	float scale;
	float target;
	/// It was still growing before the drop
	bool canGrow;
};
/// A drop by a tree: one still growing grows by its kind's growth; the extreme water grows any tree, past its full size
/// ever more slowly up to about three times, and raises how big it grows
[[nodiscard]] WaterTreeResult WaterTree(float scale, float target, const WaterTreeType& type, bool extreme);

} // namespace openblack::magic
