/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "InfluenceSystem.h"

#include <cmath>

#include <algorithm>
#include <unordered_map>

#include "3D/LandIslandInterface.h"
#include "Audio/Sound.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Influence.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/SoundTagSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// The border is drawn again only on every tenth turn, and only once a reach has moved by more than this
constexpr uint32_t k_RedrawTurns = 10;
constexpr float k_RedrawReach = 0.01f;

/// What the story gives for a land of the story, from the five lands' values and the one after them
float ForLand(const std::array<float, 5>& story, float after, int32_t land)
{
	if (land >= 1 && land <= static_cast<int32_t>(story.size()))
	{
		return story.at(static_cast<size_t>(land - 1));
	}
	return after;
}

const MapScriptGlobals& Globals()
{
	return Locator::entitiesRegistry::value().Context().mapScriptGlobals;
}

/// A building's own info, by its number and its mesh, else the first of its tribe
const GAbodeInfo* AbodeInfoOf(const Abode& abode, entt::id_type mesh, Tribe tribe)
{
	const GAbodeInfo* byTribe = nullptr;
	for (const auto& info : Locator::infoConstants::value().abode)
	{
		if (info.abodeNumber != abode.type)
		{
			continue;
		}
		if (resources::HashIdentifier(info.meshId) == mesh)
		{
			return &info;
		}
		if (byTribe == nullptr && info.tribeType == tribe)
		{
			byTribe = &info;
		}
	}
	return byTribe;
}

/// A building adds its own influence, by its size, once for itself and once for each of its people
float AbodeInfluence(entt::entity entity, const Abode& abode, Tribe tribe)
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto* info = AbodeInfoOf(abode, mesh != nullptr ? mesh->id : 0, tribe);
	if (info == nullptr)
	{
		return 0.0f;
	}
	const auto* transform = registry.TryGet<const Transform>(entity);
	const float scale = transform != nullptr ? transform->scale.x : 1.0f;
	const auto people = std::ranges::count_if(
	    abode.inhabitants, [&registry](entt::entity villager) { return registry.AnyOf<Villager>(villager); });
	return scale * info->influence * static_cast<float>(people + 1);
}

/// The first temple of each player, which is their citadel
std::array<entt::entity, static_cast<size_t>(PlayerNames::_COUNT)> Citadels()
{
	std::array<entt::entity, static_cast<size_t>(PlayerNames::_COUNT)> citadels {};
	citadels.fill(entt::null);
	Locator::entitiesRegistry::value().Each<const Temple>([&citadels](entt::entity entity, const Temple& temple) {
		const auto index = static_cast<size_t>(temple.owner);
		if (index < citadels.size() && citadels.at(index) == entt::null)
		{
			citadels.at(index) = entity;
		}
	});
	return citadels;
}

/// How far a citadel reaches: its heart's reach for the land, fixed the first time, times the land's multiplier
float CitadelReach(entt::entity temple)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* stored = registry.TryGet<const CitadelInfluence>(temple);
	if (stored == nullptr)
	{
		const auto& heart = Locator::infoConstants::value().citadelHeart;
		const auto land = Globals().landNumber;
		const float reach = land != 0 ? ForLand(heart.storyInfluence, heart.transferedDamageMultiplier, land) : heart.influence;
		stored = &registry.Assign<CitadelInfluence>(temple, reach);
	}
	return Globals().playerInfluenceMultiplier * stored->reach;
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

influence::Ground LandHeight()
{
	return [](glm::vec2 point) {
		return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
	};
}
} // namespace

void InfluenceSystem::Reset()
{
	_circles.clear();
	_ripples.clear();
	_borderShown.fill(false);
	_bordersDirty = true;
}

void InfluenceSystem::NoteReach(float reach, float drawn)
{
	if (std::fabs(reach - drawn) > k_RedrawReach)
	{
		_bordersDirty = true;
	}
}

