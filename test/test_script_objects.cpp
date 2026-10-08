/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "ECS/ScriptObjectTable.h"
#include "ECS/ScriptObjectsWorld.h"
#include "ECS/Systems/Implementations/ScriptObjectsSystem.h"

using namespace openblack::ecs::script_objects;
using openblack::ecs::systems::ScriptObjectsSystem;

namespace
{
/// A world of numbered objects, recording what the table does to them
struct FakeWorld final: World
{
	struct Object
	{
		Kind kind {Kind::Other};
		bool available {true};
		bool inScript {false};
		bool controlled {false};
		bool inPhysics {false};
		bool inMap {true};
	};
	std::map<entt::entity, Object> objects;
	std::vector<entt::entity> decided;
	std::vector<entt::entity> decideAfterPhysics;
	std::vector<entt::entity> abandoned;
	std::vector<entt::entity> deleted;

	entt::entity Add(uint32_t id, Object object = {})
	{
		const auto entity = static_cast<entt::entity>(id);
		objects[entity] = object;
		return entity;
	}

	[[nodiscard]] bool Exists(entt::entity object) const override { return objects.contains(object); }
	[[nodiscard]] bool IsAvailable(entt::entity object) const override
	{
		return Exists(object) && objects.at(object).available;
	}
	[[nodiscard]] Kind KindOf(entt::entity object) const override { return objects.at(object).kind; }
	[[nodiscard]] bool IsInScript(entt::entity object) const override { return objects.at(object).inScript; }
	void SetInScript(entt::entity object, bool inScript) override { objects.at(object).inScript = inScript; }
	[[nodiscard]] bool IsControlled(entt::entity object) const override { return objects.at(object).controlled; }
	void SetControlled(entt::entity object, bool controlled) override { objects.at(object).controlled = controlled; }
	[[nodiscard]] bool IsInPhysics(entt::entity object) const override { return objects.at(object).inPhysics; }
	[[nodiscard]] bool IsInMap(entt::entity object) const override { return objects.at(object).inMap; }
	void SetVillagerDecideWhatToDo(entt::entity villager) override { decided.push_back(villager); }
	void SetVillagerPreviousDecideWhatToDo(entt::entity villager) override { decideAfterPhysics.push_back(villager); }
	void AbandonCreatureAction(entt::entity creature) override { abandoned.push_back(creature); }
	void Delete(entt::entity object) override
	{
		deleted.push_back(object);
		objects.erase(object);
	}
};

struct System
{
	FakeWorld* world;
	std::unique_ptr<ScriptObjectsSystem> system;
};

System MakeSystem()
{
	// The table reports its errors to the scripts' log
	if (spdlog::get("scripting") == nullptr)
	{
		spdlog::create<spdlog::sinks::null_sink_mt>("scripting");
	}
	auto world = std::make_unique<FakeWorld>();
	auto* raw = world.get();
	return {.world = raw, .system = std::make_unique<ScriptObjectsSystem>(std::move(world))};
}

constexpr uint32_t k_MoveNative = 33;
constexpr uint32_t k_CreateNative = 27;
} // namespace

TEST(ScriptObjects, OnlyTheNativesThatMoveOrSetObjectsTakeControl)
{
	EXPECT_TRUE(TakesControl(17));  // set script state
	EXPECT_TRUE(TakesControl(33));  // move a thing
	EXPECT_TRUE(TakesControl(218)); // start a refereed match
	EXPECT_FALSE(TakesControl(27)); // create
	EXPECT_FALSE(TakesControl(110));
}

TEST(ScriptObjects, AnObjectInAScriptKeepsItsPlace)
{
	Table table;
	const auto first = table.Register(42, true, false);
	ASSERT_TRUE(first.has_value());
	EXPECT_EQ(*first, 1);
	// Not yet in a script, it would be given another place, as the game does
	EXPECT_EQ(table.Register(42, false, false), 2);
	EXPECT_EQ(table.Register(42, false, true), first);
	EXPECT_TRUE(table.At(*first).createdByScript);
}

TEST(ScriptObjects, TheFirstReferenceIsTold)
{
	Table table;
	const auto place = *table.Register(7, false, false);
	EXPECT_EQ(table.AddReference(place), Referenced::First);
	EXPECT_EQ(table.AddReference(place), Referenced::Again);
	table.RemoveReference(place);
	table.RemoveReference(place);
	table.RemoveReference(place);
	EXPECT_EQ(table.At(place).count, 0);
	// Let go of every reference, the place is still the object's: places are only freed when the land's scripts end
	EXPECT_EQ(table.Find(7), place);
	EXPECT_EQ(table.AddReference(0), Referenced::Nothing);
}

TEST(ScriptObjects, TheTableFillsAndIsClearedWithTheLand)
{
	Table table;
	for (uint32_t object = 1; object < k_Places; ++object)
	{
		ASSERT_TRUE(table.Register(object, false, false).has_value());
	}
	EXPECT_FALSE(table.Register(9999, false, false).has_value());
	table.Clear();
	EXPECT_TRUE(table.Register(9999, false, false).has_value());
}

TEST(ScriptObjects, ADeadTreeTakesItsTreesPlace)
{
	Table table;
	const auto place = *table.Register(5, false, false);
	table.Replace(5, 6);
	EXPECT_EQ(table.Find(6), place);
	EXPECT_FALSE(table.Find(5).has_value());
}

