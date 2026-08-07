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

#include <algorithm>
#include <filesystem>
#include <memory>
#include <utility>

#include <CHLApi.h>
#include <ECS/Components/Transform.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <LHVM.h>
#include <Locator.h>
#include <entt/entity/entity.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

using namespace openblack;
using namespace openblack::ecs::components;

namespace
{

constexpr size_t k_SetFocusFunctionId = 34;

void ExpectMatrixNear(const glm::mat3& actual, const glm::mat3& expected)
{
	for (glm::length_t column = 0; column < glm::mat3::length(); ++column)
	{
		for (glm::length_t row = 0; row < actual[column].length(); ++row)
		{
			EXPECT_NEAR(actual[column][row], expected[column][row], 0.0001f);
		}
	}
}

class CHLApiSetFocusTest: public ::testing::Test
{
protected:
	void SetUp() override
	{
		static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = Arguments {
		    .graphicsBackend = GraphicsBackend::Noop,
		    .gamePath = mockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::warn);
		_game = std::make_unique<Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());

		const auto& function = Locator::chlapi::value().GetFunctionsTable().at(k_SetFocusFunctionId);
		ASSERT_EQ(function.name, "SET_FOCUS");
	}

	void TearDown() override { _game.reset(); }

	static entt::entity CreateObject(const glm::vec3& position, const glm::mat3& rotation)
	{
		auto& registry = Locator::entitiesRegistry::value();
		auto entity = registry.Create();
		if (entity == static_cast<entt::entity>(0))
		{
			entity = registry.Create();
		}
		registry.Assign<Transform>(entity, position, rotation, glm::vec3(1.0f));
		return entity;
	}

	static void InvokeSetFocus(entt::entity entity, const glm::vec3& position)
	{
		auto& vm = Locator::vm::value();
		vm.Pusho(static_cast<uint32_t>(entity));
		vm.Pushv(position.x);
		vm.Pushv(position.y);
		vm.Pushv(position.z);
		Locator::chlapi::value().GetFunctionsTable().at(k_SetFocusFunctionId).impl();
	}

	std::unique_ptr<Game> _game;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CHLApiSetFocusTest, TurnsObjectTowardsPositionOnHorizontalPlane)
{
	const auto entity = CreateObject(glm::vec3(10.0f, 2.0f, 20.0f), glm::mat3(1.0f));

	InvokeSetFocus(entity, glm::vec3(20.0f, 100.0f, 20.0f));

	const auto& transform = Locator::entitiesRegistry::value().Get<const Transform>(entity);
	ExpectMatrixNear(transform.rotation, glm::mat3(glm::eulerAngleY(-glm::half_pi<float>())));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CHLApiSetFocusTest, KeepsRotationWhenFocusHasSameHorizontalPosition)
{
	const auto initialRotation = glm::mat3(glm::eulerAngleY(0.75f));
	const auto entity = CreateObject(glm::vec3(10.0f, 2.0f, 20.0f), initialRotation);

	InvokeSetFocus(entity, glm::vec3(10.0f, 100.0f, 20.0f));

	const auto& transform = Locator::entitiesRegistry::value().Get<const Transform>(entity);
	ExpectMatrixNear(transform.rotation, initialRotation);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CHLApiSetFocusTest, IgnoresMissingObject)
{
	InvokeSetFocus(static_cast<entt::entity>(123456), glm::vec3(20.0f, 2.0f, 20.0f));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CHLApiSetFocusTest, IgnoresNullObject)
{
	InvokeSetFocus(static_cast<entt::entity>(0), glm::vec3(20.0f, 2.0f, 20.0f));
}

} // namespace
