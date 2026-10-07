/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MiracleVisuals.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>

#include <glm/gtc/matrix_transform.hpp>

using namespace openblack::magic;
using namespace openblack::magic::visuals;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The way to something below or above the camera is nudged this far along x when it is nearly straight
constexpr float k_NearlyStraightDown = 1e-4f;
/// A glint cell is a quarter of its sheet each way
constexpr float k_GlintCellSize = 0.25f;
constexpr int k_GlintCellsPerRow = 4;
/// A phial's cell is an eighth of its sheet each way, eight to a row
constexpr float k_PhialCellSize = 0.125f;
constexpr int k_PhialCellsPerRow = 8;
/// The pulses: the freeze's speed and the others', and the two shapes
constexpr int k_FreezeReceiveType = 0;
constexpr int k_SmallReceiveType = 1;
constexpr int k_BigReceiveType = 2;
constexpr int k_InvisibleReceiveType = 7;
/// A frozen phial is tinted towards this ice colour
constexpr uint32_t k_FreezeColour = 0x354F8Du;
constexpr float k_FreezePulseSpeed = 0.35f;
constexpr float k_PulseSpeed = 0.5f;
constexpr int k_GrowingReceiveType = 5;
constexpr int k_SquashingReceiveType = 6;
constexpr float k_PulseGrowth = 1.5f;
constexpr float k_PulseSquashAcross = 0.8f;
constexpr float k_PulseSquashDown = 0.7f;
/// The rings' fixed turns
constexpr float k_SecondRingTurn = 0.5f;
constexpr float k_RingFirstTip = 0.3f;
constexpr float k_RingTurnBack = 1.0f;
constexpr float k_RingSecondTip = 0.2f;
/// The bands' timings
constexpr float k_BandDuration = 0.85f;
constexpr float k_FlyOffDuration = 1.0f;
constexpr uint8_t k_BraceletAlphaFrom = 20;
constexpr uint8_t k_BraceletAlphaTo = 130;
constexpr uint8_t k_FlyInAlphaFrom = 20;
constexpr uint8_t k_FlyInAlphaTo = 120;
constexpr uint8_t k_FlyOffAlphaAtCamera = 5;
constexpr uint8_t k_FlyOffAlphaAtHand = 50;
constexpr int k_FlyInBands = 5;
/// A band in front of the camera stays this far beyond the near plane
constexpr float k_BandNearMargin = 0.2f;
/// The announcer's power-up voices in the spell dialogue bank
constexpr int k_FirstPowerUpVoice = 10;
constexpr int k_LastPowerUpLevel = 2;

/// A frame stepped on and kept within its cells, either way round
float Wrap(float frame, float cells)
{
	frame = std::fmod(frame, cells);
	return frame < 0.0f ? frame + cells : frame;
}

/// The rows of a turn, as the game turns its matrices: each row turned about the vertical (mixing x and z) or tipped
/// (mixing x and y)
using Rows = std::array<glm::vec3, 3>;

void TurnAboutVertical(Rows& rows, float angle)
{
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	for (auto& row : rows)
	{
		const float x = row.x;
		row.x = c * x - s * row.z;
		row.z = c * row.z + s * x;
	}
}

void Tip(Rows& rows, float angle)
{
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	for (auto& row : rows)
	{
		const float x = row.x;
		row.x = s * row.y + c * x;
		row.y = c * row.y - s * x;
	}
}
} // namespace

float visuals::StepGlint(float frame, float seconds)
{
	return Wrap(frame + seconds * k_GlintFrameRate, static_cast<float>(k_GlintCells));
}

glm::vec2 visuals::GlintUvOffset(float frame)
{
	const int cell = static_cast<int>(frame) % k_GlintCells;
	return {static_cast<float>(cell % k_GlintCellsPerRow) * k_GlintCellSize,
	        static_cast<float>(cell / k_GlintCellsPerRow) * k_GlintCellSize};
}

int visuals::RingCount(int powerUp)
{
	return powerUp < 0 ? 0 : powerUp + 1;
}

uint8_t visuals::RingAlpha(uint8_t drawnAlpha)
{
	return static_cast<uint8_t>((static_cast<uint32_t>(k_RingAlpha) * drawnAlpha) >> 8u);
}

glm::mat3 visuals::RingTurn(int ring, float spin)
{
	// Laid flat: its height becomes its depth
	Rows rows {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)};
	const bool first = ring == 0;
	TurnAboutVertical(rows, (first ? 0.0f : k_SecondRingTurn) + spin);
	Tip(rows, k_RingFirstTip);
	TurnAboutVertical(rows, first ? -k_RingTurnBack : k_RingTurnBack);
	Tip(rows, k_RingSecondTip);
	// The rows are where each of the ring's axes goes: they are the columns of the turn
	return glm::transpose(glm::mat3(rows[0], rows[1], rows[2]));
}

