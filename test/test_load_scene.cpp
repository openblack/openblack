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
#include <LHScriptX/Script.h>

class LoadScene: public ::testing::Test
{
public:
	void LoadTestScene(const char* sceneScript)
	{
		auto script = openblack::lhscriptx::Script(game_.get());
		script.Load(sceneScript);
	}

protected:
	void SetUp() override
	{
		static const auto mock_game_path = std::filesystem::path(TEST_BINARY_DIR) / "mock";
		auto args = openblack::Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
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
