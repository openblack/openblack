/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "PathfindingSystem.h"

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "Locator.h"

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
	// Use doubles for atan to have same precision on linux, osx and win32
	const glm::dvec2 diff = obstacle.boundingCenter - glm::xz(transform.position);
	const auto angle = static_cast<float>(glm::atan(diff.y, diff.x));

	const float clockwiseModifier = clockwise ? 1.0f : -1.0f;
	const float angleStep = glm::radians(90.0f) - glm::radians(90.0f / 8.0f) * numCirclesAway;
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

/// Get neighbouring cells in this order for traversal
/// +-----+-----+-----+
/// | [5] | [4] | [7] |
/// +-----+-----+-----+
/// | [2] | [0] | [1] |
/// +-----+-----+-----+
/// | [6] | [3] | [8] |
/// +-----+-----+-----+
std::array<ecs::MapInterface::CellId, 9> GetNeighboringCells(const glm::vec2& pos)
{
	const auto cellIndex = MapInterface::GetGridCell(pos);
	assert(glm::compMin(cellIndex) > 0 && glm::all(glm::lessThan(cellIndex, MapInterface::k_GridSize - glm::u16vec2(1))));
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
	const auto& map = Locator::entitiesMap::value();
	auto& registry = Locator::entitiesRegistry::value();

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
				return !registry.AnyOf<Field>(f); // TODO(bwrsandman): && registry.AllOf<CollideData>();
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
bool OrbitScanForObstacle(entt::entity entity, bool clockwise, Transform& transform, WallHug& wallHug)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& map = Locator::entitiesMap::value();
	auto& reference = registry.Get<WallHugObjectReference>(entity);

	const uint32_t numAttempts = 5;
	bool found = false;
	for (uint32_t attempt = 0; attempt < numAttempts; ++attempt)
	{
		const auto& obstacleFixed = registry.Get<const Fixed>(reference.entity);
		const auto circleToVillager = glm::xz(transform.position) - obstacleFixed.boundingCenter;
		const float circleToVillagerLength = glm::length(circleToVillager);
		const float numCirclesAway = circleToVillagerLength / obstacleFixed.boundingRadius - 0.9f;
		const auto circleNormal = circleToVillager / circleToVillagerLength;

		const bool closeEnough = glm::distance2(wallHug.goal, obstacleFixed.boundingCenter) <
		                         obstacleFixed.boundingRadius * obstacleFixed.boundingRadius;
		if (closeEnough)
		{
			throw std::runtime_error("TODO: CircleSquareSweep");
			// Needs to return out of this scope and not run the external following code
		}

		for (const auto& c : GetNeighboringCells(glm::xz(transform.position)))
		{ // TODO(bwrsandman): Skip if out of bounds or in water
			const auto& e = map.GetFixedInGridCell(c);
			if (!e.empty())
			{
				auto iter = std::find_if(e.cbegin(), e.cend(), [&registry, &reference, &obstacleFixed](const auto& f) {
					if (f == reference.entity)
					{
						return false;
					}
					if (registry.AnyOf<Field>(f)) // TODO(bwrsandman): || !registry.AllOf<CollideData>();
					{
						return false;
					}
					const auto& fixed = registry.Get<const Fixed>(f);
					const auto d2 = glm::distance2(fixed.boundingCenter, obstacleFixed.boundingCenter);
					const auto r = fixed.boundingRadius + obstacleFixed.boundingRadius;
					const auto r2 = r * r;
					return d2 < r2 && d2 > 0.0f;
				});
				if (iter != e.cend())
				{
					// https://stackoverflow.com/questions/3349125/circle-circle-intersection-points
					// http://paulbourke.net/geometry/circlesphere/
					const auto& fixed = registry.Get<const Fixed>(*iter);
					const auto d2 = glm::distance2(fixed.boundingCenter, obstacleFixed.boundingCenter);
					const auto d = glm::sqrt(d2);

					// Vanilla bug: Scaling is already applied to boundingRadius, but they apply scale again
					const float fixedScale = glm::compMax(registry.Get<const Transform>(*iter).scale);
					const float obstacleScale = glm::compMax(registry.Get<const Transform>(reference.entity).scale);
					const auto r0 = fixed.boundingRadius * fixedScale;
					const auto r1 = obstacleFixed.boundingRadius * obstacleScale;

					const auto r02 = r0 * r0;
					const auto r12 = r1 * r1;
					const auto p0 = fixed.boundingCenter;
					const auto p1 = obstacleFixed.boundingCenter;
					const auto a = (r02 - r12 + d2) / (2.0f * d); // first circle to intersection midpoint
					const auto h = glm::sqrt(r02 - a * a);        // half height of intersection area
					const auto p2 = p0 + a * (p1 - p0) / d;       // midpoint of overlap
					const auto diff = p1 - p0;
					const auto difft = glm::vec2(diff.y, -diff.x); // 90 degree rotation
					const auto p3 = p2 + h * difft / d;
					const auto p4 = p2 - h * difft / d;

					const auto v0 = p3 - obstacleFixed.boundingCenter;
					const auto v1 = p4 - obstacleFixed.boundingCenter;
					const auto n0 = glm::normalize(v0);
					const auto n1 = glm::normalize(v1);
					const auto dp0 = glm::dot(n0, circleNormal);
					const auto dp1 = glm::dot(n1, circleNormal);
					const auto cp0 = glm::cross(glm::vec3(n0, 0.0f), glm::vec3(circleNormal, 0.0f)).z;
					const auto cp1 = glm::cross(glm::vec3(n1, 0.0f), glm::vec3(circleNormal, 0.0f)).z;
					auto angle0 = glm::acos(dp0);
					auto angle1 = glm::acos(dp1);

					if ((cp0 > 0.0f) ^ clockwise)
					{
						angle0 = 2.0f * glm::pi<float>() - angle0;
					}
					if ((cp1 > 0.0f) ^ clockwise)
					{
						angle1 = 2.0f * glm::pi<float>() - angle1;
					}

					const auto t0 = angle0 * 2.0f / 3.0f * r1 / wallHug.speed;
					const auto t1 = angle1 * 2.0f / 3.0f * r1 / wallHug.speed;
					int t = static_cast<int>(glm::round(glm::min(t0, t1)));

					assert(t >= 0);
					if (t < 1)
					{
						// We're too close to second circle. Act like we're on the second circle and continue looking forward by
						// recursively calling function with new obstacle.
						reference.entity = *iter;
						found = false; // will do another loop
					}
					else if (t < 4)
					{
						t = 0;
						found = true;
					}
					else
					{
						t = glm::min(t, 255);
						found = true;
					}

					reference.stepsAway = static_cast<uint8_t>(t);
				}
				else
				{
					// TODO(bwrsandman):
					// if intersect[0].obj is None:  # True
					//     self.init_steps_xz()
					//     # self.field_0x78 = 0x10
					//     self.circle_hug_info.reset(self)
					//     self.move_state = MoveState.STEP_THROUGH
					// assert(false);
					found = true;
				}
			}
		}
		if (found)
		{
			InitializeStepAroundObstacle(transform, wallHug, obstacleFixed, numCirclesAway, clockwise);
			break;
		}
	}

	return found;
}

