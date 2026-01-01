/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <concepts>

#include <glm/detail/qualifier.hpp>

namespace openblack
{

// Called Zoomer in vanilla, this is a 5 degree (quintic) polynomial.
// It has degrees of freedom in start and end position, velocity.
// The acceleration, jerk and snap are based off the first 5 taylor series
// expansion elements of the exp function.
// It takes as input t as an interpolation factor between 0 and 1
template <typename T>
    requires std::floating_point<T>
struct ZoomInterpolator
{
	T p0;
	T p1;
	T v0;
	T v1;

	explicit ZoomInterpolator(T p);
	ZoomInterpolator(T p0, T p1, T v0, T v1);

	[[nodiscard]] T PositionAt(T t) const;
	[[nodiscard]] T VelocityAt(T t) const;
	[[nodiscard]] T AccelerationAt(T t) const;
};

template <glm::length_t L, typename T, glm::qualifier Q = glm::defaultp>
    requires std::floating_point<T>
struct ZoomInterpolatorVec
{
	glm::vec<L, T, Q> p0;
	glm::vec<L, T, Q> p1;
	glm::vec<L, T, Q> v0;
	glm::vec<L, T, Q> v1;

	explicit ZoomInterpolatorVec(glm::vec<L, T, Q> p);
	ZoomInterpolatorVec(glm::vec<L, T, Q> p0, glm::vec<L, T, Q> p1, glm::vec<L, T, Q> v0, glm::vec<L, T, Q> v1);

	[[nodiscard]] glm::vec<L, T, Q> PositionAt(T t) const;
	[[nodiscard]] glm::vec<L, T, Q> VelocityAt(T t) const;
	[[nodiscard]] glm::vec<L, T, Q> AccelerationAt(T t) const;
};

using ZoomInterpolator3f = ZoomInterpolatorVec<3, float, glm::defaultp>;

} // namespace openblack
