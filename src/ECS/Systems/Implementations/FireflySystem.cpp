/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "FireflySystem.h"

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <spdlog/spdlog.h>

#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Firefly.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/TempleExterior.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "GameFireflyWorld.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The game's frames count time in milliseconds; the fireflies' loops turn by seconds
constexpr float k_SecondsPerMillisecond = 0.001f;

/// Calls a function with each kind of component that makes a thing stand over several cells of the map
template <typename Func>
void ForEachFixedKind(Func&& func)
{
	func.template operator()<Abode>();
	func.template operator()<Field>();
	func.template operator()<SpellDispenser>();
	func.template operator()<Feature>();
	func.template operator()<Flowers>();
	func.template operator()<AnimatedStatic>();
	func.template operator()<BigForest>();
	func.template operator()<MobileStatic>();
	func.template operator()<DeadTree>();
	func.template operator()<TeleportStone>();
	func.template operator()<TempleExterior>();
}

/// The thing at a place counting from the newest, in a list kept oldest first
[[nodiscard]] size_t FromNewest(size_t count, size_t place)
{
	return count - 1 - place;
}
} // namespace

FireflySystem::FireflySystem()
    : FireflySystem(std::make_unique<GameFireflyWorld>())
{
}

FireflySystem::FireflySystem(std::unique_ptr<fireflies::FireflyWorldInterface> world)
    : _world(std::move(world))
{
	auto& registry = _world->Entities();
	_connections.emplace_back(registry.OnConstruct<Tree>().connect<&FireflySystem::OnTreeMade>(*this));
	_connections.emplace_back(registry.OnDestroy<Tree>().connect<&FireflySystem::OnTreeGone>(*this));
	_connections.emplace_back(registry.OnDestroy<Firefly>().connect<&FireflySystem::OnFireflyGone>(*this));
	ForEachFixedKind([this, &registry]<typename Component>() {
		_connections.emplace_back(registry.OnConstruct<Component>().template connect<&FireflySystem::OnFixedMade>(*this));
		_connections.emplace_back(registry.OnDestroy<Component>().template connect<&FireflySystem::OnFixedGone>(*this));
	});
}

FireflySystem::~FireflySystem() = default;

void FireflySystem::OnTreeMade(entt::registry& /*registry*/, entt::entity entity)
{
	_trees.push_back(entity);
}

void FireflySystem::OnTreeGone(entt::registry& /*registry*/, entt::entity entity)
{
	std::erase(_trees, entity);
}

void FireflySystem::OnFixedMade(entt::registry& /*registry*/, entt::entity entity)
{
	if (std::ranges::find(_fixed, entity) == _fixed.end())
	{
		_fixed.push_back(entity);
	}
}

void FireflySystem::OnFixedGone(entt::registry& /*registry*/, entt::entity entity)
{
	std::erase(_fixed, entity);
}

void FireflySystem::OnFireflyGone(entt::registry& /*registry*/, entt::entity entity)
{
	std::erase(_order, entity);
}

entt::entity FireflySystem::Create(const map_coords::MapCoords& spot)
{
	auto& registry = _world->Entities();
	const auto drift = fireflies::DrawDrift([this](float x) { return _world->GameFloatRand(x); });
	const auto entity = registry.Create();
	registry.Assign<Firefly>(entity, fireflies::Make(spot, drift));
	registry.Assign<Transform>(entity, _world->ToWorld(spot), glm::mat3(1.0f),
	                           glm::vec3(fireflies::k_SpriteHalfSize, fireflies::k_SpriteHalfSize, 1.0f));
	_order.insert(_order.begin(), entity);
	return entity;
}

void FireflySystem::ProcessTurn()
{
	switch (fireflies::SendingAt(_world->VisualHour(), _world->SkyHours()))
	{
	case fireflies::Sending::Out:
		if (_topUpDue)
		{
			TopUp();
			if (const auto logger = spdlog::get("game"))
			{
				SPDLOG_LOGGER_DEBUG(logger, "Fireflies: nightfall, {} on the land", _order.size());
			}
		}
		SendOutOnce();
		_topUpDue = false;
		break;
	case fireflies::Sending::Home:
		SendHomeOnce();
		_topUpDue = true;
		break;
	case fireflies::Sending::Nothing:
		break;
	}
	for (const auto firefly : std::vector(_order))
	{
		Fly(firefly);
	}
}