template <MoveState S, typename... Exclude>
void StepForward(ecs::Registry& registry, Exclude... exclude)
{
	registry.Each<MoveStateTagComponent<S>, const WallHug, Transform>(
	    [](MoveStateTagComponent<S>& state, const WallHug& wallHug, const Transform& transform) {
		    const auto goal = glm::xz(transform.position) + wallHug.step;
		    state.stepGoal = goal;
	    },
	    exclude...);
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
	return OrbitScanForObstacle(entity, state.clockwise == MoveStateClockwise::Clockwise, transform, wallHug);
}

/// Transition from one grid cell to another requires another check for obstacle in the line
template <MoveState S>
void HandleCellTransition(ecs::Registry& registry)
{
	registry.Each<const MoveStateTagComponent<S>, WallHug, Transform>(
	    [](entt::entity entity, const MoveStateTagComponent<S>& state, WallHug& wallHug, Transform& transform) {
		    const auto position = glm::xz(transform.position);
		    const auto positionId = MapInterface::GetGridCell(position);
		    const auto goalId = MapInterface::GetGridCell(state.stepGoal);
		    if (positionId != goalId)
		    {
			    CellTransition(entity, state, transform, wallHug);
		    }
	    });
}

// TODO(bwrsandman): Vanilla is more complex than this. Update to the map might be needed when transitioning from one block to
// the other.
template <MoveState S, typename... Exclude>
void ApplyStepGoal(ecs::Registry& registry, Exclude... exclude)
{
	registry.Each<const MoveStateTagComponent<S>, Transform>(
	    [](const MoveStateTagComponent<S>& state, Transform& transform) {
		    const float altitude = Locator::terrainSystem::value().GetHeightAt(state.stepGoal);
		    transform.position = glm::xzy(glm::vec3(state.stepGoal, altitude));
	    },
	    exclude...);
}

} // namespace

