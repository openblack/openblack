/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <filesystem>

namespace openblack
{

class Game;

class FotFile
{
public:
	explicit FotFile(Game& game);

	void Load(const std::filesystem::path& path);

private:
	Game& _game;
};

} // namespace openblack
