/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandData.h"

#include <algorithm>

using namespace openblack;

LandData LandData::FromLnd(const lnd::LNDFile& lnd)
{
	LandData data;
	std::ranges::copy(lnd.GetHeader().lookUpTable, data.blockIndexLookup.begin());
	data.blocks = lnd.GetBlocks();
	data.countries = lnd.GetCountries();
	data.materials = lnd.GetMaterials();
	const auto& extra = lnd.GetExtra();
	data.noise.assign(extra.noise.texels.begin(), extra.noise.texels.end());
	data.bump.assign(extra.bump.texels.begin(), extra.bump.texels.end());
	return data;
}
