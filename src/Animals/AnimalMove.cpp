/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimalMove.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

#include "3D/MapCoords.h"
#include "Common/GUtilsAngle.h"

using namespace openblack;

namespace
{
/// A bird flies no lower than this over the land, and below this goal height it keeps to its height above the land
constexpr float k_FlyingHeight = 2.0f;
/// A quarter turn in game angles
constexpr int32_t k_QuarterTurn = 0x200;

float MetresOf(uint16_t speed)
{
	return map_coords::ToMetres(speed);
}
} // namespace

animals::Turn animals::TurnTowards(uint16_t angle, uint16_t target, uint16_t turnAngle, uint16_t speed, float distance)
{
	const auto difference = static_cast<int32_t>(gutils::GetAngleDifference(angle, target));
	const auto direction = gutils::GetAngleSign(angle, target);
	if (difference == 0)
	{
		return {.angle = target, .step = 0, .direction = 0};
	}
	const auto turn = static_cast<int32_t>(turnAngle);
	int32_t step = difference;
	if (difference > turn)
	{
		// Within twice a step over the turn angle of its goal, it turns harder the closer it is
		const float reach = 2.0f * MetresOf(speed) / gutils::ConvertGameAngleTo3D(turn);
		step = turn;
		if (distance < reach)
		{
			step = static_cast<int32_t>(static_cast<float>(std::max(turn, difference)) -
			                            (static_cast<float>(turn) * distance / reach));
		}
		step = std::min(step, difference);
	}
	const int32_t signedStep = direction < 0 ? -step : step;
	return {.angle = static_cast<uint16_t>((angle + signedStep) & gutils::k_GameAngleMask),
	        .step = signedStep,
	        .direction = direction};
}

glm::ivec2 animals::StepAlong(uint16_t angle, uint16_t speed)
{
	// A sixteenth of the speed times the table's 65536ths, a 4096th of it
	const auto quarter = static_cast<int32_t>(speed >> 4u);
	return {(quarter * gutils::Cos(angle)) >> 12, (quarter * gutils::Sin(angle)) >> 12};
}

bool animals::WithinStep(glm::ivec2 position, glm::ivec2 goal, uint16_t speed)
{
	const auto dx = static_cast<float>(position.x - goal.x);
	const auto dz = static_cast<float>(position.y - goal.y);
	const auto r = static_cast<float>(speed);
	return dx * dx + dz * dz <= r * r;
}

namespace
{
/// The turn towards the goal at the start of a turn's step, and the step that heading makes
animals::Turn TurnAndStep(animals::Move& move, uint16_t turnAngle)
{
	const auto target = gutils::GetAngleFromXZ(move.position, move.goal);
	const float distance = glm::length(glm::vec2(move.goal - move.position)) * map_coords::k_MetresPerFixed;
	const auto turn = animals::TurnTowards(move.angle, target, turnAngle, move.speed, distance);
	move.angle = turn.angle;
	move.step = animals::StepAlong(move.angle, move.speed);
	return turn;
}
} // namespace

animals::Turn animals::SetUpMove(Move& move, glm::ivec2 goal, uint16_t turnAngle)
{
	move.goal = goal;
	const auto turn = TurnAndStep(move, turnAngle);
	move.stage = WithinStep(move.position, move.goal, move.speed) ? MoveStage::AtGoal : MoveStage::StepThrough;
	return turn;
}

animals::MoveResult animals::StepMove(Move& move, uint16_t turnAngle)
{
	MoveResult result;
	switch (move.stage)
	{
	case MoveStage::FinalStep:
		move.position = move.goal;
		result.arrived = true;
		return result;
	case MoveStage::AtGoal:
		if (WithinStep(move.position, move.goal, move.speed))
		{
			move.position = move.goal;
			result.arrived = true;
			return result;
		}
		move.stage = MoveStage::StepThrough;
		[[fallthrough]];
	case MoveStage::StepThrough:
		// An animal turns afresh every turn, then takes its step
		result.turn = TurnAndStep(move, turnAngle);
		result.turned = true;
		move.position += move.step;
		if (WithinStep(move.position, move.goal, move.speed))
		{
			move.stage = MoveStage::FinalStep;
		}
		return result;
	}
	return result;
}

