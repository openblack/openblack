/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleMaths.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Common/GameRandom.h"

using namespace openblack::particles;

namespace
{
/// The low six bits of a sprite cell number are the cell
constexpr uint32_t k_SpriteCellMask = 0x3Fu;
/// A looping animation's frame may be drawn up to this many steps ahead of the last
constexpr float k_MaxLoopedFrameLerp = 5.0f;
/// Frames further out than this can no longer be moved by whole cycles in single precision
constexpr float k_LargestSteppableFrame = 16777216.0f;

/// The permutation the noise reads its lattice through
constexpr std::array<uint8_t, 256> k_Permutation {
    225, 155, 210, 108, 175, 199, 221, 144, 203, 116, 70,  213, 69,  158, 33,  252, 5,   82,  173, 133, 222, 139, 174, 27,
    9,   71,  90,  246, 75,  130, 91,  191, 169, 138, 2,   151, 194, 235, 81,  7,   25,  113, 228, 159, 205, 253, 134, 142,
    248, 65,  224, 217, 22,  121, 229, 63,  89,  103, 96,  104, 156, 17,  201, 129, 36,  8,   165, 110, 237, 117, 231, 56,
    132, 211, 152, 20,  181, 111, 239, 218, 170, 163, 51,  172, 157, 47,  80,  212, 176, 250, 87,  49,  99,  242, 136, 189,
    162, 115, 44,  43,  124, 94,  150, 16,  141, 247, 32,  10,  198, 223, 255, 72,  53,  131, 84,  57,  220, 197, 58,  50,
    208, 11,  241, 28,  3,   192, 62,  202, 18,  215, 153, 24,  76,  41,  15,  179, 39,  46,  55,  6,   128, 167, 23,  188,
    106, 34,  187, 140, 164, 73,  112, 182, 244, 195, 227, 13,  35,  77,  196, 185, 26,  200, 226, 119, 31,  123, 168, 125,
    249, 68,  183, 230, 177, 135, 160, 180, 12,  1,   243, 148, 102, 166, 38,  238, 251, 37,  240, 126, 64,  74,  161, 40,
    184, 149, 171, 178, 101, 66,  29,  59,  146, 61,  254, 107, 42,  86,  154, 4,   236, 232, 120, 21,  233, 209, 45,  98,
    193, 114, 78,  19,  206, 14,  118, 127, 48,  79,  147, 85,  30,  207, 219, 54,  88,  234, 190, 122, 95,  67,  143, 109,
    137, 214, 145, 93,  92,  100, 245, 0,   216, 186, 60,  83,  105, 97,  204, 52};

glm::mat3 Rotation(float glmAngle, const glm::vec3& axis)
{
	return glm::mat3(glm::rotate(glm::mat4(1.0f), glmAngle, axis));
}
} // namespace

std::optional<float> maths::TimedValue(float age, float dt, float start, float stop, float from, float to)
{
	if (age >= start && age <= stop)
	{
		return stop > start ? from + ((age - start) / (stop - start)) * (to - from) : to;
	}
	if (age > stop && age - dt <= stop)
	{
		return to;
	}
	return std::nullopt;
}

uint8_t maths::TruncateToByte(float value)
{
	if (!std::isfinite(value))
	{
		return 0;
	}
	return static_cast<uint8_t>(static_cast<int64_t>(value) & 0xFF);
}

std::array<uint8_t, 4> maths::TintWithPlayerColour(std::array<uint8_t, 4> rgba, uint32_t playerRgb, float blend)
{
	uint32_t colour = playerRgb & 0xFFFFFFu;
	if (colour == 0)
	{
		colour = 0xFFFFFFu;
	}
	std::array<uint32_t, 3> channels {(colour >> 16u) & 0xFFu, (colour >> 8u) & 0xFFu, colour & 0xFFu};
	if (blend != 1.0f)
	{
		const int b = static_cast<int>(blend * 255.0f) & 0xFF;
		for (auto& c : channels)
		{
			c = static_cast<uint32_t>((255 + (((static_cast<int>(c) - 255) * b) >> 8)) & 0xFF);
		}
	}
	return {static_cast<uint8_t>((rgba[0] * channels[0]) >> 8u), static_cast<uint8_t>((rgba[1] * channels[1]) >> 8u),
	        static_cast<uint8_t>((rgba[2] * channels[2]) >> 8u), static_cast<uint8_t>((rgba[3] * 255u) >> 8u)};
}

float maths::ChakraFade(float age, float ageMax, float ageZero)
{
	const float t = age < ageMax ? age / ageMax : 1.0f - ((age - ageMax) / (ageZero - ageMax));
	// Not a number, as at an age of 0 with no rise, counts as nothing
	if (!(t > 0.0f))
	{
		return 0.0f;
	}
	return std::min(t, 1.0f);
}

int maths::SoundSizeFromRadius(float radius, float small, float medium)
{
	if (radius < small)
	{
		return 3;
	}
	return radius < medium ? 2 : 1;
}

