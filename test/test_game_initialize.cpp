/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include <Game.h>
#include <gtest/gtest.h>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(GameInitialize, initializeOnly)
{
	static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
	auto args = openblack::Arguments {
	    .rendererType = bgfx::RendererType::Enum::Noop,
	    .gamePath = mockGamePath.string(),
	    .numFramesToSimulate = 0,
	    .logFile = "stdout",
	};
	std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::debug);
	auto game = std::make_unique<openblack::Game>(std::move(args));
	ASSERT_TRUE(game->Initialize());
	game.reset();
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): external macro
TEST(GameInitialize, run0Frames)
{
	static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
	auto args = openblack::Arguments {
	    .rendererType = bgfx::RendererType::Enum::Noop,
	    .gamePath = mockGamePath.string(),
	    .numFramesToSimulate = 0,
	    .logFile = "stdout",
	};
	std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::debug);
	auto game = std::make_unique<openblack::Game>(std::move(args));
	ASSERT_TRUE(game->Initialize());
	ASSERT_TRUE(game->Run());
	game.reset();
}