TEST(ScriptObjectsSystem, AFoundObjectIsInAScriptButNotControlledAtItsFirstReference)
{
	auto [world, system] = MakeSystem();
	const auto object = world->Add(3);
	system->AddReference(object);
	EXPECT_TRUE(world->objects.at(object).inScript);
	EXPECT_FALSE(world->objects.at(object).controlled);
}

TEST(ScriptObjectsSystem, AnObjectAScriptMadeIsControlledAtItsFirstReference)
{
	auto [world, system] = MakeSystem();
	const auto object = world->Add(3);
	ASSERT_TRUE(system->Register(object, true));
	system->AddReference(object);
	EXPECT_TRUE(world->objects.at(object).controlled);
}

TEST(ScriptObjectsSystem, OnlyAControllingNativeTakesControlOfWhatItIsGiven)
{
	auto [world, system] = MakeSystem();
	const auto object = world->Add(3);
	system->EnterNative(k_CreateNative);
	system->Fetch(object);
	EXPECT_FALSE(world->objects.at(object).controlled);
	system->EnterNative(k_MoveNative);
	EXPECT_EQ(system->Fetch(object), object);
	EXPECT_TRUE(world->objects.at(object).controlled);
}

TEST(ScriptObjectsSystem, AReleasedVillagerInTheMapDecidesWhatToDo)
{
	auto [world, system] = MakeSystem();
	const auto villager = world->Add(4, {.kind = Kind::Villager, .controlled = true});
	system->ReleaseFromScript(villager);
	EXPECT_FALSE(world->objects.at(villager).controlled);
	EXPECT_EQ(world->decided, std::vector {villager});
	EXPECT_TRUE(world->decideAfterPhysics.empty());
}

TEST(ScriptObjectsSystem, AReleasedVillagerInThePhysicsDecidesWhatToDoOnceOut)
{
	auto [world, system] = MakeSystem();
	const auto villager = world->Add(4, {.kind = Kind::Villager, .controlled = true, .inPhysics = true, .inMap = false});
	system->ReleaseFromScript(villager);
	EXPECT_TRUE(world->decided.empty());
	EXPECT_EQ(world->decideAfterPhysics, std::vector {villager});
}

TEST(ScriptObjectsSystem, AReleasedVillagerHeldOutOfTheMapOnlyLosesControl)
{
	auto [world, system] = MakeSystem();
	const auto held = world->Add(4, {.kind = Kind::Villager, .controlled = true, .inMap = false});
	const auto dying = world->Add(5, {.kind = Kind::Villager, .available = false, .controlled = true});
	system->ReleaseFromScript(held);
	system->ReleaseFromScript(dying);
	EXPECT_FALSE(world->objects.at(held).controlled);
	EXPECT_TRUE(world->decided.empty());
	EXPECT_TRUE(world->decideAfterPhysics.empty());
}

TEST(ScriptObjectsSystem, AReleasedCreatureGivesUpWhatItWasDoing)
{
	auto [world, system] = MakeSystem();
	const auto creature = world->Add(6, {.kind = Kind::Creature, .controlled = true});
	const auto free = world->Add(7, {.kind = Kind::Creature});
	system->ReleaseFromScript(creature);
	system->ReleaseFromScript(free);
	EXPECT_EQ(world->abandoned, std::vector {creature});
}

TEST(ScriptObjectsSystem, ClearingTheScriptsDeletesWhatTheyMadeAndLetsGoOfTheRest)
{
	auto [world, system] = MakeSystem();
	const auto made = world->Add(10);
	const auto villager = world->Add(11, {.kind = Kind::Villager});
	const auto rock = world->Add(12);
	ASSERT_TRUE(system->Register(made, true));
	system->AddReference(made);
	system->AddReference(villager);
	system->EnterNative(k_MoveNative);
	system->Fetch(villager);
	system->AddReference(rock);
	system->Reset();
	EXPECT_EQ(world->deleted, std::vector {made});
	EXPECT_FALSE(world->objects.at(villager).controlled);
	EXPECT_FALSE(world->objects.at(villager).inScript);
	EXPECT_EQ(world->decided, std::vector {villager});
	EXPECT_FALSE(world->objects.at(rock).inScript);
	// Every place is free again: the villager is given the first place anew
	system->AddReference(villager);
	EXPECT_TRUE(world->objects.at(villager).inScript);
	EXPECT_FALSE(world->objects.at(villager).controlled);
}

TEST(ScriptObjectsSystem, ADeadTreeTakesOnlyItsTreesPlace)
{
	auto [world, system] = MakeSystem();
	const auto tree = world->Add(20);
	const auto dead = world->Add(21);
	ASSERT_TRUE(system->Register(tree, true));
	system->AddReference(tree);
	system->Replace(tree, dead);
	EXPECT_FALSE(world->objects.at(dead).inScript);
	EXPECT_FALSE(world->objects.at(dead).controlled);
	// Its next reference puts it in a script again, but control only comes with a place's first reference
	system->AddReference(dead);
	EXPECT_TRUE(world->objects.at(dead).inScript);
	EXPECT_FALSE(world->objects.at(dead).controlled);
}
