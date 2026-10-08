/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <set>
#include <vector>

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "ECS/Components/Creature.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "Locator.h"

#define LOCATOR_IMPLEMENTATIONS
#include "ECS/Systems/Implementations/TornadoSystem.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::systems;

namespace
{
/// Records the creatures made to faint, which stay out cold
class FakeFights final: public CreatureFightSystemInterface
{
public:
	void ProcessTurn() override {}
	void Update(std::chrono::duration<float, std::milli> /*gameTime*/) override {}
	StartResult StartFight(entt::entity /*creature*/, entt::entity /*opponent*/) override { return StartResult::Busy; }
	void AbortFight(entt::entity /*creature*/) override {}
	[[nodiscard]] bool IsFighting(entt::entity /*creature*/) const override { return false; }
	[[nodiscard]] std::optional<entt::entity> OpponentOf(entt::entity /*creature*/) const override { return std::nullopt; }
	bool QueueMove(entt::entity /*creature*/, const creature_fight::Move& /*move*/, bool /*replace*/) override { return false; }
	void ReleaseCharge(entt::entity /*creature*/, float /*heldMs*/) override {}
	void SetAutoFighting(entt::entity /*creature*/, bool /*autoFight*/) override {}
	[[nodiscard]] bool IsAutoFighting(entt::entity /*creature*/) const override { return false; }
	bool Press(const glm::vec3& /*rayOrigin*/, const glm::vec3& /*rayDirection*/) override { return false; }
	void Release() override {}
	[[nodiscard]] bool IsPressed() const override { return false; }
	void KnockOut(entt::entity creature) override { knockedOut.push_back(creature); }
	void ForceFaint(entt::entity creature) override
	{
		fainted.push_back(creature);
		out.insert(creature);
	}
	void KillPermanently(entt::entity /*creature*/) override {}
	void Resurrect(entt::entity creature) override { out.erase(creature); }
	[[nodiscard]] bool IsKnockedOut(entt::entity creature) const override { return out.contains(creature); }
	[[nodiscard]] std::optional<creature_fight_hud::Values> GetPanel() const override { return std::nullopt; }
	void SetAngerStartsFights(bool /*enabled*/) override {}
	[[nodiscard]] bool GetAngerStartsFights() const override { return false; }
	void SetCameraWatches(bool /*enabled*/) override {}
	[[nodiscard]] bool GetCameraWatches() const override { return false; }
	[[nodiscard]] bool IsCameraOnFight() const override { return false; }

	std::vector<entt::entity> fainted;
	std::vector<entt::entity> knockedOut;
	std::set<entt::entity> out;
};

class TornadoCreature: public ::testing::Test
{
protected:
	void SetUp() override
	{
		if (spdlog::get("game") == nullptr)
		{
			spdlog::create<spdlog::sinks::null_sink_st>("game");
		}
		Locator::entitiesRegistry::emplace<Registry>();
		Locator::creatureFightSystem::emplace<FakeFights>();
		auto& registry = Locator::entitiesRegistry::value();
		creature = registry.Create();
		registry.Assign<components::Transform>(creature, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<components::Creature>(creature);
	}
	void TearDown() override
	{
		Locator::creatureFightSystem::reset();
		Locator::entitiesRegistry::reset();
	}
	[[nodiscard]] static FakeFights& Fights() { return static_cast<FakeFights&>(Locator::creatureFightSystem::value()); }

	TornadoSystem tornado;
	entt::entity creature {entt::null};
};
} // namespace

TEST_F(TornadoCreature, ACreatureCaughtFaintsHelplessAndNotTwiceWhileOutCold)
{
	tornado.CatchCreature(creature);
	ASSERT_EQ(Fights().fainted.size(), 1u);
	EXPECT_EQ(Fights().fainted.front(), creature);
	// Not as the loser of a fight
	EXPECT_TRUE(Fights().knockedOut.empty());
	// Caught again while it lies out cold, nothing more happens
	tornado.CatchCreature(creature);
	EXPECT_EQ(Fights().fainted.size(), 1u);
}

TEST_F(TornadoCreature, OnceRecoveredItCanBeCaughtAgain)
{
	tornado.CatchCreature(creature);
	Fights().Resurrect(creature);
	tornado.CatchCreature(creature);
	EXPECT_EQ(Fights().fainted.size(), 2u);
}
