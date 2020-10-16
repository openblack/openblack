/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
#include <map>
#include <memory>
#include <vector>

namespace openblack
{

class IStream;
class L3DMesh;

namespace pack
{
struct G3DTexture;
}

namespace graphics
{
class Texture2D;
}

class MeshPack
{
	enum class TextureType : uint32_t
	{
		DXT1 = 1,
		DXT3 = 2,
	};

public:
	MeshPack() = default;

	void LoadFromFile(const fs::path& path);

	using MeshesVec = std::vector<std::unique_ptr<L3DMesh>>;
	using TexturesVec = std::vector<std::unique_ptr<graphics::Texture2D>>;

	[[nodiscard]] const MeshesVec& GetMeshes() const { return _meshes; }
	[[nodiscard]] const TexturesVec& GetTextures() const { return _textures; }

	[[nodiscard]] const L3DMesh& GetMesh(int id) const { return *_meshes.at(id); }
	[[nodiscard]] const graphics::Texture2D& GetTexture(int id) const { return *_textures.at(id); }

private:
	void loadTextures(const std::map<std::string, pack::G3DTexture>& textures);
	void loadMeshes(const std::vector<std::vector<uint8_t>>& meshes);

	MeshesVec _meshes;
	TexturesVec _textures;
};
} // namespace openblack
