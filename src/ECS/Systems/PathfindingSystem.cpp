/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PathfindingSystem.h"

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIsland.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{

void InitializeStep(Transform& transform, WallHug& wallHug, float angle)
{
	transform.rotation = glm::eulerAngleY(-angle - glm::radians(90.0f));
	wallHug.step = glm::vec2(glm::cos(angle), glm::sin(angle)) * wallHug.speed;
	wallHug.yAngle = angle;
}

void InitializeStepToGoal(Transform& transform, WallHug& wallHug)
{
	const auto diff = wallHug.goal - glm::xz(transform.position);
	const auto angle = glm::atan(diff.y, diff.x);
	InitializeStep(transform, wallHug, angle);
}

void InitializeStepAroundObstacle(Transform& transform, WallHug& wallHug, const Fixed& obstacle, float numCirclesAway,
                                  bool clockwise)
{
	const auto diff = obstacle.boundingCenter - glm::xz(transform.position);
	const float clockwiseModifier = clockwise ? 1.0f : -1.0f;
	const float angleStep = glm::radians(90.0f) - glm::radians(90.0f / 8.0f) * numCirclesAway;
	const float angle = glm::atan(diff.y, diff.x);
	InitializeStep(transform, wallHug, angle + angleStep * clockwiseModifier);
}

void IterateStepAroundObstacle(Transform& transform, WallHug& wallHug, const Fixed& obstacle, bool clockwise)
{
	const float clockwiseModifier = clockwise ? 1.0f : -1.0f;
	const float angle = wallHug.yAngle;
	const float angleStep = -wallHug.speed / obstacle.boundingRadius;
	InitializeStep(transform, wallHug, angle + angleStep * clockwiseModifier);
}

void InCircleHugWithoutObject()
{
	throw std::runtime_error("TODO: Handle case of orbiting without an object to orbit");
}
bool AreWeThere(const glm::vec2& pos, const glm::vec2& goal, float threshold)
{
	return glm::distance2(pos, goal) < threshold * threshold;
}

/// If we have a number of turns to an obstacle but no obstacle saved: search in a line to find one
void LinearDetectObstacle(const MoveStateLinearTag state, const WallHugObjectReference& object)
{
	if (object.entity == entt::null)
	{
		throw std::runtime_error("TODO: LinearSquareSweep");
	}
}

/// Get neighbouring cells in this order for traversal
/// +-----+-----+-----+
/// | [5] | [4] | [7] |
/// +-----+-----+-----+
/// | [2] | [0] | [1] |
/// +-----+-----+-----+
/// | [6] | [3] | [8] |
/// +-----+-----+-----+
std::array<ecs::Map::CellId, 9> GetNeighboringCells(const glm::vec2& pos)
{
	const auto cellIndex = Map::GetGridCell(pos);
	assert(glm::compMin(cellIndex) > 0 && glm::all(glm::lessThan(cellIndex, Map::k_GridSize - glm::u16vec2(1))));
	return {
	    cellIndex,                          // Current
	    {cellIndex.x + 1, cellIndex.y},     // Right
	    {cellIndex.x - 1, cellIndex.y},     // Left
	    {cellIndex.x, cellIndex.y + 1},     // Under
	    {cellIndex.x, cellIndex.y - 1},     // Over
	    {cellIndex.x - 1, cellIndex.y - 1}, // Over and left
	    {cellIndex.x - 1, cellIndex.y + 1}, // Under and left
	    {cellIndex.x + 1, cellIndex.y - 1}, // Over and right
	    {cellIndex.x + 1, cellIndex.y + 1}, // Under and right
	};
}

/// Iterate between all adjacent grids and find closest object that the ray (step) intersects with (circle)
/// If that object is in front (and we are not in it) and less than 256 steps away, set as target and store steps
bool LinearScanForObstacle(entt::entity entity, const glm::vec2& pos, const glm::vec2& step)
{
	const auto& map = Game::Instance()->GetEntityMap();
	auto& registry = Game::Instance()->GetEntityRegistry();

	// Reference will be updated or removed
	registry.Remove<WallHugObjectReference>(entity);

	// FIXME(bwrsandman): This gets first, not closest
	std::optional<entt::entity> fixedEntity = std::nullopt;
	for (const auto& c : GetNeighboringCells(pos + step))
	{
		// TODO(bwrsandman): Skip if out of bounds or in water
		const auto& fixed = map.GetFixedInGridCell(c);
		if (!fixed.empty())
		{
			auto iter = std::find_if(fixed.cbegin(), fixed.cend(), [&registry](const auto& f) {
				return !registry.AnyOf<Field>(f); // TODO(bwrsandman) && registry.AllOf<CollideData>();
			});
			if (iter != fixed.cend())
			{
				fixedEntity = std::make_optional(*iter);
				break;
			}
		}
	}
	if (!fixedEntity.has_value())
	{
		return false;
	}

	// Do ray-circle intersection with all objects found
	const auto& fixed = registry.Get<Fixed>(*fixedEntity);
	const auto stepSize = glm::length(step);
	const auto direction = step / stepSize;
	// Do a ray-circle intersection in 2d with ray = {pos, normal}, circle = {fixed.c, fixed.r} (same as ray-sphere)
	const auto oc = pos - fixed.boundingCenter;
	const auto halfB = glm::dot(oc, direction);
	const auto c = glm::length2(oc) - fixed.boundingRadius * fixed.boundingRadius;
	const float discriminant = halfB * halfB - c;
	const bool hit = discriminant > 0;

	// TODO(bwrsandman): if no hit, go through neighbouring cells, if still none, remove WallHugReference and return 1
	if (!hit)
	{
		return false;
	}

	const float t = hit ? -halfB - glm::sqrt(discriminant) : -1.0f;
	const bool inFront = t > 0.0f;

	if (!inFront)
	{
		return false;
	}

	const auto numSteps = t / stepSize;
	assert(numSteps >= 0); // t wouldn't be positive here and size should always be positive

	// Too far
	if (numSteps >= std::numeric_limits<decltype(WallHugObjectReference::stepsAway)>::max())
	{
		return false;
	}

	// Store object and number of steps away
	registry.Assign<WallHugObjectReference>(entity, static_cast<decltype(WallHugObjectReference::stepsAway)>(numSteps),
	                                        *fixedEntity);
	return true;
}

/// If we have a number of turns to an obstacle but no obstacle saved: search in an arc to find one
bool OrbitScanForObstacle(entt::entity entity, const MoveStateOrbitTag state, Transform& transform, WallHug& wallHug)
{
	const auto& registry = Game::Instance()->GetEntityRegistry();
	const auto& reference = registry.Get<const WallHugObjectReference>(entity);
	const auto& obstacleFixed = registry.Get<const Fixed>(reference.entity);
	const auto circleToVillager = glm::xz(transform.position) - obstacleFixed.boundingCenter;
	const float circleToVillagerLength = glm::length(circleToVillager);
	const float numCirclesAway = circleToVillagerLength / obstacleFixed.boundingRadius - 0.9f;
	const auto circleNormal = circleToVillager / circleToVillagerLength;

	const bool closeEnough = glm::distance2(wallHug.goal, obstacleFixed.boundingCenter) <
	                         obstacleFixed.boundingRadius * obstacleFixed.boundingRadius;
	if (closeEnough)
		throw std::runtime_error("TODO: CircleSquareSweep");
	//  else
	//    throw std::runtime_error("TODO: CircleSquareSweep");

	InitializeStepAroundObstacle(transform, wallHug, obstacleFixed, numCirclesAway,
	                             state.clockwise == MoveStateClockwise::Clockwise);

	return true;
}

template <MoveState S>
void StepForward(ecs::Registry& registry)
{
	registry.Each<MoveStateTagComponent<S>, const WallHug, Transform>(
	    [](MoveStateTagComponent<S>& state, const WallHug& wallHug, const Transform& transform) {
		    const auto goal = glm::xz(transform.position) + wallHug.step;
		    state.stepGoal = goal;
	    });
}

template <MoveState S>
bool CellTransition(entt::entity entity, const MoveStateTagComponent<S>& state, Transform& transform, WallHug& wallHug);

template <>
bool CellTransition(entt::entity entity, [[maybe_unused]] const MoveStateTagComponent<MoveState::Linear>& state,
                    Transform& transform, WallHug& wallHug)
{
	InitializeStepToGoal(transform, wallHug);
	return LinearScanForObstacle(entity, glm::xz(transform.position), wallHug.step);
}

template <>
bool CellTransition(entt::entity entity, const MoveStateTagComponent<MoveState::Orbit>& state, Transform& transform,
                    WallHug& wallHug)
{
	return OrbitScanForObstacle(entity, state, transform, wallHug);
}

/// Transition from one grid cell to another requires another check for obstacle in the line
template <MoveState S>
void HandleCellTransition(ecs::Registry& registry)
{
	registry.Each<const MoveStateTagComponent<S>, WallHug, Transform>(
	    [](entt::entity entity, const MoveStateTagComponent<S>& state, WallHug& wallHug, Transform& transform) {
		    const auto position = glm::xz(transform.position);
		    const auto positionId = Map::GetGridCell(position);
		    const auto goalId = Map::GetGridCell(state.stepGoal);
		    if (positionId != goalId)
		    {
			    CellTransition(entity, state, transform, wallHug);
		    }
	    });
}

// TODO(bwrsandman): Vanilla is more complex than this. Update to the map might be needed when transitioning from one block to
// the other.
template <MoveState S>
void ApplyStepGoal(ecs::Registry& registry)
{
	// TODO: WallHug might be redundant here
	registry.Each<const MoveStateTagComponent<S>, const WallHug, Transform>(
	    [](const MoveStateTagComponent<S>& state, const WallHug& wallHug, Transform& transform) {
		    const float altitude = Game::Instance()->GetLandIsland().GetHeightAt(state.stepGoal);
		    transform.position = glm::xzy(glm::vec3(state.stepGoal, altitude));
	    });
}

} // namespace

