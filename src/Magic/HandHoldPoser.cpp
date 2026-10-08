/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandHoldPoser.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "3D/HandAnimation.h"
#include "3D/L3DMesh.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::magic;
using namespace openblack::magic::hand_hold;
using openblack::ecs::components::Transform;

namespace
{
/// A seed's height: its model's bounding box from bottom to top at its scale, none without a model
float SeedHeight(entt::id_type mesh, float scale)
{
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh) ? meshes.Handle(mesh)->GetBoundingBox().Size().y * scale : 0.0f;
}
} // namespace

std::optional<HandHoldPoser::HeldSeed> HandHoldPoser::Find()
{
	if (!Locator::magicSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto entity = Locator::magicSystem::value().GetHeldSeed();
	const auto& registry = Locator::entitiesRegistry::value();
	if (!entity.has_value() || !registry.Valid(*entity))
	{
		return std::nullopt;
	}
	const auto* seed = registry.TryGet<const ecs::components::SpellSeed>(*entity);
	const auto* transform = registry.TryGet<const Transform>(*entity);
	if (seed == nullptr || transform == nullptr)
	{
		return std::nullopt;
	}
	const auto& info = magic::GetSpellSeedInfo(Locator::infoConstants::value(), seed->seedType);
	const float scale = transform->scale.y;
	// Its height is that of its model
	const float height = SeedHeight(resources::HashIdentifier(info.mesh), scale);
	return HeldSeed {
	    .entity = *entity,
	    .hold = seed->holdType,
	    .hang = SeedHang(seed->holdType, info.holdLoweringMultiplier, height),
	    .reach = scale * info.holdRadius,
	    .yRotate = info.holdYRotate,
	    .effectInFingers = info.attachInHandEffectToBone == 1,
	};
}

float HandHoldPoser::Lift(const HeldSeed& seed, float landDistance)
{
	return HoldLift(seed.hold, seed.hang, HandAnimation::SizeAtDistance(landDistance));
}

bool HandHoldPoser::Pose(const std::optional<HeldSeed>& seed, const Frame& frame, HandAnimation* animation, Transform& hand)
{
	if (!seed.has_value())
	{
		return false;
	}
	const float handSize = HandAnimation::SizeAtDistance(glm::distance(frame.camera, hand.position));
	if (animation != nullptr)
	{
		const auto cycle = HoldCycle(seed->hold);
		const auto* clip = cycle.has_value() ? animation->GetAnimation(static_cast<size_t>(*cycle)) : nullptr;
		if (clip != nullptr)
		{
			animation->UpdateHeld(frame.dt, *cycle, HoldTimeMs(seed->hold, clip->duration, seed->reach, handSize),
			                      frame.cursor);
		}
		else
		{
			animation->UpdateHeld(frame.dt, HandAnimation::Cycle::Wiggle, 0, frame.cursor);
		}
	}

	// The hand and its seed roll with the pour and sway with the cursor about the line to the camera, at once
	const auto sway = CursorSway(animation != nullptr ? animation->GetCursorLag() : glm::vec2(0.0f));
	const auto up = HeldUp(frame.camera, hand.position, frame.tilt + sway.x, sway.y);
	const auto basis = HeldBasis(frame.levelTurn, up);
	hand.rotation = basis * frame.modelCorrection;

	auto& registry = Locator::entitiesRegistry::value();
	if (auto* transform = registry.TryGet<Transform>(seed->entity))
	{
		transform->position = hand.position - up * seed->hang;
		transform->rotation = SeedTurn(basis, seed->yRotate, frame.rightHanded);
	}
	return true;
}

void HandHoldPoser::Fade(const std::optional<HeldSeed>& seed, std::chrono::microseconds dt, Transform& hand)
{
	const auto entity = seed.has_value() ? seed->entity : entt::null;
	if (entity != _lastSeed && _drawnPosition.has_value())
	{
		_fade.Start(*_drawnPosition, _drawnRotation);
	}
	_lastSeed = entity;
	_fade.Step(std::chrono::duration<float>(dt).count(), hand.position, hand.rotation);
	_drawnPosition = hand.position;
	_drawnRotation = hand.rotation;
}

void HandHoldPoser::PlaceHandEffect(const std::optional<HeldSeed>& seed, const HandAnimation* animation, const Transform& hand,
                                    float handSize) const
{
	if (!seed.has_value() || !Locator::magicSystem::has_value())
	{
		return;
	}
	auto point = hand.position;
	if (seed->effectInFingers && animation != nullptr && animation->IsLoaded())
	{
		const auto model =
		    glm::translate(glm::mat4(1.0f), hand.position) * glm::mat4(hand.rotation) * glm::scale(glm::mat4(1.0f), hand.scale);
		point = glm::vec3(model * glm::vec4(animation->LeafBoneCentre(), 1.0f));
	}
	Locator::magicSystem::value().PlaceHandEffect(point, handSize);
}
