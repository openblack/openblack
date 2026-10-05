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

#include "3D/FieldCrop.h"
#include "Enums.h"

namespace openblack::ecs::components
{

/// A town's field and the crop growing in it (see field_crop)
struct Field
{
	int town;
	FieldTypeInfo type;
	field_crop::Crop crop;
	/// Which of every ten turns the crop grows on
	uint32_t growthTurn;
	/// The crop's height as it is drawn, easing towards where its food puts it
	field_crop::Settle height;
};

} // namespace openblack::ecs::components