void FireflySystem::TopUp()
{
	for (size_t count = _order.size(); count < _most; ++count)
	{
		// A coin decides between a rock and a tree. Either way a land with none of the kind ends the top up
		if (_world->GameRand(2) == 0)
		{
			if (_fixed.empty())
			{
				return;
			}
			// A random place in the list of things standing over several cells, then the first rock from there on
			const auto place = _world->GameRand(static_cast<uint32_t>(_fixed.size()));
			for (auto i = static_cast<int64_t>(FromNewest(_fixed.size(), place)); i >= 0; --i)
			{
				const auto thing = _fixed.at(static_cast<size_t>(i));
				if (_world->IsRock(thing))
				{
					if (const auto spot = _world->SpotOf(thing))
					{
						Create(*spot);
					}
					break;
				}
			}
		}
		else
		{
			if (_trees.empty())
			{
				return;
			}
			const auto place = _world->GameRand(static_cast<uint32_t>(_trees.size()));
			if (const auto spot = _world->SpotOf(_trees.at(FromNewest(_trees.size(), place))))
			{
				Create(*spot);
			}
		}
	}
}

void FireflySystem::SendOutOnce()
{
	auto& registry = _world->Entities();
	if (_order.empty() || !registry.Get<const Firefly>(_order.front()).hidden)
	{
		return;
	}
	const auto first = _order.front();
	const auto at = registry.Get<const Firefly>(first).at;
	// It comes out only from a tree or rock still standing exactly where it hides, and only if no other firefly shares
	// the spot; otherwise it is gone
	const auto things = _world->ThingsInCell(map_coords::Cell(at));
	const bool hasHome = std::ranges::any_of(things, [this, &at](entt::entity thing) {
		const auto spot = _world->SpotOf(thing);
		return _world->IsHidingPlace(thing) && spot.has_value() && *spot == at;
	});
	const bool shared = std::ranges::any_of(_order, [&registry, first, &at](entt::entity other) {
		return other != first && registry.Get<const Firefly>(other).at == at;
	});
	if (!hasHome || shared)
	{
		registry.Destroy(first);
		return;
	}
	const auto hover = PlaceToHover(at);
	auto& firefly = registry.Get<Firefly>(first);
	fireflies::FlyOut(firefly, hover, gutils::GetDistanceInMetres(hover, firefly.home), _world->ToWorld(firefly.home));
	firefly.hidden = false;
	ToTheBack();
}

void FireflySystem::SendHomeOnce()
{
	auto& registry = _world->Entities();
	if (_order.empty() || registry.Get<const Firefly>(_order.front()).hidden)
	{
		return;
	}
	auto& firefly = registry.Get<Firefly>(_order.front());
	// It looks for somewhere to hide from wherever it is now
	const auto home = PlaceToHide(firefly.at);
	fireflies::FlyHome(firefly, home, gutils::GetDistanceInMetres(firefly.hover, home));
	firefly.hidden = true;
	ToTheBack();
}

void FireflySystem::ToTheBack()
{
	std::rotate(_order.begin(), _order.begin() + 1, _order.end());
}

void FireflySystem::Fly(entt::entity entity)
{
	auto& firefly = _world->Entities().Get<Firefly>(entity);
	const bool out = firefly.state == fireflies::State::FlyingOut;
	const auto step = fireflies::Advance(firefly, _world->TurnSeconds());
	if (step.moveTo.has_value())
	{
		firefly.at = *step.moveTo;
	}
	else if (step.along.has_value())
	{
		// The way along is taken in the world, between the two ends as they stand on the land
		const auto from = _world->ToWorld(out ? firefly.home : firefly.hover);
		const auto to = _world->ToWorld(out ? firefly.hover : firefly.home);
		firefly.at = _world->FromWorld((to - from) * *step.along + from);
	}
}

