/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>

#include <glm/detail/qualifier.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

namespace glm
{
template <length_t L, typename T, qualifier Q>
void from_json(const nlohmann::json& j, vec<L, T, Q>& v)
{
	if constexpr (L >= 1)
	{
		j.at("x").get_to(v.x);
	}
	if constexpr (L >= 2)
	{
		j.at("y").get_to(v.y);
	}
	if constexpr (L >= 3)
	{
		j.at("z").get_to(v.z);
	}
	if constexpr (L >= 4)
	{
		j.at("z").get_to(v.w);
	}
}

template <length_t L, typename T, qualifier Q>
std::ostream& operator<<(std::ostream& os, const vec<L, T, Q>& v)
{
	return os << glm::to_string(v);
}
} // namespace glm

namespace std::chrono
{
template <typename _Rep, typename _Period>
void from_json(const nlohmann::json& j, duration<_Rep, _Period>& d)
{
	int asInt;
	j.get_to(asInt);
	d = decltype(d)(asInt);
}
} // namespace std::chrono
