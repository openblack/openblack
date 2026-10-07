/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "HandSystem.h"

#include <L3DFile.h>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>

#include "3D/HandMorph.h"
#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Components/HandMorph.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

bool HandSystem::Initialize() noexcept
{
	_hands[static_cast<size_t>(Side::Left)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, false);
	_hands[static_cast<size_t>(Side::Right)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, true);

	return false;
}

std::array<entt::entity, static_cast<size_t>(HandSystemInterface::Side::_Count)> HandSystem::GetPlayerHands() const noexcept
{
	return _hands;
}

std::array<std::optional<glm::vec3>, static_cast<size_t>(HandSystemInterface::Side::_Count)>
HandSystem::GetPlayerHandPositions() const noexcept
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto hands = GetPlayerHands();
	std::array<std::optional<glm::vec3>, static_cast<size_t>(Side::_Count)> result = {
	    registry.Get<Transform>(hands[static_cast<size_t>(Side::Left)]).position,
	    registry.Get<Transform>(hands[static_cast<size_t>(Side::Right)]).position,
	};
	// TODO(#693): Hand Getter should return an optional if the hand doesn't have a valid position
	// When the position is zero, it probably means it's not on the map (e.g. mouse is in the sky)
	if (result[static_cast<size_t>(Side::Left)] == glm::zero<glm::vec3>())
	{
		result[static_cast<size_t>(Side::Left)] = std::nullopt;
	}
	if (result[static_cast<size_t>(Side::Right)] == glm::zero<glm::vec3>())
	{
		result[static_cast<size_t>(Side::Right)] = std::nullopt;
	}
	return result;
}

namespace
{
std::span<const uint16_t> TexelsOf(const l3d::L3DTexture& skin)
{
	return {reinterpret_cast<const uint16_t*>(skin.texels.data()), skin.texels.size()};
}

/// The hand's skins drawn at an alignment: each of the base mesh's blended towards the one at the same place in the
/// evil or good mesh's list, the base's own where that mesh has fewer
std::vector<HandMorph::Skin> BlendSkins(const l3d::L3DFile& base, const l3d::L3DFile& look, float drawn)
{
	std::vector<HandMorph::Skin> skins;
	const auto& baseSkins = base.GetSkins();
	const auto& lookSkins = look.GetSkins();
	skins.reserve(baseSkins.size());
	for (size_t i = 0; i < baseSkins.size(); ++i)
	{
		const auto from = TexelsOf(baseSkins[i]);
		auto& skin = skins.emplace_back(HandMorph::Skin {.id = baseSkins[i].id, .texels = {}});
		skin.texels.resize(from.size());
		const auto to = i < lookSkins.size() ? TexelsOf(lookSkins[i]) : from;
		hand_morph::BlendSkin(from, to, drawn, skin.texels);
	}
	return skins;
}
} // namespace

void HandSystem::UpdateAlignmentMorph(std::optional<map_coords::MapCoords> picked, bool holdPoint)
{
	if (!Locator::alignmentSystem::has_value())
	{
		return;
	}
	const auto alignment = Locator::alignmentSystem::value().GetPlayerAlignment(PlayerNames::PLAYER_ONE);
	auto& files = Locator::resources::value().GetL3DFiles();
	const auto fileOf = [&files](size_t index) -> const l3d::L3DFile* {
		const auto id = HandMorph::k_SkinFileIds.at(index);
		return files.Contains(id) ? &*files.Handle(id) : nullptr;
	};
	auto& registry = Locator::entitiesRegistry::value();
	for (const auto entity : _hands)
	{
		auto* morph = registry.TryGet<HandMorph>(entity);
		if (morph == nullptr)
		{
			continue;
		}
		// The frame goes by the influence tested after the last one, then tests it again for the next
		const auto change = hand_morph::Advance(morph->state, alignment, morph->pointInInfluence);
		morph->point = hand_morph::NextPoint(morph->point, picked, holdPoint);
		if (Locator::influenceSystem::has_value())
		{
			morph->pointInInfluence =
			    Locator::influenceSystem::value().PlayerInfluence(PlayerNames::PLAYER_ONE, morph->point) > 0.0f;
		}
		if (change.shape)
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "The hand is drawn anew for alignment {:.3f}", morph->state.drawn);
		}
		if (!change.skin)
		{
			continue;
		}
		const auto* base = fileOf(0);
		const auto* look = fileOf(hand_morph::LookOf(*change.skin) == hand_morph::Look::Evil ? 1 : 2);
		morph->skins = base != nullptr ? BlendSkins(*base, look != nullptr ? *look : *base, *change.skin)
		                               : std::vector<HandMorph::Skin> {};
		++morph->revision;
	}
}
