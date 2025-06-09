/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <ECS/Archetypes/PlayerArchetype.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <Locator.h>
#include <gtest/gtest.h>

class TestFixed: public ::testing::Test
{
public:
	inline static const auto k_MockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";

protected:
	void SetUp() override
	{
		using namespace openblack;
		auto args = Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = k_MockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::warn);
		SetUpGame(std::move(args));
	}
	void TearDown() override
	{
		using namespace openblack;
		_game->Shutdown();
		Game::ResetGame();
		_game = nullptr;
	}

	void SetUpGame(openblack::Arguments&& args)
	{
		using namespace openblack;
		_game = &Game::CreateGame(std::move(args));
		ASSERT_TRUE(_game->Initialize());
		auto& registry = Locator::entitiesRegistry::value();
		// map gets initialized by game.Run(), before LoadMap, pre-initialize MapContext for testing
		registry.InitMapContext();
		// init. player (hand), needed for some tests
		InitializePlayerHand();
	}

	void InitializePlayerHand()
	{
		using namespace openblack;
		auto& registry = Locator::entitiesRegistry::value();
		auto& gameContext = registry.GameContext();
		// There is always at least one player
		auto playerOne = ecs::archetypes::PlayerArchetype::Create(PlayerNames::PLAYER_ONE);

		// assign Player One Hand as Game Hand
		gameContext.player = playerOne;
	}

	openblack::Game* _game {nullptr};
};
