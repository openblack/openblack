/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstddef>
#include <cstdint>

#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "3D/MapCoords.h"
#include "ECS/Components/Firefly.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Nature/Fireflies.h"
#include "Nature/FireflyWorld.h"

#define LOCATOR_IMPLEMENTATIONS
#include "ECS/Systems/Implementations/FireflySystem.h"

using namespace openblack;
using namespace openblack::fireflies;
using openblack::ecs::components::Firefly;

namespace
{
/// The hours of full night, the start and end of dusk, and full day, as the land's cycle sets them by default
constexpr std::array<float, 4> k_Hours {4.5f, 7.0f, 7.5f, 8.25f};
constexpr float k_Evening = 18.0f;
constexpr float k_Morning = 10.0f;
constexpr size_t k_Heal = 10;
constexpr size_t k_Fire = 1;

/// A land of its own: flat ground at height 0, its things kept by spot, a clock set by the test, random numbers that
/// always toss heads (1) and give half of what is asked
class FakeWorld final: public FireflyWorldInterface
{
public:
	enum class Kind : uint8_t
	{
		Tree,
		Rock,
		Building,
	};
	struct Thing
	{
		Kind kind;
		map_coords::MapCoords spot;
		float height;
	};

	ecs::Registry registry;
	std::unordered_map<entt::entity, Thing> things;
	std::vector<std::pair<size_t, map_coords::MapCoords>> rewards;
	float hour {k_Evening};
	glm::vec3 camera {0.0f};
	std::optional<ecs::components::Sprite> look;

	entt::entity Add(Kind kind, glm::vec2 metres, float height = 0.0f)
	{
		const auto entity = registry.Create();
		if (kind == Kind::Tree)
		{
			registry.Assign<ecs::components::Tree>(entity, TreeInfo::Oak, 1.0f);
		}
		else if (kind == Kind::Rock)
		{
			registry.Assign<ecs::components::MobileStatic>(entity, MobileStaticInfo::Rock);
		}
		things.emplace(entity, Thing {.kind = kind, .spot = map_coords::FromMetres(metres), .height = height});
		return entity;
	}
	void Remove(entt::entity thing)
	{
		things.erase(thing);
		registry.Destroy(thing);
	}

	ecs::Registry& Entities() override { return registry; }
	uint32_t GameRand(uint32_t n) override { return n > 1 ? 1 : 0; }
	float GameFloatRand(float x) override { return x * 0.5f; }
	[[nodiscard]] float VisualHour() const override { return hour; }
	[[nodiscard]] std::array<float, 4> SkyHours() const override { return k_Hours; }
	[[nodiscard]] float TurnSeconds() const override { return 0.1f; }
	[[nodiscard]] glm::vec3 ToWorld(const map_coords::MapCoords& coords) const override
	{
		const auto metres = map_coords::ToMetres(coords);
		return {metres.x, coords.altitude, metres.y};
	}
	[[nodiscard]] map_coords::MapCoords FromWorld(glm::vec3 point) const override
	{
		auto coords = map_coords::FromMetres({point.x, point.z});
		coords.altitude = point.y;
		return coords;
	}
	[[nodiscard]] std::vector<entt::entity> ThingsInCell(glm::ivec2 cell) const override
	{
		std::vector<entt::entity> found;
		for (const auto& [entity, thing] : things)
		{
			if (map_coords::Cell(thing.spot) == cell)
			{
				found.push_back(entity);
			}
		}
		return found;
	}
	[[nodiscard]] std::optional<map_coords::MapCoords> SpotOf(entt::entity thing) const override
	{
		const auto found = things.find(thing);
		return found == things.end() ? std::nullopt : std::optional(found->second.spot);
	}
	[[nodiscard]] bool IsHidingPlace(entt::entity thing) const override
	{
		const auto found = things.find(thing);
		return found != things.end() && found->second.kind != Kind::Building;
	}
	[[nodiscard]] bool IsRock(entt::entity thing) const override
	{
		const auto found = things.find(thing);
		return found != things.end() && found->second.kind == Kind::Rock;
	}
	[[nodiscard]] bool IsBuilding(entt::entity thing) const override
	{
		const auto found = things.find(thing);
		return found != things.end() && found->second.kind == Kind::Building;
	}
	[[nodiscard]] float HeightOf(entt::entity thing) const override { return things.at(thing).height; }
	[[nodiscard]] std::optional<size_t> MagicKindNamed(std::string_view name) const override
	{
		if (name == "HEAL")
		{
			return k_Heal;
		}
		if (name == "FIRE")
		{
			return k_Fire;
		}
		return std::nullopt;
	}
	void MakeReward(size_t kind, const map_coords::MapCoords& spot) override { rewards.emplace_back(kind, spot); }
	[[nodiscard]] glm::vec3 CameraPosition() const override { return camera; }
	[[nodiscard]] bool InView(glm::vec3 /*point*/) const override { return true; }
	[[nodiscard]] std::optional<ecs::components::Sprite> Look() const override { return look; }
};

/// Where the village stands, well inside the map
constexpr glm::vec2 k_Here {2000.0f, 2000.0f};

class Fireflies: public ::testing::Test
{
protected:
	void SetUp() override
	{
		auto world = std::make_unique<FakeWorld>();
		_world = world.get();
		_system = std::make_unique<ecs::systems::FireflySystem>(std::move(world));
	}

