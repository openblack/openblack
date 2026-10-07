/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Materials.h"

#include <PhysicsConstantsFile.h>

using namespace openblack::physics;

namespace
{
/// At or below the minimum gives the minimum, at or above the maximum the maximum
float ClampValue(float value, float minimum, float maximum)
{
	if (value <= minimum)
	{
		return minimum;
	}
	if (value >= maximum)
	{
		return maximum;
	}
	return value;
}
} // namespace

Material MaterialTable::Clamp(const Material& material)
{
	return {
	    .density = ClampValue(material.density, k_Minimum.density, k_Maximum.density),
	    .springK = ClampValue(material.springK, k_Minimum.springK, k_Maximum.springK),
	    .dampK = ClampValue(material.dampK, k_Minimum.dampK, k_Maximum.dampK),
	    .friction = ClampValue(material.friction, k_Minimum.friction, k_Maximum.friction),
	    .spinKeptPerSecond = ClampValue(material.spinKeptPerSecond, k_Minimum.spinKeptPerSecond, k_Maximum.spinKeptPerSecond),
	    .drag = ClampValue(material.drag, k_Minimum.drag, k_Maximum.drag),
	};
}

MaterialTable::MaterialTable(const std::optional<physconst::PhysicsConstantsFile>& file)
{
	if (!file.has_value())
	{
		return;
	}
	const auto declared = file->declaredRows > 0 ? static_cast<std::size_t>(file->declaredRows) : 0;
	for (std::size_t i = 0; i < _rows.size(); ++i)
	{
		if (i < declared)
		{
			// A row the file declares but doesn't hold reads as zero before its clamp
			const auto row = i < file->rows.size() ? file->rows[i] : physconst::Row {};
			_rows[i] = Clamp({
			    .density = row.density,
			    .springK = row.springK,
			    .dampK = row.dampK,
			    .friction = row.friction,
			    .spinKeptPerSecond = row.spinKept,
			    .drag = row.drag,
			});
		}
		else
		{
			_rows[i] = _rows[0];
		}
	}
}
