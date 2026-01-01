/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ZoomInterpolator.h"

#include <glm/vec3.hpp>

using namespace openblack;

template <>
ZoomInterpolator<float>::ZoomInterpolator(float p)
    : p0(p)
    , p1(p)
    , v0(0.0f)
    , v1(0.0f)
{
}

template <>
ZoomInterpolator<float>::ZoomInterpolator(float p0, float p1, float v0, float v1)
    : p0(p0)
    , p1(p1)
    , v0(v0)
    , v1(v1)
{
}

template <>
ZoomInterpolator<double>::ZoomInterpolator(double p)
    : p0(p)
    , p1(p)
    , v0(0.0)
    , v1(0.0)
{
}

template <>
ZoomInterpolator<double>::ZoomInterpolator(double p0, double p1, double v0, double v1)
    : p0(p0)
    , p1(p1)
    , v0(v0)
    , v1(v1)
{
}

template <>
ZoomInterpolator3f::ZoomInterpolatorVec(glm::vec3 p)
    : p0(p)
    , p1(p)
    , v0(0.0f)
    , v1(0.0f)
{
}

template <>
ZoomInterpolator3f::ZoomInterpolatorVec(glm::vec3 p0, glm::vec3 p1, glm::vec3 v0, glm::vec3 v1)
    : p0(p0)
    , p1(p1)
    , v0(v0)
    , v1(v1)
{
}

template <typename T>
    requires std::floating_point<T>
inline T PositionAtImpl(T p0, T p1, T v0, T v1, T t)
{
	// Catch singularity
	if (p0 == p1)
	{
		return p0;
	}

	const auto pr = p1 - p0;

	// Turn velocities into slopes. Can divide by pr since t goes from 0 to 1
	const auto m0 = v0 / pr;
	const auto m1 = v1 / pr;

	// Compute point on scaled down curve
	const auto t2 = t * t;
	const auto t3 = t2 * t;
	const auto t4 = t2 * t2;
	const auto result = //
	    t * m0 +        //
	    t2 * (static_cast<T>(-3) * m0 - static_cast<T>(3) * m1 + static_cast<T>(6)) +
	    t3 * (static_cast<T>(3) * m0 + static_cast<T>(5) * m1 - static_cast<T>(8)) +
	    t4 * (-m0 - static_cast<T>(2) * m1 + static_cast<T>(3));

	// Scale and translate point on curve
	return result * pr + p0;
}

template <typename T>
    requires std::floating_point<T>
inline T VelocityAtImpl(T p0, T p1, T v0, T v1, T t)
{
	if (p0 == p1)
	{
		return 0;
	}

	const auto pr = p1 - p0;

	// Turn velocities into slopes. Can divide by pr since t goes from 0 to 1
	const auto m0 = v0 / pr;
	const auto m1 = v1 / pr;

	// Derivative of position function with respect to t
	const auto t2 = t * t;
	const auto t3 = t2 * t;
	const auto result = //
	    m0 +            //
	    t * 2 * (static_cast<T>(-3) * m0 - static_cast<T>(3) * m1 + static_cast<T>(6)) +
	    t2 * 3 * (static_cast<T>(3) * m0 + static_cast<T>(5) * m1 - static_cast<T>(8)) +
	    t3 * 4 * (-m0 - static_cast<T>(2) * m1 + static_cast<T>(3));

	// Scale back to the original problem's units (since m0 and m1 were scaled down by dp)
	return result * pr;
}

template <typename T>
    requires std::floating_point<T>
inline T AccelerationAtImpl(T p0, T p1, T v0, T v1, T t)
{
	if (p0 == p1)
	{
		return 0;
	}

	const auto pr = p1 - p0;

	// Turn velocities into slopes. Can divide by pr since t goes from 0 to 1
	const auto m0 = v0 / pr;
	const auto m1 = v1 / pr;

	// Second derivative of position function with respect to t to find acceleration
	const auto t2 = t * t;
	const auto result = //
	    2 * (static_cast<T>(-3) * m0 - static_cast<T>(3) * m1 + static_cast<T>(6)) +
	    t * 6 * (static_cast<T>(3) * m0 + static_cast<T>(5) * m1 - static_cast<T>(8)) +
	    t2 * 12 * (-m0 - static_cast<T>(2) * m1 + static_cast<T>(3));

	// Scale back to the original problem's units (since m0 and m1 were scaled down by pr)
	return result * pr;
}

template <>
float ZoomInterpolator<float>::PositionAt(float t) const
{
	return PositionAtImpl(p0, p1, v0, v1, t);
}

template <>
double ZoomInterpolator<double>::PositionAt(double t) const
{
	return PositionAtImpl(p0, p1, v0, v1, t);
}

template <>
float ZoomInterpolator<float>::VelocityAt(float t) const
{
	return VelocityAtImpl(p0, p1, v0, v1, t);
}

template <>
double ZoomInterpolator<double>::VelocityAt(double t) const
{
	return VelocityAtImpl(p0, p1, v0, v1, t);
}

template <>
float ZoomInterpolator<float>::AccelerationAt(float t) const
{
	return AccelerationAtImpl(p0, p1, v0, v1, t);
}

template <>
double ZoomInterpolator<double>::AccelerationAt(double t) const
{
	return AccelerationAtImpl(p0, p1, v0, v1, t);
}

template <>
glm::vec3 ZoomInterpolator3f::PositionAt(float t) const
{
	return {
	    PositionAtImpl(p0.x, p1.x, v0.x, v1.x, t),
	    PositionAtImpl(p0.y, p1.y, v0.y, v1.y, t),
	    PositionAtImpl(p0.z, p1.z, v0.z, v1.z, t),
	};
}

template <>
glm::vec3 ZoomInterpolator3f::VelocityAt(float t) const
{
	return {
	    VelocityAtImpl(p0.x, p1.x, v0.x, v1.x, t),
	    VelocityAtImpl(p0.y, p1.y, v0.y, v1.y, t),
	    VelocityAtImpl(p0.z, p1.z, v0.z, v1.z, t),
	};
}

template <>
glm::vec3 ZoomInterpolator3f::AccelerationAt(float t) const
{
	return {
	    AccelerationAtImpl(p0.x, p1.x, v0.x, v1.x, t),
	    AccelerationAtImpl(p0.y, p1.y, v0.y, v1.y, t),
	    AccelerationAtImpl(p0.z, p1.z, v0.z, v1.z, t),
	};
}