	[[nodiscard]] const Firefly& Only() const
	{
		EXPECT_EQ(_system->GetFireflies().size(), 1u);
		return _world->registry.Get<const Firefly>(_system->GetFireflies().front());
	}

	void Turns(int count)
	{
		for (int i = 0; i < count; ++i)
		{
			_system->ProcessTurn();
		}
	}

	FakeWorld* _world {nullptr};
	std::unique_ptr<ecs::systems::FireflySystem> _system;
};
} // namespace

TEST(FireflyRules, TheEveningSendsThemOutAndTheMorningHome)
{
	// Past the evening dusk until midnight: out
	EXPECT_EQ(SendingAt(18.0f, k_Hours), Sending::Out);
	EXPECT_EQ(SendingAt(23.9f, k_Hours), Sending::Out);
	// The evening dusk itself, and the afternoon: nothing
	EXPECT_EQ(SendingAt(16.75f, k_Hours), Sending::Nothing);
	EXPECT_EQ(SendingAt(13.0f, k_Hours), Sending::Nothing);
	// After midnight until the morning dusk ends: nothing, however dark
	EXPECT_EQ(SendingAt(2.0f, k_Hours), Sending::Nothing);
	EXPECT_EQ(SendingAt(7.2f, k_Hours), Sending::Nothing);
	// Past the morning dusk until noon: home
	EXPECT_EQ(SendingAt(7.8f, k_Hours), Sending::Home);
	EXPECT_EQ(SendingAt(11.9f, k_Hours), Sending::Home);
	EXPECT_FLOAT_EQ(NightStage(6.0f, k_Hours), 1.4f);
	EXPECT_FLOAT_EQ(NightStage(18.0f, k_Hours), 1.4f);
}

