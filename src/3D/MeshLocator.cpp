/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshLocator.h"
#include "Game.h"

#include <locale>

using namespace openblack;

bool MeshLocator::LoadLooseMeshFiles(std::filesystem::path path)
{
	_meshes.clear();
	auto& fs = Game::instance()->GetFileSystem();

	if (!fs.Exists(path))
	{
		return false;
	}

	LoadMeshes(fs.GetAllFilePaths(path.string(), ".zzz", true));
	LoadMeshes(fs.GetAllFilePaths(path.string(), ".l3d", true));

	return true;
}
void MeshLocator::LoadMeshes(std::vector<std::filesystem::path> paths)
{
	for (auto& path : paths)
	{
		auto meshId = Game::instance()->GetMeshPack().LoadFromFile(path);
		auto name = path.stem().string();
		LowerCase(name);
		_meshes.emplace(name, std::move(meshId));
	}
}
