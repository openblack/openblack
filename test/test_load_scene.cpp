/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <CHLApi.h>
#include <ECS/Archetypes/MobileStaticArchetype.h>
#include <ECS/Components/Transform.h>
#include <ECS/Registry.h>
#include <ECS/Systems/RenderingSystemInterface.h>
#include <Game.h>
#include <LHScriptX/Script.h>
#include <LHVM.h>
#include <Locator.h>
#include <ScriptHeaders/ScriptEnums.h>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/euler_angles.hpp>
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

TEST_F(LoadScene, ScriptFocusRotatesOnlyMobileStaticsAndPreservesTilt)
{
	using namespace openblack;
	using ecs::components::Transform;
	ASSERT_TRUE(game_->LoadMap("Scripts/Land1.txt"));
	auto& registry = Locator::entitiesRegistry::value();
	auto& vm = Locator::vm::value();
	const auto& natives = Locator::chlapi::value().GetFunctionsTable();
	const auto focus = [&](entt::entity entity) {
		vm.Pusho(static_cast<uint32_t>(entity));
		vm.Pushv(0.0f);
		vm.Pushv(20.0f);
		vm.Pushv(5.0f);
		natives.at(34).impl();
	};

	vm.Pushi(static_cast<int32_t>(script::ObjectType::MobileStatic));
	vm.Pushi(static_cast<int32_t>(MobileStaticInfo::Rock));
	vm.Pushv(0.0f);
	vm.Pushv(0.0f);
	vm.Pushv(0.0f);
	natives.at(27).impl();
	const auto rock = static_cast<entt::entity>(vm.Pop().uintVal);
	ASSERT_NE(static_cast<uint32_t>(rock), 0u);
	auto& renderer = Locator::rendereringSystem::value();
	renderer.PrepareDraw(false, false, false);
	const auto beforeFocus = renderer.GetContext().instanceUniforms;
	focus(rock);
	renderer.PrepareDraw(false, false, false);
	EXPECT_NE(renderer.GetContext().instanceUniforms, beforeFocus);
	EXPECT_EQ(registry.Get<Transform>(rock).rotation, glm::mat3(glm::eulerAngleY(-glm::half_pi<float>())));

	const auto tilted =
	    ecs::archetypes::MobileStaticArchetype::Create(glm::vec3(0), MobileStaticInfo::Rock, 0.0f, 0.3f, 0.0f, 0.8f, 2.0f);
	const auto& tiltedTransform = registry.Get<Transform>(tilted);
	const auto expected = glm::mat3(glm::eulerAngleY(-glm::half_pi<float>())) * tiltedTransform.rotation;
	focus(tilted);
	for (glm::length_t axis = 0; axis < glm::mat3::length(); ++axis)
	{
		EXPECT_LT(glm::distance(tiltedTransform.rotation[axis], expected[axis]), 1e-6f);
	}
	EXPECT_EQ(tiltedTransform.scale, glm::vec3(2.0f));

	const auto other = registry.Create();
	registry.Assign<Transform>(other, glm::vec3(0), glm::mat3(1), glm::vec3(1));
	focus(other);
	EXPECT_EQ(registry.Get<Transform>(other).rotation, glm::mat3(1));
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