TEST(FireflyRules, FlightsTakeTheirLengthAtThreeMetresASecondAndEase)
{
	EXPECT_FLOAT_EQ(FlightSeconds(30.0f, 1.0f), 10.0f);
	EXPECT_FLOAT_EQ(FlightSeconds(1.0f, 1.4f), 0.5f);
	EXPECT_FLOAT_EQ(Ease(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(Ease(0.5f), 0.5f);
	EXPECT_FLOAT_EQ(Ease(1.0f), 1.0f);
	EXPECT_FLOAT_EQ(Ease(0.25f), 0.15625f);
}

TEST(FireflyRules, TheDriftGrowsOutAndDiesHome)
{
	EXPECT_FLOAT_EQ(Amplitude(State::Resting, 0.5f), 0.0f);
	EXPECT_FLOAT_EQ(Amplitude(State::Hovering, 0.0f), 1.0f);
	EXPECT_FLOAT_EQ(Amplitude(State::FlyingOut, 0.1f), 0.5f);
	EXPECT_FLOAT_EQ(Amplitude(State::FlyingOut, 0.5f), 1.0f);
	EXPECT_FLOAT_EQ(Amplitude(State::FlyingHome, 0.5f), 1.0f);
	EXPECT_NEAR(Amplitude(State::FlyingHome, 0.9f), 0.5f, 1e-5f);
}

TEST(FireflyRules, TheLoopsAreEightMetresAndOneAcross)
{
	Drift drift;
	// All angles at nothing: the slow loop 8 m along x and the quick 1 m, both level
	const auto start = DriftOffset(drift, 0.0f, 1.0f);
	EXPECT_FLOAT_EQ(start.x, 9.0f);
	EXPECT_FLOAT_EQ(start.y, 0.0f);
	EXPECT_FLOAT_EQ(start.z, 0.0f);
	// Up angles at a quarter turn: half as high as wide
	drift.phases = {0.0f, k_TwoPi / 4.0f, 0.0f, 0.0f, k_TwoPi / 4.0f, 0.0f};
	const auto up = DriftOffset(drift, 0.0f, 1.0f);
	EXPECT_FLOAT_EQ(up.y, 4.5f);
	EXPECT_FLOAT_EQ(DriftOffset(drift, 0.0f, 0.0f).y, 0.0f);
}

TEST(FireflyRules, TheyFadeBySquaredDistanceToNothingAt300Metres)
{
	EXPECT_EQ(OpacityAt(50.0f * 50.0f), 190);
	EXPECT_EQ(OpacityAt(10000.0f), 190);
	EXPECT_EQ(OpacityAt(40000.0f), 118);
	EXPECT_EQ(OpacityAt(90000.0f), 0);
	EXPECT_FALSE(OpacityAt(90001.0f).has_value());
}

TEST(FireflyRules, TheSearchCoversA600MetreSquare)
{
	EXPECT_EQ(SearchCells(k_SearchReach), 3600);
	const auto from = map_coords::FromMetres({100.0f, 200.0f});
	const auto nowhere = NowhereFrom(from, k_NowhereHoverHeight);
	EXPECT_NEAR(map_coords::ToMetres(nowhere.x), 115.0f, 0.001f);
	EXPECT_EQ(nowhere.z, from.z);
	EXPECT_FLOAT_EQ(nowhere.altitude, 4.0f);
}

TEST(FireflyRules, EachDrawsTwoSpeedsThenSixAngles)
{
	std::vector<float> asked;
	const auto drift = DrawDrift([&asked](float x) {
		asked.push_back(x);
		return x * 0.5f;
	});
	ASSERT_EQ(asked.size(), 8u);
	EXPECT_FLOAT_EQ(asked[0], 0.8f);
	EXPECT_FLOAT_EQ(asked[1], 0.8f);
	for (size_t i = 2; i < asked.size(); ++i)
	{
		EXPECT_FLOAT_EQ(asked[i], k_TwoPi);
	}
	EXPECT_FLOAT_EQ(drift.flightSpeed, 1.0f);
	EXPECT_FLOAT_EQ(drift.quickSpeed, 1.0f);
}

TEST(FireflyRules, TheRewardIsDrawnByWeight)
{
	RewardTable table;
	EXPECT_FALSE(table.Pick(0.5f).has_value());
	table.SetWeight(k_Fire, 1.0f);
	table.SetWeight(k_Heal, 3.0f);
	table.SetWeight(99, 5.0f);
	EXPECT_FLOAT_EQ(table.Total(), 4.0f);
	EXPECT_FALSE(table.Pick(0.0f).has_value());
	EXPECT_EQ(table.Pick(0.5f), k_Fire);
	EXPECT_EQ(table.Pick(1.0f), k_Fire);
	EXPECT_EQ(table.Pick(1.01f), k_Heal);
	EXPECT_EQ(table.Pick(4.0f), k_Heal);
	EXPECT_FALSE(table.Pick(4.5f).has_value());
	// A weight on the first kind is a chance of nothing
	table.SetWeight(0, 1.0f);
	EXPECT_FALSE(table.Pick(0.5f).has_value());
	// A land closing clears the weights but keeps the running totals until the next weight
	table.ClearWeights();
	EXPECT_FLOAT_EQ(table.Weight(k_Heal), 0.0f);
	EXPECT_FLOAT_EQ(table.Total(), 5.0f);
	table.SetWeight(k_Fire, 2.0f);
	EXPECT_FLOAT_EQ(table.Total(), 2.0f);
}

TEST_F(Fireflies, NightfallTopsUpToFiftyAndSharersAreRemovedOneATurn)
{
	_world->Add(FakeWorld::Kind::Tree, k_Here);
	_world->Add(FakeWorld::Kind::Building, k_Here + glm::vec2(20.0f, 0.0f), 3.0f);
	// The first evening turn makes 50 in the only tree, and the first in line is gone, sharing its spot
	Turns(1);
	EXPECT_EQ(_system->GetFireflies().size(), 49u);
	// One a turn goes until one is left, which then comes out
	Turns(48);
	ASSERT_EQ(_system->GetFireflies().size(), 1u);
	EXPECT_TRUE(Only().hidden);
	Turns(1);
	EXPECT_FALSE(Only().hidden);
	EXPECT_EQ(Only().state, State::FlyingOut);
}

TEST_F(Fireflies, OutTheyHoverTwoMetresAboveTheBuilding)
{
	// Hidden in a rock: with no trees on the land the evening's top up makes none
	_world->Add(FakeWorld::Kind::Building, k_Here + glm::vec2(30.0f, 0.0f), 3.0f);
	_world->Add(FakeWorld::Kind::Rock, k_Here);
	_system->Create(map_coords::FromMetres(k_Here));
	Turns(1);
	const auto& firefly = Only();
	EXPECT_EQ(firefly.state, State::FlyingOut);
	EXPECT_EQ(firefly.hover.x, map_coords::FromMetres(k_Here + glm::vec2(30.0f, 0.0f)).x);
	EXPECT_FLOAT_EQ(firefly.hover.altitude, 5.0f);
	// 30 m at its speed of 1, three metres a second, takes 10 s: a hundred turns, the first flown as it sets off
	EXPECT_NEAR(firefly.flightSeconds, 10.0f, 0.01f);
	int turns = 1;
	while (firefly.state == State::FlyingOut && turns < 200)
	{
		Turns(1);
		++turns;
	}
	EXPECT_NEAR(turns, 100, 1);
	EXPECT_EQ(firefly.state, State::Hovering);
	EXPECT_EQ(firefly.at, firefly.hover);
}

TEST_F(Fireflies, WithNoBuildingTheyHover15MetresEast)
{
	_world->Add(FakeWorld::Kind::Rock, k_Here);
	_system->Create(map_coords::FromMetres(k_Here));
	Turns(1);
	const auto& firefly = Only();
	EXPECT_NEAR(map_coords::ToMetres(firefly.hover.x), k_Here.x + 15.0f, 0.001f);
	EXPECT_FLOAT_EQ(firefly.hover.altitude, 4.0f);
}

TEST_F(Fireflies, OneWhoseTreeIsGoneIsRemovedAtNightfall)
{
	const auto tree = _world->Add(FakeWorld::Kind::Tree, k_Here);
	_world->Add(FakeWorld::Kind::Rock, k_Here + glm::vec2(50.0f));
	_system->Create(map_coords::FromMetres(k_Here));
	_world->Remove(tree);
	_world->hour = 13.0f;
	Turns(1);
	EXPECT_EQ(_system->GetFireflies().size(), 1u);
	// The top up waits for the evening: the coin's heads give trees, and there are none left, so it stops at once
	_world->hour = k_Evening;
	Turns(1);
	EXPECT_TRUE(_system->GetFireflies().empty());
}

TEST_F(Fireflies, AtDawnTheyHideInTheNearestRockOrTreeAndCanBeCaughtThere)
{
	_world->Add(FakeWorld::Kind::Building, k_Here + glm::vec2(10.0f, 0.0f), 3.0f);
	_world->Add(FakeWorld::Kind::Rock, k_Here);
	const auto rock = map_coords::FromMetres(k_Here + glm::vec2(12.0f, 0.0f));
	_world->Add(FakeWorld::Kind::Rock, k_Here + glm::vec2(12.0f, 0.0f));
	_system->Create(map_coords::FromMetres(k_Here));
	_system->SetRewardWeight("HEAL", 1.0f);
	_system->SetRewardWeight("NOT A MIRACLE", 1.0f);
	Turns(200);
	EXPECT_EQ(Only().state, State::Hovering);
	// Out, it can't be caught at its tree
	EXPECT_FALSE(_system->Catch(map_coords::FromMetres(k_Here)));

	_world->hour = k_Morning;
	Turns(200);
	const auto& firefly = Only();
	EXPECT_TRUE(firefly.hidden);
	EXPECT_EQ(firefly.state, State::Resting);
	// The second rock is nearer the building than the one it came from
	EXPECT_EQ(firefly.at, rock);

	EXPECT_FALSE(_system->Catch(map_coords::FromMetres(k_Here)));
	EXPECT_TRUE(_system->Catch(rock));
	EXPECT_TRUE(_system->GetFireflies().empty());
	ASSERT_EQ(_world->rewards.size(), 1u);
	EXPECT_EQ(_world->rewards.front().first, k_Heal);
	EXPECT_EQ(_world->rewards.front().second, rock);
	// One pick-up catches one
	EXPECT_FALSE(_system->Catch(rock));
}

TEST_F(Fireflies, WithNoWeightsACatchGivesNothing)
{
	_world->Add(FakeWorld::Kind::Tree, k_Here);
	_world->hour = 13.0f;
	_system->Create(map_coords::FromMetres(k_Here));
	EXPECT_TRUE(_system->Catch(map_coords::FromMetres(k_Here)));
	EXPECT_TRUE(_world->rewards.empty());
}

TEST_F(Fireflies, ALandClosingRemovesThemAndTopsUpAgain)
{
	_world->Add(FakeWorld::Kind::Tree, k_Here);
	Turns(1);
	EXPECT_FALSE(_system->GetFireflies().empty());
	_system->Reset();
	EXPECT_TRUE(_system->GetFireflies().empty());
	Turns(1);
	EXPECT_EQ(_system->GetFireflies().size(), 49u);
}

TEST_F(Fireflies, OutTheyAreDrawnBetweenTheirLastTwoTurnsAndFadeWithDistance)
{
	_world->look = ecs::components::Sprite {.tint = glm::vec4(1.0f)};
	_world->Add(FakeWorld::Kind::Building, k_Here + glm::vec2(30.0f, 0.0f), 3.0f);
	_world->Add(FakeWorld::Kind::Rock, k_Here);
	const auto entity = _system->Create(map_coords::FromMetres(k_Here));
	// Hidden, it isn't drawn
	_system->Update(16.0f, 0.5f);
	EXPECT_FALSE(_world->registry.AllOf<ecs::components::Sprite>(entity));

	Turns(10);
	auto& firefly = _world->registry.Get<Firefly>(entity);
	// Its drift set to a plain one, so where it is drawn can be worked out
	firefly.drift = {};
	_world->camera = _world->ToWorld(firefly.at) + glm::vec3(0.0f, 0.0f, 200.0f);
	_system->Update(0.0f, 0.5f);
	const auto previous = _world->ToWorld(firefly.previous);
	const auto now = _world->ToWorld(firefly.at);
	const auto offset = DriftOffset(firefly.drift, firefly.clock, Amplitude(firefly.state, firefly.progress));
	const auto expected = (previous + now) * 0.5f + offset;
	const auto& drawn = _world->registry.Get<const ecs::components::Transform>(entity).position;
	EXPECT_NEAR(drawn.x, expected.x, 1e-3f);
	EXPECT_NEAR(drawn.y, expected.y, 1e-3f);
	EXPECT_NEAR(drawn.z, expected.z, 1e-3f);
	ASSERT_TRUE(_world->registry.AllOf<ecs::components::Sprite>(entity));
	// The opacity is taken from where it was drawn the frame before: here its hiding place, 200 m away
	EXPECT_NEAR(_world->registry.Get<const ecs::components::Sprite>(entity).tint.a, 118.0f / 255.0f, 0.01f);

	// Beyond 300 m it isn't drawn, and its loops don't turn
	_world->camera = drawn + glm::vec3(0.0f, 0.0f, 400.0f);
	const float clock = firefly.clock;
	_system->Update(100.0f, 0.5f);
	EXPECT_FALSE(_world->registry.AllOf<ecs::components::Sprite>(entity));
	EXPECT_FLOAT_EQ(firefly.clock, clock);
}
