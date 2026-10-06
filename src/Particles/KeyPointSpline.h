/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <span>
#include <vector>

namespace openblack::particles
{

/// A cubic spline through key points, as the particle files give curves over an atom's life (a stretch or a height
/// against its age). Built from the file's flat (t, value) pairs, with the curve flat at both ends.
class KeyPointSpline
{
public:
	struct Key
	{
		float t;
		float value;
		/// The curve's second derivative at the key
		float curvature;
	};

	KeyPointSpline() = default;
	/// The keys from (t, value) pairs, an odd last number ignored. With zeroEndSlopes the curve leaves the first key and
	/// arrives at the last one flat, as every particle rule has it; without, the ends are free (a natural spline).
	explicit KeyPointSpline(std::span<const float> pairs, bool zeroEndSlopes = true);

	/// The curve at t, between the keys round it and carried on past the ends. With fewer than two keys, or two keys at
	/// the same t round it, there is no curve and `unchanged` is given back.
	[[nodiscard]] float Evaluate(float t, float unchanged) const;
	[[nodiscard]] const std::vector<Key>& Keys() const { return _keys; }

private:
	std::vector<Key> _keys;
};

} // namespace openblack::particles
