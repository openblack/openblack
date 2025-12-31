/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <ECS/Archetypes/PlayerArchetype.h>
#include <Game.h>
#include <LHScriptX/Script.h>
#include <Locator.h>
#include <gtest/gtest.h>

#include "test_fixed.h"

class LoadScene: public TestFixed
{
protected:
	void SetUp() override
	{
		using namespace openblack;
		auto args = openblack::Arguments {
		    .rendererType = bgfx::RendererType::Enum::Noop,
		    .gamePath = k_MockGamePath.string(),
		    .numFramesToSimulate = 0,
		    .logFile = "stdout",
		};
		std::fill_n(args.logLevels.begin(), args.logLevels.size(), spdlog::level::debug);
		SetUpGame(std::move(args));
	}

public:
	void LoadTestScene(const char* sceneScript)
	{
		openblack::lhscriptx::Script script;
		script.Load(sceneScript);
	}
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