float animals::FlyingHeight(float groundBefore, float heightBefore, float groundAfter, float groundAtGoal, float goalHeight,
                            float change)
{
	if (goalHeight < k_FlyingHeight)
	{
		// Low down it keeps its height over the land, with no snapping onto the goal's
		if (heightBefore > goalHeight + change)
		{
			return heightBefore - change;
		}
		if (heightBefore < goalHeight - change)
		{
			return heightBefore + change;
		}
		return heightBefore;
	}
	const float now = groundBefore + heightBefore;
	const float wanted = groundAtGoal + goalHeight;
	float height = now;
	if (wanted + change < now)
	{
		height = now - change;
	}
	else if (wanted - change > now)
	{
		height = now + change;
	}
	return std::max(height - groundAfter, k_FlyingHeight);
}

glm::vec2 animals::WorkingPosition(glm::vec2 prey, float preyRadius, glm::vec2 hunter, float hunterRadius)
{
	const float angle = gutils::Get3DAngleFromXZ(prey, hunter);
	const auto offset = gutils::GetPointFromAngle(angle, hunterRadius + preyRadius);
	return prey + glm::vec2(offset.x, offset.z);
}

bool animals::OutsideTurningCircles(glm::vec2 animal, uint16_t angle, uint16_t speed, uint16_t turnAngle, glm::vec2 point)
{
	// The radius it turns in: twice its speed over its turn angle in radians, a whole number of map units
	const float radius = map_coords::ToMetres(
	    static_cast<int32_t>(static_cast<float>(2 * static_cast<int32_t>(speed)) / gutils::ConvertGameAngleTo3D(turnAngle)));
	for (const int32_t side : {k_QuarterTurn, -k_QuarterTurn})
	{
		const auto sideAngle = static_cast<uint16_t>((angle + side) & gutils::k_GameAngleMask);
		const glm::vec2 centre =
		    animal + glm::vec2(gutils::GetXByAngle(sideAngle, radius), gutils::GetZByAngle(sideAngle, radius));
		if (!(glm::distance(point, centre) > radius))
		{
			return false;
		}
	}
	return true;
}

bool animals::OverLandAllTheWay(glm::vec2 bird, glm::vec2 point, const std::function<bool(glm::ivec2 cell)>& hasLand)
{
	// A metre value on the middle of its 16 m square, truncated, and divided by the square towards nothing
	const auto middle = [](float metres) {
		const auto whole = static_cast<int32_t>(metres);
		return static_cast<float>((whole / 16 * 16) + 8);
	};
	const glm::vec2 from(middle(point.x), middle(point.y));
	const glm::vec2 to(middle(bird.x), middle(bird.y));
	const glm::vec2 sign(to.x - from.x > 0.0f ? 1.0f : -1.0f, to.y - from.y > 0.0f ? 1.0f : -1.0f);
	const glm::vec2 length = sign * (to - from);
	glm::vec2 gone(0.0f);
	constexpr float k_Step = 16.0f;
	while (!(length.x <= gone.x && length.y <= gone.y))
	{
		// Across or along, whichever keeps the walk nearer the line
		const float side = gone.y * length.x - gone.x * length.y;
		if (side == 0.0f ? length.x <= length.y : side <= 0.0f)
		{
			gone.y += k_Step;
		}
		else
		{
			gone.x += k_Step;
		}
		const glm::vec2 at = sign * gone + from;
		const glm::ivec2 cell(static_cast<int32_t>(at.x * 0.1f), static_cast<int32_t>(at.y * 0.1f));
		if (cell.x < 0 || cell.x >= static_cast<int32_t>(map_coords::k_MapCells) || cell.y < 0 ||
		    cell.y >= static_cast<int32_t>(map_coords::k_MapCells) || !hasLand(cell))
		{
			return false;
		}
	}
	return true;
}
