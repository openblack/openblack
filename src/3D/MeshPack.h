/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace openblack
{

class IStream;
class L3DMesh;

namespace g3d
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

	void LoadFromFile(const std::string& filename);

	using MeshesVec = std::vector<std::unique_ptr<L3DMesh>>;
	using TexturesVec = std::vector<std::unique_ptr<graphics::Texture2D>>;

	[[nodiscard]] const MeshesVec& GetMeshes() const { return _meshes; }
	[[nodiscard]] const TexturesVec& GetTextures() const { return _textures; }

	[[nodiscard]] const L3DMesh& GetMesh(int id) const { return *_meshes.at(id); }
	[[nodiscard]] const graphics::Texture2D& GetTexture(int id) const { return *_textures.at(id); }

private:
	void loadTextures(const std::map<std::string, g3d::G3DTexture>& textures);
	void loadMeshes(const std::vector<std::vector<uint8_t>>& meshes);

	MeshesVec _meshes;
	TexturesVec _textures;
};
} // namespace openblack
