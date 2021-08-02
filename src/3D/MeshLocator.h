/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "L3DMesh.h"
#include "MeshPack.h"
#include "Common/FileSystem.h"

#include <map>

namespace openblack
{
class MeshLocator
{
public:
	bool LoadLooseMeshFiles(std::filesystem::path path);
	std::map<std::string, MeshId>& GetMeshes() { return _meshes; };
private:
	void LoadMeshes(std::vector<std::filesystem::path> paths);
	std::map<std::string, MeshId> _meshes;
};
}