/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureModeSystem.h"

#include <glm/gtx/vec_swizzle.hpp>

#include "3D/LandIslandInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "Camera/CreatureCameraModel.h"
#include "Camera/DefaultWorldCameraModel.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

namespace openblack::ecs::systems
{

namespace
{
/// The local player, whose creature C locks onto
constexpr PlayerNames k_LocalPlayer = PlayerNames::PLAYER_ONE;

bool TempleActive()
{
	return Locator::temple::has_value() && Locator::temple::value().Active();
}
} // namespace

CreatureModeSystem::CreatureModeSystem() = default;
CreatureModeSystem::~CreatureModeSystem() = default;

std::optional<entt::entity> CreatureModeSystem::PlayersCreature() const
{
	// The player's creature as the leash knows it: the one held on its leash, or its first
	if (!Locator::leashSystem::has_value())
	{
		return std::nullopt;
	}
	return Locator::leashSystem::value().PlayersCreature(k_LocalPlayer);
}

std::optional<creature_follow::View> CreatureModeSystem::GetView() const
{
	return OwnsCamera() ? std::optional(_model->GetView()) : std::nullopt;
}

bool CreatureModeSystem::OwnsCamera() const
{
	return _model != nullptr && Locator::camera::has_value() && &Locator::camera::value().GetModel() == _model;
}

void CreatureModeSystem::ReleaseCamera()
{
	if (OwnsCamera() && _playerModel != nullptr)
	{
		Locator::camera::value().SetModel(std::move(_playerModel));
	}
	_model = nullptr;
	_playerModel.reset();
	_leaving = false;
	_held = {};
}

bool CreatureModeSystem::StillValid() const
{
	if (!_creature.has_value() || !Locator::entitiesRegistry::has_value())
	{
		return false;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(*_creature) || !registry.AllOf<components::Creature, components::Transform>(*_creature))
	{
		return false;
	}
	// A script's cinema bars take the camera
	return !Locator::cinematicDirectorSystem::has_value() || Locator::cinematicDirectorSystem::value().IsInterfaceActive();
}

bool CreatureModeSystem::Enter(entt::entity creature)
{
	if (!Locator::camera::has_value() || !Locator::entitiesRegistry::has_value() || TempleActive())
	{
		return false;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(creature) || !registry.AllOf<components::Creature, components::Transform>(creature))
	{
		return false;
	}
	auto& camera = Locator::camera::value();
	if (OwnsCamera())
	{
		// Already locked on, it moves over to the other creature from where the camera is
		ReleaseCamera();
	}
	// Only the player's own camera is taken over: not the temple's, nor the editor's while it follows what it picked
	if (dynamic_cast<DefaultWorldCameraModel*>(&camera.GetModel()) == nullptr)
	{
		_creature.reset();
		return false;
	}
	const auto& body = registry.Get<const components::Creature>(creature);
	const auto& transform = registry.Get<const components::Transform>(creature);
	auto model = std::make_unique<CreatureCameraModel>(camera.GetOrigin(), camera.GetFocus(), transform.position,
	                                                   creature_mode::CreatureHeight(body.size));
	_model = model.get();
	_playerModel = camera.SetModel(std::move(model));
	_creature = creature;
	_leaving = false;
	// The button that double clicked is still down: only a grip of the land after this one gives the camera back
	_wasGripping = true;
	return true;
}

void CreatureModeSystem::Leave()
{
	_creature.reset();
	if (_model == nullptr)
	{
		return;
	}
	if (OwnsCamera())
	{
		ReleaseCamera();
		return;
	}
	// The temple or the editor took the camera from this mode's: it is given back once they return it
	_leaving = true;
}

void CreatureModeSystem::PressCreatureKey()
{
	const auto yours = PlayersCreature();
	switch (creature_mode::OnCreatureKey(_creature, yours))
	{
	case creature_mode::KeyAction::Enter:
		Enter(*yours);
		break;
	case creature_mode::KeyAction::Leave:
		Leave();
		break;
	case creature_mode::KeyAction::None:
		break;
	}
}

bool CreatureModeSystem::Press(const creature_mode::Press& press)
{
	const auto creature = _doubleClicks.OnPress(press);
	return creature.has_value() && Enter(*creature);
}

void CreatureModeSystem::HoldKeys(int across, int along, bool shift, bool ctrl, float seconds)
{
	_held = {.across = across, .along = along, .shift = shift, .ctrl = ctrl, .seconds = seconds};
}

void CreatureModeSystem::ClearView()
{
	if (!OwnsCamera() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto& land = Locator::terrainSystem::value();
	const auto focus = _model->GetTargetFocus();
	_model->ClearView(land.GetNormalAt(glm::xz(focus)), [&land](glm::vec2 point) { return land.GetHeightAt(point); });
}

void CreatureModeSystem::Update(std::chrono::microseconds dt, const Frame& frame)
{
	if (!Locator::camera::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	// The camera was handed back to this mode after leaving was asked for
	if (_leaving && OwnsCamera())
	{
		ReleaseCamera();
	}

	// C locks onto the player's creature, and lets go of it again
	if (Locator::gameActionSystem::has_value() && !TempleActive())
	{
		using input::BindableActionMap;
		const auto& actions = Locator::gameActionSystem::value();
		if (actions.Get(BindableActionMap::ZOOM_TO_CREATURE) && actions.GetChanged(BindableActionMap::ZOOM_TO_CREATURE))
		{
			PressCreatureKey();
		}
	}

	if (!_creature.has_value())
	{
		return;
	}
	// The temple and the editor take the camera, and the camera stays with them until they hand it back
	if (TempleActive())
	{
		Leave();
		return;
	}
	if (!OwnsCamera())
	{
		return;
	}
	// The cursor keys alone, or the hand taking a fresh grip of the land, give the player the camera back
	const bool gripped = frame.handGripping && !_wasGripping;
	_wasGripping = frame.handGripping;
	if (!StillValid() || _model->WantsToLeave() || gripped)
	{
		Leave();
		return;
	}

	const auto& registry = Locator::entitiesRegistry::value();
	const auto& body = registry.Get<const components::Creature>(*_creature);
	const auto& transform = registry.Get<const components::Transform>(*_creature);
	_model->SetTarget(transform.position, creature_mode::CreatureHeight(body.size));

	if (_held.seconds > 0.0f)
	{
		const auto seconds = std::min(std::chrono::duration<float>(dt).count(), _held.seconds);
		_held.seconds -= seconds;
		auto keys = creature_follow::KeysFor(_held.across, _held.along, seconds);
		keys.shift = _held.shift;
		keys.ctrl = _held.ctrl;
		const auto width = Locator::windowing::has_value() ? static_cast<float>(Locator::windowing::value().GetSize().x) : 1.0f;
		if (_model->Turn(keys, width) == creature_follow::KeyOutcome::Leave)
		{
			Leave();
		}
	}
}

} // namespace openblack::ecs::systems