void maths::AdvanceFrame(float& previous, float& current, float dt, float rate, int frames, bool playing)
{
	previous = current;
	if (!playing)
	{
		return;
	}
	current = previous + dt * rate;
	if (frames <= 0)
	{
		return;
	}
	const auto n = static_cast<float>(frames);
	const float twoN = n + n;
	if (!std::isfinite(current) || std::abs(current) > k_LargestSteppableFrame || std::abs(previous) > k_LargestSteppableFrame)
	{
		current = std::isfinite(current) ? std::fmod(current, n) : 0.0f;
		previous = current;
	}
	if (rate > 0.0f)
	{
		while (current > twoN && previous > twoN)
		{
			current -= n;
			previous -= n;
		}
	}
	else
	{
		while (current < 0.0f || previous < 0.0f)
		{
			current += twoN;
			previous += twoN;
		}
	}
}

float maths::LerpFrame(float previous, float current, float t, bool loop)
{
	const float limit = loop ? k_MaxLoopedFrameLerp : 1.0f;
	return (current - previous) * std::clamp(t, 0.0f, limit) + previous;
}

int maths::FrameIndex(float frame, int frames, bool loop)
{
	const int n = std::max(1, frames);
	if (loop)
	{
		float wrapped = std::fmod(frame, static_cast<float>(n));
		if (wrapped < 0.0f)
		{
			wrapped += static_cast<float>(n);
		}
		return std::min(static_cast<int>(wrapped), n - 1);
	}
	return std::clamp(static_cast<int>(frame), 0, n - 1);
}

maths::UvRect maths::SpriteCellUv(int cell, int cellsPerRow)
{
	const auto n = static_cast<uint32_t>(std::max(1, cellsPerRow));
	const auto c = static_cast<uint32_t>(cell) & k_SpriteCellMask;
	const float size = 1.0f / static_cast<float>(n);
	return {.corner = {static_cast<float>(c % n) * size, static_cast<float>(c / n) * size}, .size = {size, size}};
}

glm::mat3 maths::AngleY(float a)
{
	return Rotation(-a, {0.0f, 1.0f, 0.0f});
}

glm::mat3 maths::AngleXYZ(float x, float y, float z)
{
	return Rotation(-z, {0.0f, 0.0f, 1.0f}) * Rotation(-y, {0.0f, 1.0f, 0.0f}) * Rotation(-x, {1.0f, 0.0f, 0.0f});
}

glm::mat3 maths::TurnAboutAxis(const glm::mat3& rotation, int axis, float a)
{
	glm::vec3 direction(0.0f);
	direction[std::clamp(axis, 0, 2)] = 1.0f;
	return Rotation(-a, direction) * rotation;
}

float maths::SpriteAngle(const glm::mat3& rotation)
{
	return std::atan2(rotation[0][2], rotation[0][0]);
}

float maths::ScreenVelocityAngle(const glm::vec3& velocity, const glm::vec3& right, const glm::vec3& up)
{
	const float x = glm::dot(velocity, right);
	const float y = glm::dot(velocity, up);
	return std::atan2(-y, x) + std::numbers::pi_v<float> * 0.5f;
}

maths::ValueNoise::ValueNoise()
{
	// Drawn once from a seed of 0 before the game seeds its generators, so the same values in every game
	uint32_t seed = 0;
	for (auto& value : _values)
	{
		const float r = game_random::FloatRand(2.0f, seed);
		value = 1.0f - r;
	}
}

float maths::ValueNoise::Lattice(int x, int y, int z) const
{
	const auto index = [](int i) { return static_cast<size_t>(static_cast<uint32_t>(i) & 0xFFu); };
	const auto pz = k_Permutation.at(index(z));
	const auto py = k_Permutation.at(index(static_cast<int>(pz) + y));
	const auto px = k_Permutation.at(index(static_cast<int>(py) + x));
	return _values.at(px);
}

float maths::ValueNoise::At(const glm::vec3& point) const
{
	const auto ix = static_cast<int>(std::floor(point.x));
	const auto iy = static_cast<int>(std::floor(point.y));
	const auto iz = static_cast<int>(std::floor(point.z));
	const float fx = point.x - static_cast<float>(ix);
	const float fy = point.y - static_cast<float>(iy);
	const float fz = point.z - static_cast<float>(iz);
	std::array<float, 2> alongZ {};
	for (int k = 0; k < 2; ++k)
	{
		std::array<float, 2> alongY {};
		for (int j = 0; j < 2; ++j)
		{
			const float a = Lattice(ix, iy + j, iz + k);
			const float b = Lattice(ix + 1, iy + j, iz + k);
			alongY.at(static_cast<size_t>(j)) = (b - a) * fx + a;
		}
		alongZ.at(static_cast<size_t>(k)) = (alongY[1] - alongY[0]) * fy + alongY[0];
	}
	return (alongZ[1] - alongZ[0]) * fz + alongZ[0];
}

glm::vec2 maths::ValueNoise::Wind(const glm::vec3& point) const
{
	return {At(point), At({point.y, point.z, point.x})};
}
