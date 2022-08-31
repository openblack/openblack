/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#pragma once

#include <Game.h>
#include <LHScriptX/Script.h>
#include <glm/gtx/string_cast.hpp>
#include <gtest/gtest.h>

namespace glm
{
template <length_t L, typename T, qualifier Q>
std::ostream& operator<<(std::ostream& os, const vec<L, T, Q>& v)
{
	return os << glm::to_string(v);
}
} // namespace glm

class TestGame: public ::testing::Test
{
protected:
	void SetUp() override
	{
		static const auto mockGamePath = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = openblack::Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = mockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::warn);
		_game = std::make_unique<openblack::Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());
	}
	void TearDown() override { _game.reset(); }
	std::unique_ptr<openblack::Game> _game;
};

class TestLoadSceneScript: public TestGame
{
public:
	void LoadTestScene(const char* sceneScript)
	{
		auto script = openblack::lhscriptx::Script(_game.get());
		script.Load(sceneScript);
	}
};
