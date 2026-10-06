/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "KeyPointSpline.h"

using openblack::particles::KeyPointSpline;

namespace
{
/// A slope this large stands for "no slope given": the end of the curve is left free
constexpr float k_FreeSlope = 1e30f;
constexpr float k_FreeSlopeThreshold = 0.99e30f;
constexpr float k_Sixth = 1.0f / 6.0f;
} // namespace

KeyPointSpline::KeyPointSpline(std::span<const float> pairs, bool zeroEndSlopes)
{
	const size_t n = pairs.size() / 2;
	_keys.reserve(n);
	for (size_t i = 0; i < n; ++i)
	{
		_keys.push_back({pairs[2 * i], pairs[(2 * i) + 1], 0.0f});
	}
	if (n < 2)
	{
		return;
	}
	auto& k = _keys;
	const float firstSlope = zeroEndSlopes ? 0.0f : k_FreeSlope;
	const float lastSlope = firstSlope;
	std::vector<float> u(n - 1, 0.0f);
	if (firstSlope > k_FreeSlopeThreshold)
	{
		k[0].curvature = 0.0f;
	}
	else
	{
		k[0].curvature = -0.5f;
		const float h = k[1].t - k[0].t;
		u[0] = 3.0f / h * ((k[1].value - k[0].value) / h - firstSlope);
	}
	// The tridiagonal system's decomposition
	for (size_t i = 1; i + 1 < n; ++i)
	{
		const float sig = (k[i].t - k[i - 1].t) / (k[i + 1].t - k[i - 1].t);
		const float p = sig * k[i - 1].curvature + 2.0f;
		k[i].curvature = (sig - 1.0f) / p;
		u[i] = (k[i + 1].value - k[i].value) / (k[i + 1].t - k[i].t) - (k[i].value - k[i - 1].value) / (k[i].t - k[i - 1].t);
		u[i] = (6.0f * u[i] / (k[i + 1].t - k[i - 1].t) - sig * u[i - 1]) / p;
	}
	float qn = 0.0f;
	float un = 0.0f;
	if (lastSlope <= k_FreeSlopeThreshold)
	{
		qn = 0.5f;
		const float h = k[n - 1].t - k[n - 2].t;
		un = 3.0f / h * (lastSlope - (k[n - 1].value - k[n - 2].value) / h);
	}
	k[n - 1].curvature = (un - qn * u[n - 2]) / (qn * k[n - 2].curvature + 1.0f);
	// Back substitution
	for (size_t i = n - 1; i-- > 0;)
	{
		k[i].curvature = k[i].curvature * k[i + 1].curvature + u[i];
	}
}

float KeyPointSpline::Evaluate(float t, float unchanged) const
{
	const auto& k = _keys;
	if (k.size() < 2)
	{
		return unchanged;
	}
	// Bisection for the keys round t
	size_t lo = 0;
	size_t hi = k.size() - 1;
	while (hi - lo > 1)
	{
		const size_t mid = (hi + lo) / 2;
		if (k[mid].t > t)
		{
			hi = mid;
		}
		else
		{
			lo = mid;
		}
	}
	const float h = k[hi].t - k[lo].t;
	if (h == 0.0f)
	{
		return unchanged;
	}
	const float a = (k[hi].t - t) / h;
	const float b = (t - k[lo].t) / h;
	return a * k[lo].value + b * k[hi].value +
	       ((a * a * a - a) * k[lo].curvature + (b * b * b - b) * k[hi].curvature) * (h * h) * k_Sixth;
}