std::optional<fireflies::Found> FireflySystem::Search(const map_coords::MapCoords& from, bool building)
{
	return fireflies::SearchRoughlyNearest(
	    from, fireflies::k_SearchReach,
	    [this, building](glm::ivec2 cell, auto&& meet) {
		    for (const auto thing : _world->ThingsInCell(cell))
		    {
			    if (!(building ? _world->IsBuilding(thing) : _world->IsHidingPlace(thing)))
			    {
				    continue;
			    }
			    if (const auto spot = _world->SpotOf(thing); spot.has_value() && meet(thing, *spot))
			    {
				    return;
			    }
		    }
	    },
	    [this](uint32_t n) { return _world->GameRand(n); });
}

map_coords::MapCoords FireflySystem::PlaceToHover(const map_coords::MapCoords& from)
{
	if (const auto found = Search(from, true))
	{
		auto spot = found->spot;
		spot.altitude = _world->HeightOf(found->thing) + fireflies::k_AboveBuilding + spot.altitude;
		return spot;
	}
	return fireflies::NowhereFrom(from, fireflies::k_NowhereHoverHeight);
}

map_coords::MapCoords FireflySystem::PlaceToHide(const map_coords::MapCoords& from)
{
	if (const auto found = Search(from, false))
	{
		return found->spot;
	}
	return fireflies::NowhereFrom(from, 0.0f);
}

void FireflySystem::Update(float milliseconds, float turnFraction)
{
	auto& registry = _world->Entities();
	const auto look = _world->Look();
	const auto camera = _world->CameraPosition();
	for (const auto entity : _order)
	{
		auto& firefly = registry.Get<Firefly>(entity);
		std::optional<uint8_t> opacity;
		// Only one out of hiding and in view is drawn; how far it is from the camera is taken from where it was last drawn
		if (firefly.state != fireflies::State::Resting && _world->InView(_world->ToWorld(firefly.at)))
		{
			const auto away = camera - firefly.drawn;
			opacity = fireflies::OpacityAt(glm::dot(away, away));
		}
		if (!opacity.has_value() || !look.has_value())
		{
			if (registry.AllOf<Sprite>(entity))
			{
				registry.Remove<Sprite>(entity);
			}
			continue;
		}
		// Its loops turn only while it is drawn, about a place between its last two turns' places
		firefly.clock = milliseconds * k_SecondsPerMillisecond + firefly.clock;
		firefly.amplitude = fireflies::Amplitude(firefly.state, firefly.progress);
		const auto offset = fireflies::DriftOffset(firefly.drift, firefly.clock, firefly.amplitude);
		const auto previous = _world->ToWorld(firefly.previous);
		const auto now = _world->ToWorld(firefly.at);
		firefly.drawn = offset + ((now - previous) * turnFraction + previous);
		registry.Get<Transform>(entity).position = firefly.drawn;
		auto* sprite = registry.TryGet<Sprite>(entity);
		if (sprite == nullptr)
		{
			sprite = &registry.Assign<Sprite>(entity, *look);
		}
		sprite->tint.a = static_cast<float>(*opacity) / 255.0f;
	}
}

bool FireflySystem::Catch(const map_coords::MapCoords& spot)
{
	auto& registry = _world->Entities();
	const auto found = std::ranges::find_if(
	    _order, [&registry, &spot](entt::entity entity) { return registry.Get<const Firefly>(entity).at == spot; });
	if (found == _order.end())
	{
		return false;
	}
	registry.Destroy(*found);
	// The land's table draws the reward: a roll of nothing, or the first kind, gives nothing
	const auto kind = _rewards.Pick(_world->GameFloatRand(_rewards.Total()));
	if (kind.has_value())
	{
		_world->MakeReward(*kind, spot);
	}
	if (const auto logger = spdlog::get("game"))
	{
		SPDLOG_LOGGER_DEBUG(logger, "Fireflies: one caught, giving magic type {}",
		                    kind.has_value() ? static_cast<int>(*kind) : -1);
	}
	return true;
}

void FireflySystem::SetRewardWeight(std::string_view magicName, float weight)
{
	if (const auto kind = _world->MagicKindNamed(magicName))
	{
		_rewards.SetWeight(*kind, weight);
	}
}

void FireflySystem::Reset()
{
	auto& registry = _world->Entities();
	for (const auto entity : std::vector(_order))
	{
		registry.Destroy(entity);
	}
	_order.clear();
	_rewards.ClearWeights();
	_topUpDue = true;
}
