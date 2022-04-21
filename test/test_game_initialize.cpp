/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Game.h>

TEST(GameInitialize, run_0_frames)
{
	static const auto mock_game_path = std::filesystem::path(TEST_BINARY_DIR) / "mock";
	auto args = openblack::Arguments {
	    .rendererType = bgfx::RendererType::Enum::Noop,
	    .gamePath = mock_game_path.string(),
	    .numFramesToSimulate = 0,
	    .logFile = "stdout",
	};
	std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::debug);
	auto game = std::make_unique<openblack::Game>(std::move(args));
	ASSERT_TRUE(game->Run());
	game.reset();
}
