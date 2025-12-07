/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <3D/AllMeshes.h>
#include <ECS/Archetypes/AbodeArchetype.h>
#include <ECS/Archetypes/TownArchetype.h>
#include <ECS/Archetypes/Utils.h>
#include <ECS/Components/Fixed.h>
#include <ECS/Components/Transform.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <Locator.h>
#include <glm/gtx/string_cast.hpp>
#include <gtest/gtest.h>

#include "third_party/json_helpers.h"
#include "vector_compare.h"

using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack;

class TestFixed: public ::testing::Test
{
protected:
	void SetUp() override
	{
		static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = mockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::warn);
		_game = std::make_unique<Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());
	}
	void TearDown() override { _game.reset(); }
	std::unique_ptr<Game> _game;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(TestFixed, celticTownCentreMeshBoundingCircle)
{
	Transform transform {{}, glm::identity<glm::mat3>(), {1.0f, 1.0f, 1.0f}};
	const auto [center, radius] = GetFixedObstacleBoundingCircle(MeshId::BuildingCelticVillageCentre, transform);
	ASSERT_VEC2_NEAR(center, glm::vec2(0.00188350677f, 0.22130537f));
	ASSERT_EQ(radius, 12.9809408f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(TestFixed, celticTownCentreBoundingCircle)
{
	TownArchetype::Create(0, glm::vec3(2185.72f, 0.0f, 2315.78f), PlayerNames::PLAYER_ONE, Tribe::CELTIC);
	const glm::vec3 pos = {2188.23f, 0.0f, 2317.47f};
	auto entity = AbodeArchetype::Create(0, {2188.23f, 0.0f, 2317.47f}, AbodeInfo::CelticTownCentre, 2.932f, 1.0f, 0, 0);
	const auto& fixed = Locator::entitiesRegistry::value().Get<const Fixed>(entity);
	ASSERT_VEC2_NEAR(fixed.boundingCenter, glm::vec2(2188.182129f, 2317.253906f));
	ASSERT_EQ(fixed.boundingRadius, 12.9809408f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(TestFixed, celticAbodeMeshBoundingCircle)
{
	Transform transform {{}, glm::identity<glm::mat3>(), {1.0f, 1.0f, 1.0f}};
	const auto [center, radius] = GetFixedObstacleBoundingCircle(MeshId::BuildingCeltic5A, transform);
	ASSERT_VEC2_NEAR(center, glm::vec2(0.0883901119f, 0.188308239f));
	ASSERT_EQ(radius, 6.33014917f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(TestFixed, celticAbodeBoundingCircle)
{
	TownArchetype::Create(0, glm::vec3(2185.72f, 0.0f, 2315.78f), PlayerNames::PLAYER_ONE, Tribe::CELTIC);
	auto entity = AbodeArchetype::Create(0, glm::vec3(2224.63f, 0.0f, 2372.52f),
	                                     AbodeInfo::CelticTempleY, // CELTIC_ABODE_F
	                                     2.932f, 1.0f, 0, 0);
	const auto& fixed = Locator::entitiesRegistry::value().Get<const Fixed>(entity);
	ASSERT_VEC2_NEAR(fixed.boundingCenter, glm::vec2(2224.504150f, 2372.354248f));
	ASSERT_FLOAT_EQ(fixed.boundingRadius, 6.33014917f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(TestFixed, celticAbodeBoundingCircleScaled)
{
	const float scale = 1.095f;
	TownArchetype::Create(0, glm::vec3(2185.72f, 0.0f, 2315.78f), PlayerNames::PLAYER_ONE, Tribe::CELTIC);
	auto entity = AbodeArchetype::Create(0, glm::vec3(2224.63f, 0.0f, 2372.52f),
	                                     AbodeInfo::CelticTempleY, // CELTIC_ABODE_F
	                                     2.932f, scale, 0, 0);
	const auto& fixed = Locator::entitiesRegistry::value().Get<const Fixed>(entity);
	ASSERT_VEC2_NEAR(fixed.boundingCenter, glm::vec2(2224.492432f, 2372.338379f));
	ASSERT_FLOAT_EQ(fixed.boundingRadius, 6.33014917f * scale);
}
