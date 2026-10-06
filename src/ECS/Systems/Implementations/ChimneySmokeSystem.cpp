/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ChimneySmokeSystem.h"

#include "3D/L3DMesh.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/ChimneySmoke.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using openblack::ecs::components::Abode;
using openblack::ecs::components::ChimneySmoke;
using openblack::ecs::components::Transform;

namespace
{
chimney_smoke::Random GameRandom()
{
	return [](float a, float b) { return Locator::gameRandom::value().CrtRandom(a, b); };
}

/// The player's hand in the world, if it is in it
std::optional<glm::vec3> HandPosition()
{
	if (!Locator::handSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto hand = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	const auto* transform = registry.TryGet<const Transform>(hand);
	if (transform == nullptr || transform->position == glm::vec3(0.0f))
	{
		return std::nullopt;
	}
	return transform->position;
}
} // namespace

void ChimneySmokeSystem::Attach(entt::entity abode, const graphics::L3DMesh& mesh, const Transform& transform, bool workshop)
{
	const auto& point = mesh.GetChimneyPos();
	if (!point.has_value())
	{
		return;
	}
	// The chimney's top through the home's turn and size, at its place
	const auto chimney = transform.position + (transform.rotation * (*point * transform.scale));
	Locator::entitiesRegistry::value().Assign<ChimneySmoke>(
	    abode,
	    chimney_smoke::Create(chimney, workshop ? chimney_smoke::k_WorkshopSmoke : chimney_smoke::k_HomeSmoke, GameRandom()));
}

void ChimneySmokeSystem::ProcessTurn()
{
	const auto position = HandPosition();
	if (!position)
	{
		return;
	}
	if (_lastHandPosition)
	{
		const auto millisecondsPerTurn = std::chrono::duration<float, std::milli>(TimeSystemInterface::k_TurnDuration).count();
		_handVelocity = chimney_smoke::EaseHandVelocity(_handVelocity, *position - *_lastHandPosition, millisecondsPerTurn);
	}
	_lastHandPosition = position;
}

void ChimneySmokeSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	if (const auto position = HandPosition())
	{
		_handWind = chimney_smoke::WindOf(*position, _handVelocity);
	}
	const auto random = GameRandom();
	Locator::entitiesRegistry::value().Each<const Abode, ChimneySmoke>([&](const Abode& abode, ChimneySmoke& smoke) {
		// Lit while someone is home
		if (!chimney_smoke::UpdateState(smoke, abode.presentAtHome > 0))
		{
			return;
		}
		chimney_smoke::Advance(smoke, gameTime.count(), chimney_smoke::Drift(smoke.chimney, _handWind, random));
	});
}
