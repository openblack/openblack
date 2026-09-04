/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <array>
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

struct VanillaColumn
{
	CreatureType creatureType;
	size_t column;
};

// Expected column of each openblack creature type in InfoConstants::creatureDesireForType, written out by hand from
// vanilla's CREATURE_TYPE order (Ape = 0, Cow = 1 ... Gorilla = 16) so that it does not mirror VanillaCreatureTypeIndex.
constexpr std::array<VanillaColumn, k_CreatureDesireForTypeColumns> k_VanillaColumns = {{
    {CreatureType::GiantApe, 0},
    {CreatureType::Cow, 1},
    {CreatureType::Tiger, 2},
    {CreatureType::Leopard, 3},
    {CreatureType::Wolf, 4},
    {CreatureType::Lion, 5},
    {CreatureType::Horse, 6},
    {CreatureType::Tortoise, 7},
    {CreatureType::Zebra, 8},
    {CreatureType::BrownBear, 9},
    {CreatureType::PolarBear, 10},
    {CreatureType::Sheep, 11},
    {CreatureType::Chimp, 12},
    {CreatureType::Ogre, 13},
    {CreatureType::Mandrill, 14},
    {CreatureType::Rhino, 15},
    {CreatureType::Gorilla, 16},
}};
static_assert(k_VanillaColumns.size() == static_cast<size_t>(CreatureType::_COUNT) - 1);

constexpr std::string_view k_MindName = "ComputerControlledCreature";
constexpr float k_UnusedColumnSentinel = -1.0f;

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
TEST_F(CreatureMindTest, VanillaCreatureTypeIndexMatchesHandWrittenColumns)
{
	std::array<bool, k_CreatureDesireForTypeColumns> columnSeen {};
	for (const auto& [creatureType, column] : k_VanillaColumns)
	{
		SCOPED_TRACE(static_cast<int>(creatureType));
		EXPECT_EQ(VanillaCreatureTypeIndex(creatureType), column);
		EXPECT_FALSE(columnSeen.at(column));
		columnSeen.at(column) = true;
	}
	EXPECT_TRUE(std::all_of(columnSeen.begin(), columnSeen.end(), [](bool seen) { return seen; }));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, LoaderUsesVanillaCreatureTypeColumns)
{
	const CreatureMindLoader loader;

	for (const auto& [creatureType, column] : k_VanillaColumns)
	{
		SCOPED_TRACE(static_cast<int>(creatureType));
		for (size_t desireIndex = 0; desireIndex < _info->creatureDesireForType.size(); ++desireIndex)
		{
			auto& values = _info->creatureDesireForType.at(desireIndex).byCreatureType;
			values.fill(k_UnusedColumnSentinel);
			values.at(column) = static_cast<float>(100 * column + desireIndex);
		}

		const auto mind = loader(CreatureMindLoader::FromDiskTag {}, std::filesystem::path(k_MindName), creatureType);
		for (size_t desireIndex = 0; desireIndex < mind->desireIncreaseTimes.size(); ++desireIndex)
		{
			EXPECT_FLOAT_EQ(mind->desireIncreaseTimes.at(desireIndex), static_cast<float>(100 * column + desireIndex));
		}
	}
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, LoaderRejectsUnknownCreatureTypeWithoutCachingIt)
{
	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
	const auto identifier = CreatureMindLoader::Identifier(k_MindName, CreatureType::Unknown);

	EXPECT_THROW(minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, k_MindName, CreatureType::Unknown),
	             std::runtime_error);
	EXPECT_FALSE(minds.Contains(identifier));
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, FailedLoadCanRecoverAfterInfoConstantsBecomesAvailable)
{
	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
	const auto identifier = CreatureMindLoader::Identifier(k_MindName, CreatureType::Cow);
	Locator::infoConstants::reset();
	_info = nullptr;

	EXPECT_THROW(minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, k_MindName, CreatureType::Cow), std::runtime_error);
	EXPECT_FALSE(minds.Contains(identifier));

	auto info = std::make_unique<InfoConstants>();
	for (auto& desire : info->creatureDesireForType)
	{
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Cow)) = 2.5f;
	}
	_info = info.get();
	Locator::infoConstants::reset(info.release());

	const auto result = minds.Load(identifier, CreatureMindLoader::FromDiskTag {}, k_MindName, CreatureType::Cow);
	EXPECT_TRUE(result.second);
	EXPECT_TRUE(minds.Contains(identifier));
	EXPECT_FLOAT_EQ(result.first->second->desireIncreaseTimes.front(), 2.5f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, CacheRetainsDistinctIncreaseTimesForTheSameMindFileByCreatureType)
{
	for (auto& desire : _info->creatureDesireForType)
	{
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Cow)) = 1.25f;
		desire.byCreatureType.at(static_cast<size_t>(CreatureType::Tiger)) = 2.5f;
	}

	Locator::resources::emplace<Resources>();
	auto& minds = Locator::resources::value().GetCreatureMinds();
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
	EXPECT_FLOAT_EQ(minds.Handle(cowMindId)->desireIncreaseTimes.front(), 1.25f);
	EXPECT_FLOAT_EQ(minds.Handle(tigerMindId)->desireIncreaseTimes.front(), 2.5f);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST_F(CreatureMindTest, SystemCopiesLoadedMindIncreaseTimesIntoCreatureState)
{
	for (size_t desireIndex = 0; desireIndex < _info->creatureDesireForType.size(); ++desireIndex)
	{
		auto& values = _info->creatureDesireForType.at(desireIndex).byCreatureType;
		values.fill(0.0f);
		values.at(static_cast<size_t>(CreatureType::Cow)) = static_cast<float>(desireIndex) + 0.25f;
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
	for (size_t desireIndex = 0; desireIndex < state.desireIncreaseTimes.size(); ++desireIndex)
	{
		EXPECT_FLOAT_EQ(state.desireIncreaseTimes.at(desireIndex), static_cast<float>(desireIndex) + 0.25f);
	}
}

} // namespace