void InfluenceSystem::ProcessTowns()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::unordered_map<uint32_t, Tribe> tribes;
	registry.Each<const Town, const Tribe>([&](const Town& town, const Tribe& tribe) { tribes.emplace(town.id, tribe); });
	std::unordered_map<uint32_t, float> buildings;
	registry.Each<const Abode>([&](entt::entity entity, const Abode& abode) {
		if (const auto tribe = tribes.find(abode.townId); tribe != tribes.end())
		{
			buildings[abode.townId] += AbodeInfluence(entity, abode, tribe->second);
		}
	});
	const auto& info = Locator::infoConstants::value().town;
	const auto land = Globals().landNumber;
	const float base = land != 0 ? ForLand(info.storyInfluence, info.maxForTimeWillWorkUntil, land) : info.influence;
	const float multiplier = Globals().townInfluenceMultiplier;
	registry.Each<const Town>([&](entt::entity entity, const Town& town) {
		auto& influence = registry.AnyOf<TownInfluence>(entity) ? registry.Get<TownInfluence>(entity)
		                                                        : registry.Assign<TownInfluence>(entity);
		influence.radius = (base + buildings[town.id]) * multiplier;
		NoteReach(influence.radius, influence.drawnRadius);
	});
}

void InfluenceSystem::ProcessCitadels()
{
	const auto citadels = Citadels();
	auto& registry = Locator::entitiesRegistry::value();
	for (size_t player = 0; player < citadels.size(); ++player)
	{
		const auto citadel = citadels.at(player);
		if (citadel == entt::null)
		{
			continue;
		}
		// A player's border shows once their citadel stands
		if (player < _borderShown.size())
		{
			_borderShown.at(player) = true;
		}
		const float reach = CitadelReach(citadel);
		NoteReach(reach, registry.Get<const CitadelInfluence>(citadel).drawnRadius);
	}
}

void InfluenceSystem::DrawBorders()
{
	_circles.clear();
	auto& registry = Locator::entitiesRegistry::value();
	const auto citadels = Citadels();
	const auto ground = LandHeight();
	// Every player's but the neutral one's: a circle about their citadel, then one about each of their towns
	for (size_t player = 0; player < static_cast<size_t>(PlayerNames::NEUTRAL); ++player)
	{
		const auto name = static_cast<PlayerNames>(player);
		if (const auto citadel = citadels.at(player); citadel != entt::null)
		{
			const float reach = CitadelReach(citadel);
			if (const auto* transform = registry.TryGet<const Transform>(citadel); transform != nullptr && reach != 0.0f)
			{
				influence::AddCircle(_circles, name, transform->position, reach, ground);
			}
			registry.Get<CitadelInfluence>(citadel).drawnRadius = reach;
		}
		registry.Each<const Town, TownInfluence, const Transform>(
		    [&](const Town& town, TownInfluence& influence, const Transform& transform) {
			    if (town.owner != name)
			    {
				    return;
			    }
			    if (influence.radius != 0.0f)
			    {
				    influence::AddCircle(_circles, name, transform.position, influence.radius, ground);
			    }
			    influence.drawnRadius = influence.radius;
		    });
		registry.Each<const InfluenceSource, const Transform>([&](const InfluenceSource& source, const Transform& transform) {
			if (source.player == name && source.radius > 0.0f)
			{
				influence::AddCircle(_circles, name, transform.position, source.radius, ground);
			}
		});
	}
	_bordersDirty = false;
}

void InfluenceSystem::ProcessTurn(uint32_t turn)
{
	ProcessTowns();
	ProcessCitadels();
	if (_bordersDirty && turn % k_RedrawTurns == 0)
	{
		DrawBorders();
	}
}

void InfluenceSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	_scrollRemainder += gameTime.count();
	const auto step = static_cast<int32_t>(_scrollRemainder);
	_scrollRemainder -= static_cast<float>(step);
	_scrollClock = (_scrollClock + step) % influence::k_ScrollWrap;

	std::erase_if(_ripples,
	              [&gameTime](influence::Ripple& ripple) { return !influence::AdvanceRipple(ripple, gameTime.count()); });

	// While the game runs, the hand crossing a border sounds once, at the hand
	if (Locator::time::value().IsPaused())
	{
		return;
	}
	if (const auto hand = HandPosition(); hand.has_value() && CrossBorders(*hand) && Locator::soundTagSystem::has_value())
	{
		Locator::soundTagSystem::value().CreatePointSound(static_cast<entt::id_type>(audio::SoundId::G_HandThroughInfluence_01),
		                                                  *hand, false);
	}
}

bool InfluenceSystem::CrossBorders(const glm::vec3& hand)
{
	std::array<bool, k_Players> inside {};
	for (const auto& circle : _circles)
	{
		if (influence::Inside(circle.centre, circle.radius, hand))
		{
			inside.at(static_cast<size_t>(circle.player)) = true;
		}
	}
	bool crossed = false;
	if (!_handSeen)
	{
		// The first time, only where it is
		_handWasInside = inside;
		_handSeen = true;
	}
	else
	{
		for (size_t player = 0; player < k_Players; ++player)
		{
			if (inside.at(player) == _handWasInside.at(player))
			{
				continue;
			}
			_handWasInside.at(player) = inside.at(player);
			// The circle whose edge the hand went over: none if the border moved under a still hand
			const auto circle = std::ranges::find_if(_circles, [&](const influence::Circle& c) {
				return static_cast<size_t>(c.player) == player &&
				       influence::Inside(c.centre, c.radius, _handBefore) != influence::Inside(c.centre, c.radius, hand);
			});
			if (circle == _circles.end() || !IsBorderShown(circle->player))
			{
				continue;
			}
			_ripples.insert(_ripples.begin(),
			                influence::MakeRipple(*circle, _handBefore, hand, LandHeight(), [](float a, float b) {
				                return Locator::gameRandom::value().CrtRandom(a, b);
			                }));
			crossed = true;
		}
	}
	_handBefore = {hand.x, 0.0f, hand.z};
	return crossed;
}

float InfluenceSystem::PlayerInfluence(PlayerNames player, const map_coords::MapCoords& position) const
{
	auto& registry = Locator::entitiesRegistry::value();
	// Each is measured from its map position, in the game's map units
	const auto distanceTo = [&position](const glm::vec3& point) {
		return gutils::GetDistanceInMetres(map_coords::FromMetres({point.x, point.z}), position);
	};
	// Under another player's shield a player has no influence at all
	bool shielded = false;
	registry.Each<const AntiInfluence, const Transform>([&](const AntiInfluence& ring, const Transform& transform) {
		shielded = shielded || (ring.owner != player && distanceTo(transform.position) < ring.radius);
	});
	if (shielded)
	{
		return 0.0f;
	}
	float sum = 0.0f;
	// The citadel's reach, where it reaches
	if (const auto citadel = Citadels().at(static_cast<size_t>(player)); citadel != entt::null)
	{
		const float reach = CitadelReach(citadel);
		if (const auto* transform = registry.TryGet<const Transform>(citadel);
		    transform != nullptr && distanceTo(transform->position) < reach)
		{
			sum = reach;
		}
	}
	// And each of the player's towns', where it reaches
	registry.Each<const Town, const TownInfluence, const Transform>(
	    [&](const Town& town, const TownInfluence& influence, const Transform& transform) {
		    if (town.owner == player && distanceTo(transform.position) < influence.radius)
		    {
			    sum += influence.radius;
		    }
	    });
	// And any other source of the player's, where it reaches
	registry.Each<const InfluenceSource, const Transform>([&](const InfluenceSource& source, const Transform& transform) {
		if (source.player == player && distanceTo(transform.position) < source.radius)
		{
			sum += source.radius;
		}
	});
	return std::clamp(sum, -1.0f, 1.0f);
}

bool InfluenceSystem::IsBorderShown(PlayerNames player) const
{
	const auto index = static_cast<size_t>(player);
	return index < _borderShown.size() && _borderShown.at(index);
}
