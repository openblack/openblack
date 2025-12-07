/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <filesystem>
#include <fstream>
#include <tuple>

#include <ECS/Components/Transform.h>
#include <ECS/Components/Villager.h>
#include <ECS/Components/WallHug.h>
#include <ECS/Map.h>
#include <ECS/Registry.h>
#include <ECS/Systems/PathfindingSystemInterface.h>
#include <Game.h>
#include <LHScriptX/Script.h>
#include <Locator.h>
#include <gtest/gtest.h>
#include <json_helpers.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using nlohmann::json;
using namespace openblack;

enum MOVE_STATE
{
	MOVE_STATE_ARRIVED,
	MOVE_STATE_FINAL_STEP,
	MOVE_STATE_STEP_THROUGH,
	MOVE_STATE_LINEAR,
	MOVE_STATE_LINEAR_CW,
	MOVE_STATE_LINEAR_CCW,
	MOVE_STATE_ORBIT_CW,
	MOVE_STATE_ORBIT_CCW,
	MOVE_STATE_EXIT_CIRCLE_CCW,
	MOVE_STATE_EXIT_CIRCLE_CW,

	_MOVE_STATE_INVALID = -1,
};

// NOLINTNEXTLINE(modernize-avoid-c-arrays): external macro
NLOHMANN_JSON_SERIALIZE_ENUM( //
    MOVE_STATE,               //
    {
        {_MOVE_STATE_INVALID, nullptr},
        {MOVE_STATE_ARRIVED, "ARRIVED"},
        {MOVE_STATE_FINAL_STEP, "FINAL_STEP"},
        {MOVE_STATE_STEP_THROUGH, "STEP_THROUGH"},
        {MOVE_STATE_LINEAR, "LINEAR"},
        {MOVE_STATE_LINEAR_CW, "LINEAR_CW"},
        {MOVE_STATE_LINEAR_CCW, "LINEAR_CCW"},
        {MOVE_STATE_ORBIT_CW, "ORBIT_CW"},
        {MOVE_STATE_ORBIT_CCW, "ORBIT_CCW"},
        {MOVE_STATE_EXIT_CIRCLE_CCW, "EXIT_CIRCLE_CCW"},
        {MOVE_STATE_EXIT_CIRCLE_CW, "EXIT_CIRCLE_CW"},
    })

enum VILLAGER_STATE
{
	VILLAGER_STATE_MOVE_TO_POS,
	VILLAGER_STATE_GO_AND_CHILLOUT_OUTSIDE_HOME,
	VILLAGER_STATE_MOVE_ON_PATH,
	VILLAGER_STATE_ARRIVES_HOME,

	_VILLAGER_STATE_INVALID = -1,
};

// NOLINTNEXTLINE(modernize-avoid-c-arrays): external macro
NLOHMANN_JSON_SERIALIZE_ENUM( //
    VILLAGER_STATE,           //
    {
        {_VILLAGER_STATE_INVALID, nullptr},
        {VILLAGER_STATE_MOVE_TO_POS, "MOVE_TO_POS"},
        {VILLAGER_STATE_GO_AND_CHILLOUT_OUTSIDE_HOME, "GO_AND_CHILLOUT_OUTSIDE_HOME"},
        {VILLAGER_STATE_MOVE_ON_PATH, "MOVE_ON_PATH"},
        {VILLAGER_STATE_ARRIVES_HOME, "ARRIVES_HOME"},
    })

class MobileWallHugWalks: public ::testing::Test
{
protected:
	struct State
	{
		struct CircleHugInfo
		{
			// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
			std::optional<uint32_t> obj_index;
			// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
			uint8_t turns_to_obstacle;
			// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
			uint8_t field_0x5; // TODO(bwrsandman): Unknown function
			// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
			uint16_t field_0x6; // TODO(bwrsandman): Unknown function
		};

		uint32_t turn;
		uint32_t id;
		glm::vec2 pos;
		// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		uint16_t field_0x24; // TODO(bwrsandman): Unknown function
		                     // NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		uint8_t field_0x26;  // TODO(bwrsandman): Unknown function
		                     // NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		uint32_t turns_until_next_state_change;
		float speed;
		// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		float y_angle;
		// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		MOVE_STATE move_state;
		glm::vec2 step;
		// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		CircleHugInfo circle_hug_info;
		uint32_t status; // Probably unused
		glm::vec2 goal;
		float distance; // Derived
		// NOLINTNEXTLINE(readability-identifier-naming): Needs to match json
		std::tuple<VILLAGER_STATE, VILLAGER_STATE> state_map;
	};

