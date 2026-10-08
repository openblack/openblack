/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// A tribe's power behind a miracle, as its name spinning in the world (see Magic/TribalPowerSpin.h): round the hand of
// this computer's player while the miracle is held, rising from where it was cast once it is, in the player's colour,
// written in the game's font.

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <string>

#include <fmt/format.h>
#include <glm/glm.hpp>

#include "Camera/Camera.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Gui/GameInterface.h"
#include "Locator.h"
#include "MiracleFxSystem.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using magic::tribal_spin::Runner;

namespace
{
/// A player's colour, fully opaque
glm::u8vec4 ColourOf(PlayerNames player)
{
	const auto& colours = ecs::components::Player::k_Colours;
	const uint32_t argb = colours.at(static_cast<size_t>(player) & (colours.size() - 1));
	return {static_cast<uint8_t>((argb >> 16) & 0xFF), static_cast<uint8_t>((argb >> 8) & 0xFF),
	        static_cast<uint8_t>(argb & 0xFF), 0xFF};
}

/// This computer's player
constexpr auto k_LocalPlayer = PlayerNames::PLAYER_ONE;
} // namespace

std::u16string MiracleFxSystem::TribeName(Tribe tribe) const
{
	if (_interface == nullptr || tribe == Tribe::NONE)
	{
		return {};
	}
	// HELP_TEXT_TRIBEPOWER_01 ("Celtic Power") to _09, by tribe
	return std::u16string(_interface->GetTexts().Get(fmt::format("HELP_TEXT_TRIBEPOWER_{:02}", static_cast<int>(tribe) + 1)));
}

void MiracleFxSystem::StartTribalPowerRing(Tribe tribe)
{
	StopTribalPowerRing();
	auto runner = std::make_unique<Runner>(TribeName(tribe), glm::vec3(0.0f), ColourOf(k_LocalPlayer), true);
	_ring = runner.get();
	_runners.push_back(std::move(runner));
}

void MiracleFxSystem::StopTribalPowerRing()
{
	if (_ring != nullptr)
	{
		std::erase_if(_runners, [this](const auto& runner) { return runner.get() == _ring; });
		_ring = nullptr;
	}
}

void MiracleFxSystem::ReleaseTribalPowerRing(Tribe tribe, glm::vec3 handPosition)
{
	if (_ring != nullptr)
	{
		_ring->Release(handPosition);
		_ring = nullptr;
		return;
	}
	_runners.push_back(std::make_unique<Runner>(TribeName(tribe), handPosition, ColourOf(k_LocalPlayer), false));
}

void MiracleFxSystem::TribalPowerColumn(Tribe tribe, glm::vec3 position, PlayerNames player)
{
	_runners.push_back(std::make_unique<Runner>(TribeName(tribe), position, ColourOf(player), false));
}

void MiracleFxSystem::UpdateTribalPower(float gameSeconds)
{
	if (_runners.empty())
	{
		return;
	}
	Runner::Frame frame;
	if (Locator::camera::has_value())
	{
		frame.camera = Locator::camera::value().GetOrigin();
		frame.cameraFocus = Locator::camera::value().GetFocus();
	}
	// The hand as it is drawn: where it is, and its model's z axis with its size
	const auto hand = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	if (const auto* transform = Locator::entitiesRegistry::value().TryGet<const ecs::components::Transform>(hand))
	{
		frame.handPosition = transform->position;
		frame.handZ = transform->rotation[2] * transform->scale.z;
	}
	std::erase_if(_runners, [&](auto& runner) {
		if (runner->Update(frame, gameSeconds))
		{
			return false;
		}
		if (runner.get() == _ring)
		{
			_ring = nullptr;
		}
		return true;
	});
}

std::vector<OrientedTextVertex> MiracleFxSystem::GetTribalPowerText() const
{
	std::vector<OrientedTextVertex> vertices;
	if (_interface == nullptr)
	{
		return vertices;
	}
	const auto& font = _interface->GetFont();
	for (const auto& runner : _runners)
	{
		const auto colour = runner->Colour();
		for (const auto& letter : runner->GetSpin().Letters())
		{
			const TextFrame frame {.origin = letter.origin, .axes = letter.axes};
			AppendOrientedText(vertices, font, frame, 0, letter.down, std::u16string_view(&letter.character, 1),
			                   glm::vec3(0.0f), letter.size, 1.0f, {colour.r, colour.g, colour.b, letter.alpha});
		}
	}
	return vertices;
}

const graphics::Texture2D* MiracleFxSystem::GetTextTexture() const
{
	return _interface != nullptr ? &_interface->GetFontTexture() : nullptr;
}
