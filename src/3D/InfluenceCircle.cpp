/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "InfluenceCircle.h"

#include <cmath>

#include <algorithm>

namespace openblack::influence
{

namespace
{
constexpr float k_TwoPi = 6.28318548f;
/// A step round the circle every 20 units, between 8 and 250 of them
constexpr float k_StepsPerUnit = 0.05f;
constexpr int32_t k_FewestSteps = 8;
constexpr int32_t k_MostSteps = 250;
/// The texture goes round about once every 111 units, and up a turn for each 60 units of radius, at most 6
constexpr float k_MinusOneOver111 = -0.009009008f;
constexpr float k_OneOver60 = 0.0166666675f;
constexpr int32_t k_MostTurns = 6;
/// The curtain's rows over the ground, and their places in the texture
constexpr std::array<float, 3> k_RowHeights = {0.0f, 20.0f, 40.0f};
constexpr std::array<float, 3> k_RowTexture = {0.0f, 0.2f, 0.4f};
/// The camera's heights the border comes in between, and its strength there
constexpr float k_HiddenBelow = 100.0f;
constexpr float k_WholeAbove = 200.0f;
constexpr uint8_t k_WholeAlpha = 120;

/// A whole number, as the game cuts it
int32_t Cut(float value)
{
	return static_cast<int32_t>(value);
}

/// Whether a point is inside a circle across the ground
bool Inside(const glm::vec3& centre, float radius, const glm::vec3& point)
{
	const float dx = point.x - centre.x;
	const float dz = point.z - centre.z;
	return (dx * dx) + (dz * dz) < radius * radius;
}

/// Hides the columns of a circle that are inside another
void HideInside(Circle& circle, const Circle& other)
{
	for (size_t i = 0; i < circle.Columns(); ++i)
	{
		if (Inside(other.centre, other.radius, circle.curtain.positions.at(3 * i)))
		{
			circle.hidden.at(i) = true;
		}
	}
}

/// Two circles of the same player: one wholly inside the other goes, else each hides its columns inside the other
void Overlap(Circle& circle, Circle& older)
{
	if (circle.dead || older.dead || circle.player != older.player)
	{
		return;
	}
	const auto apart = circle.centre - older.centre;
	const float distance = std::sqrt((apart.x * apart.x) + (apart.y * apart.y) + (apart.z * apart.z));
	if (distance + circle.radius < older.radius)
	{
		circle.dead = true;
	}
	else if (distance + older.radius < circle.radius)
	{
		older.dead = true;
	}
	else
	{
		HideInside(circle, older);
		HideInside(older, circle);
	}
}
} // namespace

Curtain MakeCurtain(const Ground& ground, const glm::vec3& centre, float radius)
{
	Curtain curtain;
	const float circumference = radius * k_TwoPi;
	const int32_t steps = std::clamp(Cut(circumference * k_StepsPerUnit), k_FewestSteps, k_MostSteps);
	const float uStep = static_cast<float>(1 - Cut(circumference * k_MinusOneOver111)) / static_cast<float>(steps);
	const float vStep = static_cast<float>(std::min(Cut(radius * k_OneOver60), k_MostTurns)) / static_cast<float>(steps);
	// The curtain keeps the shape of the land under it, from the land's height at the centre
	const float centreHeight = ground(glm::vec2(centre.x, centre.z));
	float u = 0.0f;
	float v = 0.0f;
	const auto addColumn = [&](float x, float z) {
		const float rise = ground(glm::vec2(x, z)) - centreHeight;
		for (size_t row = 0; row < k_RowHeights.size(); ++row)
		{
			curtain.positions.emplace_back(x, rise + (centreHeight + k_RowHeights.at(row)), z);
			curtain.uvs.emplace_back(u, v + k_RowTexture.at(row));
		}
	};
	for (int32_t i = 0; i < steps; ++i)
	{
		const float angle = static_cast<float>(i) * k_TwoPi / static_cast<float>(steps);
		addColumn((std::cos(angle) * radius) + centre.x, (std::sin(angle) * radius) + centre.z);
		const auto b = static_cast<uint32_t>(3 * i);
		for (const uint32_t index : {b, b + 3, b + 4, b, b + 4, b + 1, b + 1, b + 4, b + 5, b + 1, b + 5, b + 2})
		{
			curtain.indices.push_back(index);
		}
		u = uStep + u;
		v = vStep + v;
	}
	// The ring closes where it started
	addColumn(radius + centre.x, centre.z);
	return curtain;
}

void AddCircle(std::vector<Circle>& circles, PlayerNames player, const glm::vec3& centre, float radius, const Ground& ground)
{
	Circle circle {.player = player, .centre = centre, .radius = radius, .curtain = MakeCurtain(ground, centre, radius)};
	circle.hidden.assign(circle.Columns(), false);
	circles.insert(circles.begin(), std::move(circle));
	for (size_t i = 1; i < circles.size(); ++i)
	{
		Overlap(circles.front(), circles.at(i));
	}
	std::erase_if(circles, [](const Circle& c) { return c.dead; });
}

std::optional<uint8_t> CurtainAlpha(float cameraHeight)
{
	if (cameraHeight <= k_HiddenBelow)
	{
		return std::nullopt;
	}
	if (cameraHeight >= k_WholeAbove)
	{
		return k_WholeAlpha;
	}
	return static_cast<uint8_t>(Cut((cameraHeight - k_HiddenBelow) * 0.01f * static_cast<float>(k_WholeAlpha)));
}

glm::vec2 ScrollOffset(int32_t milliseconds)
{
	const auto wrapped = milliseconds % k_ScrollWrap;
	return {static_cast<float>(wrapped) * 0.0001f, static_cast<float>(-wrapped) * 0.0002f};
}

float OnRange(float distance, float radius, float fullPart, float fallingPart, float small)
{
	const float full = fullPart * radius;
	if (distance <= full)
	{
		return 1.0f;
	}
	const float outer = (fallingPart + fullPart) * radius;
	if (distance <= outer)
	{
		return (1.0f - (distance - full) / (outer - full)) * (1.0f - small);
	}
	if (distance < radius)
	{
		return (1.0f - (distance - outer) / (radius - outer)) * 0.2f;
	}
	return 0.0f;
}

} // namespace openblack::influence