void PathfindingSystem::Update()
{
	auto& registry = Locator::entitiesRegistry::value();

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
	//         If this is the first turn and there is step size defined
	registry.Each<const MoveStateLinearTag, Transform, WallHug>(
	    [](entt::entity entity, const MoveStateLinearTag&, Transform& transform, WallHug& wallHug) {
		    if (wallHug.step == glm::vec2(0.0f, 0.0))
		    {
			    InitializeStepToGoal(transform, wallHug);
			    LinearScanForObstacle(entity, glm::xz(transform.position), wallHug.step);
		    }
	    },
	    entt::exclude<WallHugObjectReference>);

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

	// 4a. STEP_THROUGH, EXIT_CIRCLE_CW, EXIT_CIRCLE_CCW, LINEAR without obstacles:
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
	    [](const MoveStateOrbitTag&, WallHugObjectReference& reference) {
		    if (reference.stepsAway == std::numeric_limits<decltype(reference.stepsAway)>::max())
		    {
			    return;
		    }
		    if (reference.stepsAway == 0)
		    {
			    reference.stepsAway = std::numeric_limits<decltype(reference.stepsAway)>::max();
		    }
		    else
		    {
			    --reference.stepsAway;
		    }
	    });
	// Call OrbitScanForObstacle for those without reference, jumping from one circle to the next
	// registry.Each<const MoveStateOrbitTag, entt::exclude_t<WallHugObjectReference>>( // FIXME: Exclusion list is not working
	registry.Each<const MoveStateOrbitTag>([&registry](entt::entity entity, [[maybe_unused]] const MoveStateOrbitTag& state) {
		if (!registry.AnyOf<WallHugObjectReference>(entity))
		{
			throw std::runtime_error("TODO: probably transitioning to another circle, scan and select new reference");
		}
	});
	ApplyStepGoal<MoveState::Orbit>(registry);
	// Check if it's time to exit circle hug
	registry.Each<const MoveStateOrbitTag, WallHug, Transform, WallHugObjectReference>(
	    [&registry](entt::entity entity, const MoveStateOrbitTag& state, WallHug& wallHug, Transform& transform,
	                WallHugObjectReference& reference) {
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
		    // Add exit tag, current tag stay to avoid 6. and is removed after
		    registry.Assign<MoveStateExitCircleTag>(entity, state.clockwise, state.stepGoal);
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
			    OrbitScanForObstacle(entity, newState.clockwise == MoveStateClockwise::Clockwise, transform, wallHug);
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
		    if (object.entity != entt::null && !registry.AnyOf<MoveStateOrbitTag>(entity))
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

	// Remove leftover tag from orbit to exit circle transition
	registry.Each<const MoveStateExitCircleTag, const MoveStateOrbitTag>(
	    [&registry](entt::entity entity, const MoveStateExitCircleTag, const MoveStateOrbitTag) {
		    registry.Remove<MoveStateOrbitTag>(entity);
	    });
}