void PathfindingSystem::Update()
{
	auto& registry = Game::Instance()->GetEntityRegistry();

	// 1.  ARRIVED:
	//         If AreWeThere is false, set to STEP_THROUGH (and it will trigger following steps)
	registry.Each<const MoveStateArrivedTag, const Transform, const WallHug>(
	    [&registry](entt::entity entity, const MoveStateArrivedTag& state, const Transform& transform, const WallHug& wallHug) {
		    if (AreWeThere(glm::xz(transform.position), wallHug.goal, wallHug.speed))
		    {
			    registry.SwapComponents<MoveStateStepThroughTag>(entity, state, state.clockwise);
		    }
	    });

	// 2.  LINEAR, LINEAR_CW, LINEAR_CCW
	//         If we have a number of turns to an obstacle but no obstacle saved: do linear_square_sweep to find one
	registry.Each<const MoveStateLinearTag, const WallHugObjectReference>(LinearDetectObstacle);

	// 3.  ORBIT_CW, ORBIT_CCW, EXIT_CIRCLE_CW, EXIT_CIRCLE_CCW:
	//         If there is no recorded obstacle (what we orbit), this is an unimplemented error
	//         exclude from next parts
	registry.Each<const MoveStateOrbitTag>([&registry](entt::entity entity, [[maybe_unused]] const MoveStateOrbitTag& state) {
		if (!registry.AllOf<WallHugObjectReference>(entity))
		{
			InCircleHugWithoutObject();
		}
	});
	registry.Each<const MoveStateOrbitTag, const WallHugObjectReference>(
	    []([[maybe_unused]] const MoveStateOrbitTag& state, const WallHugObjectReference& object) {
		    if (object.entity == entt::null)
		    {
			    InCircleHugWithoutObject();
		    }
	    });
	registry.Each<const MoveStateExitCircleTag>(
	    [&registry](entt::entity entity, [[maybe_unused]] const MoveStateExitCircleTag& state) {
		    if (!registry.AllOf<WallHugObjectReference>(entity))
		    {
			    InCircleHugWithoutObject();
		    }
	    });
	registry.Each<const MoveStateExitCircleTag, const WallHugObjectReference>(
	    []([[maybe_unused]] const MoveStateExitCircleTag& state, const WallHugObjectReference& object) {
		    if (object.entity == entt::null)
		    {
			    InCircleHugWithoutObject();
		    }
	    });

	// 4a. STEP_THROUGH, EXIT_CIRCLE_CW, EXIT_CIRCLE_CCW:
	//         Do StepForward and ApplyStepGoal for the step distance -> no change to state
	StepForward<MoveState::StepThrough>(registry);
	StepForward<MoveState::ExitCircle>(registry);
	ApplyStepGoal<MoveState::StepThrough>(registry);
	ApplyStepGoal<MoveState::ExitCircle>(registry);

	// 4b. FINAL_STEP, ARRIVED:
	//         Do ApplyStepGoal for the remaining distance to the goal and return a message to change LIVING STATE
	//         exclude from next parts -> no change to state
	ApplyStepGoal<MoveState::FinalStep>(registry);
	ApplyStepGoal<MoveState::Arrived>(registry);

	// 4c. ORBIT_CW, ORBIT_CCW:
	registry.Each<const MoveStateOrbitTag, const WallHugObjectReference, WallHug, Transform>(
	    [&registry](const MoveStateOrbitTag& state, const WallHugObjectReference& reference, WallHug& wallHug,
	                Transform& transform) {
		    IterateStepAroundObstacle(transform, wallHug, registry.Get<Fixed>(reference.entity),
		                              state.clockwise == MoveStateClockwise::Clockwise);
	    });
	StepForward<MoveState::Orbit>(registry);
	HandleCellTransition<MoveState::Orbit>(registry);
	// Decrement turns to object, remove reference once at 0, 0xFF means there is obstacle
	// TODO(#500): split WallHugObjectReference into FutureObstacle and HuggedObstacle
	registry.Each<const MoveStateOrbitTag, WallHugObjectReference>(
	    [&registry](entt::entity entity, const MoveStateOrbitTag&, WallHugObjectReference& reference) {
		    if (reference.stepsAway == std::numeric_limits<decltype(reference.stepsAway)>::max())
		    {
			    return;
		    }
		    if (reference.stepsAway == 0)
		    {
			    registry.Remove<WallHugObjectReference>(entity);
		    }
		    else
		    {
			    --reference.stepsAway;
		    }
	    });
	// Call OrbitScanForObstacle for those without reference
	registry.Each<const MoveStateOrbitTag, entt::exclude_t<WallHugObjectReference>>(
	    [](entt::entity entity, [[maybe_unused]] const MoveStateOrbitTag& state) {
		    throw std::runtime_error("TODO the call to OrbitScanForObstacle doesn't work without reference");
	    });
	ApplyStepGoal<MoveState::Orbit>(registry);
	// Check if it's time to exit circle hug
	registry.Each<const MoveStateOrbitTag, WallHug, Transform, const WallHugObjectReference>(
	    [&registry](entt::entity entity, const MoveStateOrbitTag& state, WallHug& wallHug, Transform& transform,
	                const WallHugObjectReference& reference) {
		    const auto pos = glm::xz(transform.position);
		    if (AreWeThere(pos, wallHug.goal, 0.0f))
		    {
			    registry.SwapComponents<MoveStateFinalStepTag>(entity, state, MoveStateClockwise::Undefined, wallHug.goal);
			    registry.Remove<WallHugObjectReference>(entity);
		    }

		    const auto diff = pos - wallHug.goal;
		    // If continuing around the circle gets us closer, keep hugging
		    // 2D cross product gives the sin between both vectors
		    const float sin = glm::cross(glm::vec3(wallHug.step, 0.0f), glm::vec3(diff, 0.0f)).z;
		    const auto newClockwise = sin > 0.0f ? MoveStateClockwise::Clockwise : MoveStateClockwise::CounterClockwise;
		    assert(state.clockwise != MoveStateClockwise::Undefined);
		    if (state.clockwise == newClockwise)
		    {
			    return;
		    }
		    // If the angle isn't larger than 90 degrees, keep hugging
		    if (glm::dot(wallHug.step, diff) > 0.0f)
		    {
			    return;
		    }

		    const auto& obstacle = registry.Get<const Fixed>(reference.entity);
		    const auto normal = pos - obstacle.boundingCenter;
		    InitializeStep(transform, wallHug, glm::atan(normal.y, normal.x));
		    registry.SwapComponents<MoveStateExitCircleTag>(entity, state, state.clockwise, state.stepGoal);
	    });

	// 4d. LINEAR, LINEAR_CW, LINEAR_CCW:
	//         Do move_to_circle_hug (complex) -> can change state to ORBIT*
	StepForward<MoveState::Linear>(registry);
	HandleCellTransition<MoveState::Linear>(registry);
	// Decrement turns to object, transition to orbit at 0
	registry.Each<const MoveStateLinearTag, Transform, WallHug, WallHugObjectReference>(
	    [&registry](entt::entity entity, const MoveStateLinearTag& state, Transform& transform, WallHug& wallHug,
	                WallHugObjectReference& reference) {
		    assert(reference.stepsAway != 0xFF); // In this case, the component should have been removed
		    if (reference.stepsAway == 0)
		    {
			    auto clockwise = state.clockwise;
			    if (clockwise == MoveStateClockwise::Undefined)
			    {
				    const auto& circleHugFixed = registry.Get<Fixed>(reference.entity);
				    const auto diff = glm::xz(transform.position) - circleHugFixed.boundingCenter;
				    // 2D cross product gives the sin between both vectors
				    const float sin = glm::cross(glm::vec3(wallHug.step, 0.0f), glm::vec3(diff, 0.0f)).z;
				    // Positive is 180 degrees clockwise, negative is 180 degrees counter-clockwise
				    clockwise = sin > 0.0f ? MoveStateClockwise::Clockwise : MoveStateClockwise::CounterClockwise;
			    }
			    // Add orbit, remove linear later
			    auto& newState = registry.Assign<MoveStateOrbitTag>(entity, clockwise, state.stepGoal);
			    reference.stepsAway = std::numeric_limits<decltype(reference.stepsAway)>::max(); // FIXME: useless value
			    // TODO(#500): reference.entity should probably be put in another component
			    // registry.Remove<WallHugObjectReference>(entity);
			    // registry.Remove<MoveStateLinearTag>(entity); // TODO(#500): Maybe do this later

			    // TODO(bwrsandman): perhaps move this to another Each call
			    OrbitScanForObstacle(entity, newState, transform, wallHug);
		    }
		    else
		    {
			    --reference.stepsAway;
		    }
	    });

	ApplyStepGoal<MoveState::Linear>(registry);
	// Clean-up: Remove those which have been transitioned
	registry.Each<const MoveStateLinearTag, const MoveStateOrbitTag>(
	    [&registry](entt::entity entity, const MoveStateLinearTag, const MoveStateOrbitTag) {
		    registry.Remove<MoveStateLinearTag>(entity);
	    });

	// 5.  NOT(FINAL_STEP, ARRIVED): ** PRIOR TO ANY CHANGE OF THE ABOVE STEPS (4c):
	//         if AreWeThere(): sets to FINAL_STEP
	registry.Each<WallHug, const Transform>(
	    [&registry](entt::entity entity, WallHug& wallHug, const Transform& transform) {
		    if (AreWeThere(glm::xz(transform.position), wallHug.goal, wallHug.speed))
		    {
			    registry.Assign<MoveStateFinalStepTag>(entity, MoveStateClockwise::Undefined, wallHug.goal);
			    registry.Remove<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag>(entity);
		    }
	    },
	    entt::exclude<MoveStateFinalStepTag, MoveStateArrivedTag>);

	// 6.  EXIT_CIRCLE_CW, EXIT_CIRCLE_CCW ** PRIOR TO ANY CHANGE OF THE ABOVE STEPS (4c):
	//         if the distance to obstacle is greater than the radius of the circle: set to LINEAR_(C)CW and do
	//         linear_square_sweep
	registry.Each<const MoveStateExitCircleTag, WallHug, const WallHugObjectReference, Transform>(
	    [&registry](entt::entity entity, const MoveStateExitCircleTag& state, WallHug& wallHug,
	                const WallHugObjectReference& object, Transform& transform) {
		    if (object.entity != entt::null)
		    {
			    const auto position = glm::xz(transform.position);
			    const auto& fixed = registry.Get<const Fixed>(object.entity);
			    if (!AreWeThere(position, fixed.boundingCenter, wallHug.speed))
			    {
				    if (!AreWeThere(position, fixed.boundingCenter, fixed.boundingRadius))
				    {
					    InitializeStepToGoal(transform, wallHug);
					    registry.SwapComponents<MoveStateLinearTag>(entity, state, state.clockwise, state.stepGoal);
					    LinearScanForObstacle(entity, position, wallHug.step);
				    }
			    }
		    }
	    });
}