glm::mat3 visuals::FacingCamera(const glm::vec3& point, const glm::vec3& camera, Facing facing)
{
	auto away = point - camera;
	// Straight up or down from the camera, it is nudged a little along x
	if (std::abs(away.x) < k_NearlyStraightDown && std::abs(away.z) < k_NearlyStraightDown)
	{
		away.x = away.x > 0.0f ? k_NearlyStraightDown : -k_NearlyStraightDown;
	}
	const auto forward = glm::normalize(away);
	const glm::vec3 worldUp {0.0f, 1.0f, 0.0f};
	const auto up = glm::normalize(worldUp - glm::dot(worldUp, forward) * forward);
	const auto across = glm::cross(up, forward);
	return facing == Facing::Ring ? glm::mat3(-forward, up, across) : glm::mat3(across, -forward, up);
}

glm::mat4 visuals::RingModel(int ring, float spin, const glm::vec3& point, float scale, const glm::vec3& camera)
{
	const glm::mat3 turn = FacingCamera(point, camera, Facing::Ring) * RingTurn(ring, spin) * (k_RingScale * scale);
	glm::mat4 model(turn);
	model[3] = glm::vec4(point, 1.0f);
	return model;
}

float visuals::StepPhialFrame(float frame, float seconds)
{
	return Wrap(frame + seconds * k_PhialFrameRate, static_cast<float>(k_PhialCells));
}

glm::vec2 visuals::PhialUvOffset(float frame)
{
	const int cell = static_cast<int>(frame) % k_PhialCells;
	return {static_cast<float>(cell % k_PhialCellsPerRow) * k_PhialCellSize,
	        static_cast<float>(cell / k_PhialCellsPerRow) * k_PhialCellSize};
}

float visuals::PhialPulseSpeed(int receiveType)
{
	return receiveType == k_FreezeReceiveType ? k_FreezePulseSpeed : k_PulseSpeed;
}

float visuals::PhialPulse(float phase)
{
	return (std::sin(phase * k_TwoPi) + 1.0f) * 0.5f;
}

glm::vec3 visuals::PhialScale(int receiveType, float phase)
{
	const float pulse = PhialPulse(phase);
	// Across and through it swells, or squashes; never up
	if (receiveType == k_GrowingReceiveType)
	{
		const float swell = 1.0f + pulse * k_PulseGrowth;
		return {swell, 1.0f, swell};
	}
	if (receiveType == k_SquashingReceiveType)
	{
		return {1.0f - pulse * k_PulseSquashAcross, 1.0f, 1.0f - pulse * k_PulseSquashDown};
	}
	return glm::vec3(1.0f);
}

uint32_t visuals::FreezeTint(float pulse)
{
	const auto k = static_cast<int32_t>(std::nearbyint(pulse * 255.0f));
	uint32_t tint = 0;
	for (const uint32_t shift : {16u, 8u, 0u})
	{
		const auto icy = static_cast<int32_t>((k_FreezeColour >> shift) & 0xFFu);
		const int32_t darkened = ((255 - icy) * k + 255) / 256;
		tint |= static_cast<uint32_t>(255 - darkened) << shift;
	}
	return tint;
}

std::vector<PhialDraw> visuals::PhialDraws(int receiveType, float phase, float pulse, uint8_t globeAlpha)
{
	const auto alpha = static_cast<uint32_t>(globeAlpha);
	switch (receiveType)
	{
	case k_SmallReceiveType:
		// Shrinking away from the full phial, with a faint full one over it
		return {{.size = 1.0f - phase, .alpha = globeAlpha, .lit = false},
		        {.size = 1.0f, .alpha = static_cast<uint8_t>((alpha * 5u) >> 4u), .lit = false}};
	case k_BigReceiveType:
	{
		// The full phial, with a fading one growing out of it
		const auto fading = static_cast<uint32_t>(static_cast<int32_t>(255.0f - 255.0f * phase));
		return {{.size = 1.0f, .alpha = globeAlpha, .lit = false},
		        {.size = 1.0f + phase, .alpha = static_cast<uint8_t>((fading * alpha) >> 8u), .lit = false}};
	}
	case k_InvisibleReceiveType:
		return {{.size = 1.0f,
		         .alpha = static_cast<uint8_t>(static_cast<int32_t>(static_cast<float>(alpha) * pulse)),
		         .lit = true}};
	default:
		return {{.size = 1.0f, .alpha = 255, .lit = true}};
	}
}

void visuals::SetBracelets(HandBands& bands, int count, bool delayed)
{
	count = std::clamp(count, 0, k_MostBracelets);
	while (static_cast<int>(bands.bracelets.size()) > count)
	{
		bands.bracelets.pop_back();
	}
	while (static_cast<int>(bands.bracelets.size()) < count)
	{
		bands.bracelets.push_back({
		    .kind = BandKind::Bracelet,
		    .index = static_cast<int>(bands.bracelets.size()) + 1,
		    .delay = delayed ? k_BandSettleDelay : 0.0f,
		    .duration = k_BandDuration,
		    .alphaFrom = k_BraceletAlphaFrom,
		    .alphaTo = k_BraceletAlphaTo,
		});
	}
}

