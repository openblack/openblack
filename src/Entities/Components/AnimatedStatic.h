/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Enums.h"

#include <string>
#include <unordered_map>

namespace openblack
{

struct AnimatedStatic
{
	std::string type;
};

struct MobileStatic
{
	MobileStaticInfo type;
};

struct MobileObject
{
	MobileObjectInfo type;
};

struct Feature
{
	FeatureInfo type;
};

struct Field
{
	int town;
};

struct Forest
{
	int type;
};

struct Town
{
	int id;
	std::unordered_map<std::string, float> beliefs;
	bool uninhabitable = false;
};

} // namespace openblack
