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

#include <numbers>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::particles
{

/// A sheet of light standing along a row of points, as the game raises one along a recognised gesture's trail. Over each
/// point it rises as a wall, its top spread out from the middle of the points a tenth further, its light brightest a
/// quarter of the way up and dark at the bottom and the top, a sheet of stars sliding up through it. Its height comes
/// from a strength fed in at its first point that runs along it one point every few hundredths of a second, times a wave
/// that rolls along it: each point's height is the strength it was given that long ago times the wave.
class LightSheet
{
public:
	/// The top of the sheet is spread out from the middle of its points by this
	static constexpr float k_Spread = 1.1f;
	/// How far up the sheet its brightest line is
	static constexpr float k_BrightLine = 0.25f;
	/// The stars slide up the sheet this many times a second
	static constexpr float k_SlideSpeed = 1.0f;
	/// The wave has three crests along the sheet and rolls along it at three radians a second, between 0.5 and 1.1 of
	/// the sheet's height
	static constexpr float k_WaveSpan = 6.0f * std::numbers::pi_v<float>;
	static constexpr double k_WaveSpeed = 3.0;
	static constexpr double k_WaveDepth = 0.3;
	static constexpr double k_WaveMiddle = 0.8;
	/// A point's light is its height times this, at most 255
	static constexpr double k_LightPerHeight = 50.0;
	/// The stars repeat this often along the sheet, by distance
	static constexpr float k_StarsPerUnit = 0.1f;
	/// The texture's row of the brightest line
	static constexpr float k_BrightRow = 0.75f;

	/// A corner of the sheet in the world, its colour and the colour added after its texture as 0xAARRGGBB
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t argb;
		uint32_t specularArgb;
	};

	/// Stands it on its points, its light of a colour (0xRRGGBB), its height and how often its strength moves one point
	/// along
	void Start(std::vector<glm::vec3> points, uint32_t rgb, float height, float shiftSeconds);
	[[nodiscard]] bool Started() const { return !_points.empty(); }
	/// The strength fed in at its first point from now on, 0 to 1
	void SetStrength(float strength) { _strength = strength; }
	/// Time passes: the wave rolls on, the stars slide and the strengths move along
	void Update(float seconds);
	/// Its corners, three over each point (the land, the bright line and the top), and its triangles, four between each
	/// pair of points; nothing for fewer than two points. Building it also settles how far the stars have slid and where
	/// its middle is.
	void Build(std::vector<Vertex>& vertices, std::vector<uint32_t>& triangles);
	/// The middle of its points when it was last built, where it takes its place among what blends
	[[nodiscard]] glm::vec3 Middle() const { return _middle; }

	[[nodiscard]] const std::vector<float>& Strengths() const { return _strengths; }
	[[nodiscard]] const std::vector<float>& Heights() const { return _heights; }

private:
	std::vector<glm::vec3> _points;
	/// The strength each point was given, its first point's the newest
	std::vector<float> _strengths;
	/// Each point's height as the wave has it
	std::vector<float> _heights;
	uint32_t _rgb {0};
	float _height {0.0f};
	float _shiftSeconds {0.0f};
	float _strength {0.0f};
	float _time {0.0f};
	float _sinceShift {0.0f};
	float _slide {0.0f};
	glm::vec3 _middle {0.0f};
};

} // namespace openblack::particles