void visuals::AddFlyIn(HandBands& bands, bool delayed)
{
	const float settle = delayed ? k_BandSettleDelay : 0.0f;
	for (int i = 1; i <= k_FlyInBands; ++i)
	{
		bands.flying.push_back({
		    .kind = BandKind::FlyIn,
		    .index = static_cast<int>(bands.flying.size()),
		    .delay = static_cast<float>(i) * k_FlyInSpacing + settle,
		    .duration = k_BandDuration,
		    .alphaFrom = k_FlyInAlphaFrom,
		    .alphaTo = k_FlyInAlphaTo,
		});
	}
}

void visuals::AddFlyOff(HandBands& bands)
{
	bands.flying.push_back({
	    .kind = BandKind::FlyOff,
	    .index = static_cast<int>(bands.flying.size()),
	    .delay = 0.0f,
	    .duration = k_FlyOffDuration,
	    .alphaFrom = k_FlyOffAlphaAtCamera,
	    .alphaTo = k_FlyOffAlphaAtHand,
	});
}

void visuals::StepBands(HandBands& bands, float seconds)
{
	const auto step = [seconds](HandBand& band) {
		band.age += seconds;
		const float since = band.age - band.delay;
		if (since <= 0.0f)
		{
			return;
		}
		// Further up the arm, a little faster
		const float rate = (static_cast<float>(band.index) * k_BandSpinPerIndex + 1.0f) * k_BandSpin;
		band.spin = std::fmod(band.spin + rate * seconds, k_TwoPi);
		if (since / band.duration > 1.0f)
		{
			band.done = true;
		}
	};
	// The bands that finished their flight were drawn there once, and go
	std::erase_if(bands.flying, [](const HandBand& band) { return band.done; });
	std::ranges::for_each(bands.bracelets, step);
	std::ranges::for_each(bands.flying, step);
}

BandPose visuals::PoseOf(const HandBand& band)
{
	const float since = band.age - band.delay;
	if (since <= 0.0f)
	{
		return {.shown = false, .t = 0.0f, .alpha = 0};
	}
	float t = since / band.duration;
	// The band flying off goes from the hand to the camera
	if (band.kind == BandKind::FlyOff)
	{
		t = 1.0f - t;
	}
	t = std::clamp(t, 0.0f, 1.0f);
	const float alpha = static_cast<float>(band.alphaFrom) + (static_cast<float>(band.alphaTo) - band.alphaFrom) * t;
	return {.shown = true, .t = t, .alpha = static_cast<uint8_t>(alpha)};
}

glm::mat4 visuals::BandOnHandUnspun(const HandBand& band)
{
	const float along = static_cast<float>(band.index) * k_BandOffsetPerIndex + k_BandFirstOffset;
	return glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, along}) * glm::scale(glm::mat4(1.0f), glm::vec3(k_BandRadius));
}

glm::mat4 visuals::BandOnHand(const HandBand& band)
{
	// Spinning about its own length, the other way round from a turn by the spin
	return BandOnHandUnspun(band) * glm::rotate(glm::mat4(1.0f), -band.spin, {0.0f, 0.0f, 1.0f});
}

glm::mat4 visuals::BandAtCamera(float nearPlane)
{
	const float ahead = std::max(k_BandFlyDistance, nearPlane + k_BandNearMargin);
	return glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, ahead}) * glm::scale(glm::mat4(1.0f), glm::vec3(k_BandFlyScale));
}

glm::mat4 visuals::BandBetween(const glm::mat4& atCamera, const glm::mat4& onHand, float t)
{
	// Its size is that of its second axis at either end
	const float from = glm::length(glm::vec3(atCamera[1]));
	const float to = glm::length(glm::vec3(onHand[1]));
	const float size = (to - from) * t + from;
	glm::mat4 between(1.0f);
	for (int axis = 0; axis < 3; ++axis)
	{
		between[axis] = glm::vec4(glm::normalize(glm::vec3(atCamera[axis])) * size, 0.0f);
	}
	between[3] = glm::vec4(glm::vec3(atCamera[3]) + (glm::vec3(onHand[3]) - glm::vec3(atCamera[3])) * t, 1.0f);
	return between;
}

glm::mat4 visuals::BandBlended(const glm::mat4& atCamera, const glm::mat4& onHand, float t)
{
	return atCamera + (onHand - atCamera) * t;
}

float visuals::StepGlowFrame(float frame, float seconds)
{
	// It runs backwards, and once below nothing comes back into two runs of its cells
	static_assert(k_GlowFrameRate < 0.0f);
	const float run = 2.0f * static_cast<float>(k_GlowCells);
	frame += seconds * k_GlowFrameRate;
	return frame < 0.0f ? std::fmod(frame, run) + run : frame;
}

glm::vec2 visuals::GlowUvOffset(float frame)
{
	const int cell = static_cast<int>(std::nearbyint(frame)) % k_GlowCells;
	constexpr int k_CellsPerRow = 8;
	return {static_cast<float>(cell % k_CellsPerRow) * k_GlowCellSize,
	        static_cast<float>(cell / k_CellsPerRow) * k_GlowCellSize};
}

std::optional<int> visuals::PowerUpVoiceSample(int powerUp)
{
	if (powerUp < 0 || powerUp > k_LastPowerUpLevel)
	{
		return std::nullopt;
	}
	return k_FirstPowerUpVoice + powerUp;
}
