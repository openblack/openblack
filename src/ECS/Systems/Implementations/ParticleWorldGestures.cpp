/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// What the gesture effects ask of the world: the trails of recognised gestures, the sheets of light along them, the
// hand's glow as they flash, and the chain behind the gesturing hand stepped every frame

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>

#include "ECS/Components/HandGlow.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Locator.h"
#include "ParticleSystem.h"
#include "Particles/GestureTrail.h"
#include "Particles/LightSheet.h"

using namespace openblack;
using namespace openblack::ecs::systems;

std::shared_ptr<particles::GestureTrail> GameParticleWorld::TakeGestureTrail()
{
	if (_gestureTrails.empty())
	{
		return nullptr;
	}
	auto trail = std::move(_gestureTrails.back());
	_gestureTrails.pop_back();
	return trail;
}

void GameParticleWorld::QueueGestureTrail(std::shared_ptr<particles::GestureTrail> trail)
{
	_gestureTrails.push_back(std::move(trail));
}

void GameParticleWorld::AddLightSheet(const std::shared_ptr<particles::LightSheet>& sheet)
{
	_lightSheets.push_back(sheet);
}

std::vector<std::shared_ptr<particles::LightSheet>> GameParticleWorld::LightSheets() const
{
	std::erase_if(_lightSheets, [](const auto& sheet) { return sheet.expired(); });
	std::vector<std::shared_ptr<particles::LightSheet>> sheets;
	sheets.reserve(_lightSheets.size());
	for (const auto& sheet : _lightSheets)
	{
		sheets.push_back(sheet.lock());
	}
	return sheets;
}

void GameParticleWorld::SetHandGlow(uint32_t rgb)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::handSystem::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto hand = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	if (!registry.Valid(hand))
	{
		return;
	}
	if (auto* glow = registry.TryGet<ecs::components::HandGlow>(hand))
	{
		glow->rgb = rgb & 0xFFFFFFu;
	}
	else if (rgb != 0)
	{
		registry.Assign<ecs::components::HandGlow>(hand, rgb & 0xFFFFFFu);
	}
}

void ParticleSystem::AddGestureTrail(std::shared_ptr<particles::GestureTrail> trail)
{
	if (trail != nullptr)
	{
		_world.QueueGestureTrail(std::move(trail));
	}
}

void ParticleSystem::KeepGestureTrails()
{
	if (IsRunning(_gestureTrails))
	{
		return;
	}
	// This computer's own: it draws on its own random numbers, in its player's colour
	_gestureTrails = Start(ParticleType::Gesture, glm::vec3(0.0f), 1.0f, false);
	SetPlayer(_gestureTrails, static_cast<int>(PlayerNames::PLAYER_ONE));
}

void ParticleSystem::UpdateFrame(float gameSeconds, const HandFrame& hand)
{
	if (_paused)
	{
		return;
	}
	for (const auto& sheet : _world.LightSheets())
	{
		sheet->Update(gameSeconds);
	}

	if (!IsRunning(_gestureChain))
	{
		_gestureChain = Start(ParticleType::GestureLocal, glm::vec3(0.0f), 1.0f, false);
		SetPlayer(_gestureChain, static_cast<int>(PlayerNames::PLAYER_ONE));
		if (const auto it = FindRunning(_gestureChain); it != _effects.end())
		{
			it->everyFrame = true;
		}
	}
	if (gameSeconds <= 0.0f)
	{
		return;
	}
	const auto it = FindRunning(_gestureChain);
	if (it == _effects.end())
	{
		return;
	}
	// While gesturing, the chain is sized by how far the hand is from the camera as well as by the hand
	const float distanceScale =
	    hand.gesturing ? particles::gesture_trail::ChainDistanceScale(glm::distance(hand.position, hand.cameraPosition)) : 1.0f;
	it->effect->SetMagnitude(hand.size * distanceScale);
	it->effect->SetProcessInfo({.handPosition = hand.position, .enabled = hand.gesturing});
	if (StepEffect(*it->effect, gameSeconds))
	{
		_effects.erase(FindRunning(_gestureChain));
	}
}

void ParticleSystem::AddLightSheets(particles::draw::Frame& frame) const
{
	std::vector<particles::LightSheet::Vertex> vertices;
	std::vector<uint32_t> triangles;
	for (const auto& sheet : _world.LightSheets())
	{
		// It takes its place by where its middle was when it was last drawn
		const auto sortPoint = sheet->Middle();
		sheet->Build(vertices, triangles);
		particles::draw::AddLightSheet(frame, vertices, triangles, sortPoint);
	}
}
