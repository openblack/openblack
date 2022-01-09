/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ScriptingBindingUtils.h"

#include "3D/LandIsland.h"
#include "Game.h"

namespace openblack::lhscriptx
{

glm::vec2 GetHorizontalPosition(const std::string& str)
{
	const auto pos = str.find_first_of(',');
	const auto y = std::stof(str.substr(pos + 1));
	const auto x = std::stof(str.substr(0, pos));
	return glm::vec2(x, y);
}

template <>
glm::vec3 GetParamValueRegular(const ScriptCommandContext& ctx, int index)
{
	const auto& param = ctx[index];
	const auto& island = ctx.GetGame().GetLandIsland();
	const auto pos = GetHorizontalPosition(param.GetString());
	return {pos.x, island.GetHeightAt(pos), pos.y};
}

} // namespace openblack::lhscriptx