	void SetUp() override
	{
		const auto* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		const auto testResultsPath = std::filesystem::path(k_ScenarioPath) / (testName + std::string(".json"));
		json results;
		std::ifstream(testResultsPath) >> results;
		_startTurn = results["start_turn"];
		_lastTurn = results["last_turn"];
		const auto testScenePath = std::filesystem::path(k_ScenarioPath) / results["map_file"];
		const auto& villagerStates = results["villager_states"];
		_expectedStates.reserve(villagerStates.size());
#define GET_SAME_TYPE_AS(name) s[#name].get<decltype(_expectedStates[0].name)>()
#define GET_SAME_TYPE_AS_HEX(name) \
	static_cast<decltype(_expectedStates[0].name)>(std::stoul(s[#name].get<std::string>(), nullptr, 16))
#define GET_VEC2(name) decltype(_expectedStates[0].name)(s[#name][0].get<float>(), s[#name][1].get<float>())
		for (const auto& s : villagerStates)
		{
			auto circleHugInfo = s["circle_hug_info"];
			const auto& e = _expectedStates.emplace_back(State {
			    GET_SAME_TYPE_AS(turn),
			    GET_SAME_TYPE_AS(id),
			    GET_VEC2(pos),
			    GET_SAME_TYPE_AS_HEX(field_0x24),
			    GET_SAME_TYPE_AS_HEX(field_0x26),
			    GET_SAME_TYPE_AS(turns_until_next_state_change),
			    GET_SAME_TYPE_AS(speed),
			    GET_SAME_TYPE_AS(y_angle),
			    GET_SAME_TYPE_AS(move_state),
			    GET_VEC2(step),
			    {
			        circleHugInfo["obj_index"].is_null() ? std::nullopt
			                                             : std::make_optional(circleHugInfo["obj_index"].get<uint32_t>()),
			        static_cast<uint8_t>(std::stoi(circleHugInfo["turns_to_obj"].get<std::string>(), nullptr, 16)),
			        static_cast<uint8_t>(std::stoi(circleHugInfo["field_0x5"].get<std::string>(), nullptr, 16)),
			        static_cast<uint16_t>(std::stoi(circleHugInfo["field_0x6"].get<std::string>(), nullptr, 16)),
			    },
			    GET_SAME_TYPE_AS_HEX(status),
			    GET_VEC2(goal),
			    GET_SAME_TYPE_AS(distance),
			    GET_SAME_TYPE_AS(state_map),
			});

			ASSERT_NE(e.move_state, _MOVE_STATE_INVALID) << "Unexpected Move State: " + s["move_state"].get<std::string>();
			ASSERT_NE(std::get<0>(e.state_map), _VILLAGER_STATE_INVALID)
			    << "Unexpected Villager State: " + s["state_map"][0].get<std::string>();
			ASSERT_NE(std::get<1>(e.state_map), _VILLAGER_STATE_INVALID)
			    << "Unexpected Villager State: " + s["state_map"][1].get<std::string>();
		}

#undef GET_SAME_TYPE_AS
#undef GET_SAME_TYPE_AS_HEX
#undef GET_MAP_COORDS

		ASSERT_TRUE(std::filesystem::exists(testScenePath));

		{
			std::ifstream ifs(testScenePath);
			_sceneScript = std::string(std::istreambuf_iterator<char> {ifs}, {});
		}

		static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = openblack::Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = mockGamePath.string(),
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::warn);
		args.logLevels[static_cast<uint8_t>(openblack::LoggingSubsystem::pathfinding)] = spdlog::level::debug;
		_game = std::make_unique<openblack::Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());
		openblack::lhscriptx::Script script;
		script.Load(_sceneScript);

		_villagerEntt = Locator::entitiesRegistry::value().Front<const ecs::components::Villager>();
		auto& villagerTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(_villagerEntt);

		villagerTransform.position = glm::vec3(_expectedStates[0].pos.x, 0.0f, _expectedStates[0].pos.y);
	}

	void TearDown() override { _game.reset(); }

	void MobileWallHugScenarioAssert()
	{
		auto& map = Locator::entitiesMap::value();
		auto& registry = Locator::entitiesRegistry::value();
		map.Rebuild();
		registry.Each<ecs::components::WallHug>([&registry, this](entt::entity entity, ecs::components::WallHug& wallHug) {
			using namespace openblack::ecs::components;
			registry.Assign<MoveStateLinearTag>(entity);
			wallHug.speed = _expectedStates[0].speed;
			wallHug.step = _expectedStates[0].step;
			wallHug.goal = _expectedStates[0].goal;
		});

		for (uint32_t turn = _startTurn; turn < _lastTurn; ++turn)
		{
			const auto& villagerComp = registry.Get<ecs::components::Villager>(_villagerEntt);
			const auto& villagerTransform = registry.Get<ecs::components::Transform>(_villagerEntt);
			const auto& villagerWallhug = registry.Get<ecs::components::WallHug>(_villagerEntt);
			bool villagerHasObstacle = registry.AnyOf<ecs::components::WallHugObjectReference>(_villagerEntt);
			const auto& state = _expectedStates[turn - _startTurn];
			const auto msg = std::string("on turn ") + std::to_string(turn) + " in range " + std::to_string(_startTurn) + "-" +
			                 std::to_string(_lastTurn);

			switch (state.move_state)
			{
			default:
			case _MOVE_STATE_INVALID:
				ASSERT_TRUE(false);
				break;
			case MOVE_STATE_LINEAR:
			case MOVE_STATE_LINEAR_CW:
			case MOVE_STATE_LINEAR_CCW:
			{
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateLinearTag>(_villagerEntt)) << msg;
				const auto& villagerState = registry.Get<ecs::components::MoveStateLinearTag>(_villagerEntt);
				if (state.move_state == MOVE_STATE_LINEAR)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::Undefined) << msg;
				}
				else if (state.move_state == MOVE_STATE_LINEAR_CW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::Clockwise) << msg;
				}
				else if (state.move_state == MOVE_STATE_LINEAR_CCW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::CounterClockwise) << msg;
				}
			}
			break;
			case MOVE_STATE_ORBIT_CW:
			case MOVE_STATE_ORBIT_CCW:
			{
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateOrbitTag>(_villagerEntt)) << msg;
				const auto& villagerState = registry.Get<ecs::components::MoveStateOrbitTag>(_villagerEntt);
				if (state.move_state == MOVE_STATE_ORBIT_CW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::Clockwise) << msg;
				}
				else if (state.move_state == MOVE_STATE_ORBIT_CCW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::CounterClockwise) << msg;
				}
			}
			break;
			case MOVE_STATE_EXIT_CIRCLE_CW:
			case MOVE_STATE_EXIT_CIRCLE_CCW:
			{
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateExitCircleTag>(_villagerEntt)) << msg;
				const auto& villagerState = registry.Get<ecs::components::MoveStateExitCircleTag>(_villagerEntt);
				if (state.move_state == MOVE_STATE_EXIT_CIRCLE_CW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::Clockwise) << msg;
				}
				else if (state.move_state == MOVE_STATE_EXIT_CIRCLE_CCW)
				{
					ASSERT_EQ(villagerState.clockwise, ecs::components::MoveStateClockwise::CounterClockwise) << msg;
				}
			}
			break;
			case MOVE_STATE_ARRIVED:
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateArrivedTag>(_villagerEntt)) << msg;
				break;
			case MOVE_STATE_FINAL_STEP:
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateFinalStepTag>(_villagerEntt)) << msg;
				break;
			case MOVE_STATE_STEP_THROUGH:
				ASSERT_TRUE(registry.AllOf<ecs::components::MoveStateStepThroughTag>(_villagerEntt)) << msg;
				break;
			}
			ASSERT_FLOAT_EQ(villagerTransform.position.x, state.pos.x) << msg;
			ASSERT_FLOAT_EQ(villagerTransform.position.z, state.pos.y) << msg;
			if (state.move_state != MOVE_STATE_FINAL_STEP) // Set in the next turn
			{
				ASSERT_NEAR(villagerWallhug.step.x, state.step.x, 0.001f) << msg;
				ASSERT_NEAR(villagerWallhug.step.y, state.step.y, 0.001f) << msg;
			}
			ASSERT_FLOAT_EQ(villagerWallhug.goal.x, state.goal.x) << msg;
			ASSERT_FLOAT_EQ(villagerWallhug.goal.y, state.goal.y) << msg;
			ASSERT_EQ(villagerHasObstacle, state.circle_hug_info.obj_index.has_value()) << msg;
			if (state.circle_hug_info.turns_to_obstacle != 0xFF || state.circle_hug_info.obj_index.has_value())
			{
				ASSERT_TRUE(villagerHasObstacle) << msg;
				const auto& ref = registry.Get<ecs::components::WallHugObjectReference>(_villagerEntt);
				// ASSERT_EQ(ref.stepsAway, state.circle_hug_info.turns_to_obstacle) << msg;
			}

			ASSERT_NO_THROW(Locator::pathfindingSystem::value().Update()) << msg;
		}
	}

	static constexpr std::string_view k_ScenarioPath = TEST_BINARY_DIR "/mobile_wall_hug/scenarios";
	std::string _sceneScript;
	uint32_t _startTurn;
	uint32_t _lastTurn;
	std::vector<State> _expectedStates;
	std::unique_ptr<openblack::Game> _game;
	entt::entity _villagerEntt;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(MobileWallHugWalks, mobilewallhug1)
{
	MobileWallHugScenarioAssert();
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(MobileWallHugWalks, mobilewallhug2)
{
	MobileWallHugScenarioAssert();
}

// TODO(bwrsandman): Remove DISABLED_ prefix once walking on footpath is implemented
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(MobileWallHugWalks, DISABLED_footpath1)
{
	MobileWallHugScenarioAssert();
}

// TODO(bwrsandman): Remove DISABLED_ prefix once walking on footpath is implemented
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(MobileWallHugWalks, DISABLED_footpath2)
{
	MobileWallHugScenarioAssert();
}
