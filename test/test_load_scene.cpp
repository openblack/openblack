/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <map>

#include <ECS/Components/Creature.h>
#include <ECS/Components/CreatureState.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <LHScriptX/Script.h>
#include <Locator.h>
#include <entt/core/hashed_string.hpp>
#include <gtest/gtest.h>

class LoadScene: public ::testing::Test
{
public:
	void LoadTestScene(const char* sceneScript)
	{
		openblack::lhscriptx::Script script;
		script.Load(sceneScript);
	}

protected:
	void SetUp() override
	{
		static const auto mock_game_path = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = openblack::Arguments {
		    .graphicsBackend = openblack::GraphicsBackend::Noop,
		    .gamePath = mock_game_path.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::debug);
		game_ = std::make_unique<openblack::Game>(std::move(args));
		ASSERT_TRUE(game_->Initialize());
	}
	void TearDown() override { game_.reset(); }
	std::unique_ptr<openblack::Game> game_;
};

TEST_F(LoadScene, minimal)
{
	LoadTestScene(R""""(
VERSION(2.300000)
)"""");
}

TEST_F(LoadScene, landscape_only)
{
	LoadTestScene(R""""(
VERSION(2.300000)
LOAD_LANDSCAPE(".\Data\Landscape\Land1.lnd")
)"""");
}

TEST_F(LoadScene, load_abode_no_landscape)
{
	const char* sceneScript = R""""(
VERSION(2.300000)
CREATE_ABODE(0, "2224.63,2372.52", "CELTIC_ABODE_F", 11100, 1095, 0, 0)
)"""";
	ASSERT_THROW(LoadTestScene(sceneScript), std::runtime_error);
}

TEST_F(LoadScene, load_celtic_abode_f)
{
	LoadTestScene(R""""(
VERSION(2.300000)
LOAD_LANDSCAPE(".\Data\Landscape\Land1.lnd")
CREATE_ABODE(0, "2224.63,2372.52", "CELTIC_ABODE_F", 11100, 1095, 0, 0)
)"""");
}

TEST_F(LoadScene, CreaturesSharingAMindReadDesireTimesFromTheirInfoRecords)
{
	using namespace openblack;
	using namespace openblack::ecs::components;

	LoadTestScene(R""""(
VERSION(2.300000)
LOAD_LANDSCAPE(".\Data\Landscape\Land1.lnd")
CREATE_CREATURE_FROM_FILE("PLAYER_ONE", 1, "SharedMind", "1000.00,1000.00")
CREATE_CREATURE_FROM_FILE("PLAYER_TWO", 17, "SharedMind", "1010.00,1000.00")
)"""");

	std::map<CreatureType, std::array<float, 2>> increaseTimes;
	Locator::entitiesRegistry::value().Each<const Creature, const CreatureState>(
	    [&](const Creature& creature, const CreatureState& state) {
		    EXPECT_EQ(creature.mind, entt::hashed_string::value("SharedMind"));
		    increaseTimes.emplace(creature.species,
		                          std::array {state.desireIncreaseTimes.front(), state.desireIncreaseTimes.back()});
	    });
	const std::map<CreatureType, std::array<float, 2>> expected {{CreatureType::Cow, {2.5f, 6.0f}},
	                                                             {CreatureType::GiantApe, {8.0f, 3.5f}}};
	EXPECT_EQ(increaseTimes, expected);
}
