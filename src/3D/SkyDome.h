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

#include <array>
#include <optional>
#include <span>

#include <glm/gtc/type_precision.hpp>
#include <glm/vec3.hpp>

/// The sky's dome: three pictures of the sky for each alignment, by day, at dusk and at night, blended into one for the
/// time of day. The blend doesn't keep up with every change of the sky: it starts again only once the sky has moved
/// on far enough from the one it was built for, and then builds the dome a band of rows a frame, top down. The rows'
/// texels blend in whole steps of the 5-bit channels the pictures are stored in. The alignments' domes are then
/// mixed by the sky's alignment.
namespace openblack::sky_dome
{

/// The rows of a dome's picture
inline constexpr uint16_t k_Rows = 256;
/// The rows built again each frame while the dome follows the sky
inline constexpr uint16_t k_RowsPerFrame = 32;
/// How far the sky moves on before the dome starts following it again
inline constexpr double k_Hysteresis = static_cast<double>(0.03f);

/// Rows of the dome to blend again for a sky type
struct Rows
{
	float skyType;
	uint16_t first;
	uint16_t count;

	bool operator==(const Rows&) const = default;
};

/// What a frame blends again: a whole dome after a jump, then the band it is following with
struct FrameRows
{
	std::array<Rows, 2> rows {};
	uint8_t count {0};

	[[nodiscard]] std::span<const Rows> Get() const { return std::span(rows).first(count); }
};

/// The dome following the sky type, with sky types from 0 at night to 2 by day
class Follow
{
public:
	/// As the sky is set up: the whole dome built for the sky type now, though it follows on as if built by day
	explicit Follow(float skyType);

	/// Once a frame, with the frame's sky type
	[[nodiscard]] FrameRows Advance(float skyType);
	/// The time jumps: the whole dome is built again for the sky type now
	void Jump(float skyType);

	[[nodiscard]] float Following() const { return _following; }
	[[nodiscard]] uint16_t RowsDone() const { return _rowsDone; }

private:
	float _following {2.0f};
	uint16_t _rowsDone {k_Rows};
	std::optional<float> _wholeDome;
};

/// Two of three pictures and how much of the second there is, of 255
struct Pair
{
	uint8_t lower;
	uint8_t upper;
	uint8_t weight;

	bool operator==(const Pair&) const = default;
};

/// The pictures of the time of day a sky type blends, as 0 night, 1 dusk and 2 day: day into dusk, or dusk into night
[[nodiscard]] Pair TimePair(float skyType);
/// The alignments' domes a sky alignment from 0, evil, to 2, good, mixes: the second drawn over the first
[[nodiscard]] Pair AlignmentPair(float alignment);
/// A 5-bit channel of two pictures blended by a weight of 255, in whole steps: the weights only add up to 255 of 256
[[nodiscard]] uint8_t BlendChannel(uint8_t lower, uint8_t upper, uint8_t weight);

/// The colours each alignment's dome is drawn in: its picture times the first, with the second added, 0 to 255
struct Tint
{
	glm::u8vec3 modulate {255};
	glm::u8vec3 add {0};

	bool operator==(const Tint&) const = default;
};

/// What tints the dome in a frame
struct TintInputs
{
	/// The colour of the distance haze, 0 to 255, which is a third of the land's
	glm::vec3 hazeColour;
	/// The overcast at the camera, 0 for a clear sky and 1 for a full one
	float overcast;
	/// A flash of lightning, 0 to 255
	uint8_t flash;
	/// How dark an evil sky is, 0 to 90
	uint8_t darkness;
	/// The fog setting: an overcast turns the dome towards the haze's colour
	bool fog;
	/// The weather setting: an evil sky darkens the dome
	bool weather;
};

/// How dark the sky is for an alignment from 0, evil, to 2, good: nothing until it is a little evil, 90 at its most
[[nodiscard]] uint8_t Darkness(float alignment);
/// The dome's colours: white with nothing added on a clear day, towards the haze's colour as an overcast comes in, a
/// little darker under an evil sky and towards white in a flash of lightning, all in whole steps
[[nodiscard]] Tint TintOf(const TintInputs& inputs);

} // namespace openblack::sky_dome
