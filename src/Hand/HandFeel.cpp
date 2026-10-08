/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandFeel.h"

#include <cmath>

#include <glm/geometric.hpp>

using namespace openblack;

namespace
{
/// How much of the way from the face towards the camera the hand leans, and how much it stands upwards
constexpr float k_TowardsCamera = 0.25f;
constexpr float k_Upwards = 0.5f;
/// Holding something, the hand is kept out from the face by this share of the held thing's radius
constexpr float k_HeldShare = 0.5f;

[[nodiscard]] glm::vec3 Normalised(glm::vec3 v)
{
	if (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f)
	{
		return v;
	}
	return v * (1.0f / std::sqrt(glm::dot(v, v)));
}
} // namespace

hand_feel::Feel hand_feel::FeelOf(const Picked& picked)
{
	if (picked.ignored)
	{
		return Feel::Nothing;
	}
	if (picked.living)
	{
		return picked.creature ? Feel::OnModel : Feel::AtPosition;
	}
	return picked.posedModel && !picked.animatedStatic ? Feel::AtPosition : Feel::OnModel;
}

glm::vec3 hand_feel::FeelDirection(glm::vec3 camera, glm::vec3 cursorDirection, std::optional<glm::vec3> raisedCursorGround)
{
	if (!raisedCursorGround.has_value())
	{
		return cursorDirection;
	}
	return Normalised(*raisedCursorGround - camera);
}

hand_feel::Rest hand_feel::RestOnModel(glm::vec3 camera, glm::vec3 cursorDirection, glm::vec3 feltDirection, glm::vec3 hitPoint,
                                       glm::vec3 hitNormal, bool feelsModel, std::optional<Holding> holding)
{
	const auto normal = Normalised(hitNormal);
	auto point = hitPoint;
	auto towardsCamera = camera - point;
	// How far along the felt direction the face is
	const float along = -glm::dot(towardsCamera, feltDirection);
	if (holding.has_value())
	{
		// Holding something, the point is put on the line of sight at the same distance
		point = cursorDirection * along + camera;
		towardsCamera = camera - point;
	}
	if (1.0f <= along)
	{
		towardsCamera = Normalised(towardsCamera);
	}
	else
	{
		point = camera + cursorDirection;
		towardsCamera = -cursorDirection;
	}
	if (holding.has_value())
	{
		float deeper = 0.0f;
		if (holding->creature.has_value())
		{
			const float across = glm::length(glm::vec2(point.x, point.z) - holding->creature->centre);
			if (across < holding->creature->radius)
			{
				deeper = holding->creature->radius - across;
			}
		}
		point += towardsCamera * (holding->radius * k_HeldShare + deeper);
	}
	if (!feelsModel)
	{
		return {.point = point, .up = std::nullopt};
	}
	const auto up = towardsCamera * k_TowardsCamera - normal + glm::vec3(0.0f, k_Upwards, 0.0f);
	return {.point = point, .up = up};
}

glm::vec3 hand_feel::RestAtPosition(glm::vec3 camera, glm::vec3 cursorDirection, glm::vec3 feltDirection, glm::vec3 position,
                                    float radius)
{
	const float distance = glm::length(position - camera) - radius;
	const auto first = camera + Normalised(cursorDirection) * distance;
	return camera + Normalised(cursorDirection) * ((first.y - position.y) * feltDirection.y + distance);
}

bool hand_feel::TurnsToLand(bool restsOnFace, bool overThing, float aboveLand, float handHeight)
{
	return !restsOnFace || (overThing && aboveLand < handHeight * 0.5f);
}
