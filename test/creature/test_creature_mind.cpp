/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "Creature/CreatureMind.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureState.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/CreatureMindSystem.h"
#include "Enums.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/Loaders.h"
#include "Resources/Resources.h"

namespace
{

using namespace openblack;
using namespace openblack::creature;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using namespace openblack::resources;

constexpr size_t k_CreatureTypeCount = static_cast<size_t>(CreatureType::_COUNT) - 1;

class CreatureMindTest: public testing::Test
{
protected:
	void SetUp() override
	{
		spdlog::drop("game");
		spdlog::null_logger_mt("game");

		auto info = std::make_unique<InfoConstants>();
		_info = info.get();
		Locator::infoConstants::reset(info.release());
	}

	void TearDown() override
	{
		Locator::resources::reset();
		Locator::entitiesRegistry::reset();
		Locator::infoConstants::reset();
		spdlog::drop("game");
	}

	InfoConstants* _info = nullptr;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, LoaderMapsEachCreatureTypeToItsInfoConstantsColumn)
{
	const CreatureMindLoader loader;

	for (size_t speciesIndex = 0; speciesIndex < k_CreatureTypeCount; ++speciesIndex)
	{
		SCOPED_TRACE(speciesIndex);
		for (size_t desireIndex = 0; desireIndex < _info->creatureDesireForType.size(); ++desireIndex)
		{
			auto& values = _info->creatureDesireForType.at(desireIndex).byCreatureType;
			values.fill(-1.0f);
			values.at(speciesIndex) = static_cast<float>(100 * speciesIndex + desireIndex);
		}

		const auto creatureType = static_cast<CreatureType>(speciesIndex + 1);
		const auto mind =
		    loader(CreatureMindLoader::FromDiskTag {}, std::filesystem::path("ComputerControlledCreature"), creatureType);
		for (size_t desireIndex = 0; desireIndex < mind->baseDesireWeights.size(); ++desireIndex)
		{
			EXPECT_FLOAT_EQ(mind->baseDesireWeights.at(desireIndex), static_cast<float>(100 * speciesIndex + desireIndex));
		}
	}
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, LoaderRejectsUnknownCreatureTypeWithoutCachingIt)
{
	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
	const auto identifier = CreatureMindLoader::Identifier("ComputerControlledCreature", CreatureType::Unknown);

	EXPECT_THROW(
	    minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, "ComputerControlledCreature", CreatureType::Unknown),
	    std::runtime_error);
	EXPECT_FALSE(minds.Contains(identifier));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, FailedLoadCanRecoverAfterInfoConstantsBecomesAvailable)
{
	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
	const auto identifier = CreatureMindLoader::Identifier("ComputerControlledCreature", CreatureType::Cow);
	Locator::infoConstants::reset();
	_info = nullptr;

	EXPECT_THROW(minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, "ComputerControlledCreature", CreatureType::Cow),
	             std::runtime_error);
	EXPECT_FALSE(minds.Contains(identifier));

	auto info = std::make_unique<InfoConstants>();
	for (auto& desire : info->creatureDesireForType)
	{
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Cow) - 1) = 2.5f;
	}
	_info = info.get();
	Locator::infoConstants::reset(info.release());

	const auto result =
	    minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, "ComputerControlledCreature", CreatureType::Cow);
	EXPECT_TRUE(result.second);
	EXPECT_TRUE(minds.Contains(identifier));
	EXPECT_FLOAT_EQ(result.first->second->baseDesireWeights.front(), 2.5f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, CacheRetainsDistinctWeightsForTheSameMindFileByCreatureType)
{
	for (auto& desire : _info->creatureDesireForType)
	{
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Cow) - 1) = 1.25f;
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Tiger) - 1) = 2.5f;
	}

	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
	constexpr std::string_view k_MindName = "ComputerControlledCreature";
	const auto cowResult = minds.Load(CreatureMindLoader::Identifier(k_MindName, CreatureType::Cow),
	                                  CreatureMindLoader::FromDiskTag {}, k_MindName, CreatureType::Cow);
	const auto cowMindId = cowResult.first->first;
	const auto cowInserted = cowResult.second;
	const auto tigerResult = minds.Load(CreatureMindLoader::Identifier(k_MindName, CreatureType::Tiger),
	                                    CreatureMindLoader::FromDiskTag {}, k_MindName, CreatureType::Tiger);
	const auto tigerMindId = tigerResult.first->first;

	EXPECT_TRUE(cowInserted);
	EXPECT_TRUE(tigerResult.second);
	EXPECT_NE(cowMindId, tigerMindId);
	EXPECT_EQ(minds.Size(), 2);
	EXPECT_FLOAT_EQ(minds.Handle(cowMindId)->baseDesireWeights.front(), 1.25f);
	EXPECT_FLOAT_EQ(minds.Handle(tigerMindId)->baseDesireWeights.front(), 2.5f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, SystemCopiesLoadedMindWeightsIntoCreatureState)
{
	for (size_t desireIndex = 0; desireIndex < _info->creatureDesireForType.size(); ++desireIndex)
	{
		auto& values = _info->creatureDesireForType.at(desireIndex).byCreatureType;
		values.fill(0.0f);
		values.at(static_cast<size_t>(CreatureType::Cow) - 1) = static_cast<float>(desireIndex) + 0.25f;
	}

	Locator::resources::emplace<Resources>();
	Locator::entitiesRegistry::emplace<ecs::Registry>();

	auto& minds = Locator::resources::value().GetCreatureMinds();
	const auto result = minds.Load(CreatureMindLoader::Identifier("ComputerControlledCreature", CreatureType::Cow),
	                               CreatureMindLoader::FromDiskTag {}, "ComputerControlledCreature", CreatureType::Cow);
	const entt::id_type mindId = result.first->first;

	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<Creature>(entity, PlayerNames::PLAYER_ONE, CreatureType::Cow, mindId);
	registry.Assign<CreatureState>(entity);

	CreatureMindSystem {}.Update();

	const auto& state = registry.Get<CreatureState>(entity);
	EXPECT_TRUE(state.initialized);
	EXPECT_EQ(state.mind, mindId);
	for (size_t desireIndex = 0; desireIndex < state.desireWeights.size(); ++desireIndex)
	{
		EXPECT_FLOAT_EQ(state.desireWeights.at(desireIndex), static_cast<float>(desireIndex) + 0.25f);
	}
}

} // namespace
