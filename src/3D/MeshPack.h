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
#ifndef OPENBLACK_MESHPACK_H
#define OPENBLACK_MESHPACK_H

#include <memory>
#include <unordered_map>
#include <vector>

namespace openblack
{

class IStream;
class L3DMesh;

namespace graphics
{
class Texture2D;
}

class MeshPack
{
	struct LHBlockHeader
	{
		char blockName[32];
		uint32_t blockSize;
		uint32_t position;
	};

	enum class TextureType : uint32_t
	{
		DXT1 = 1,
		DXT3 = 2,
	};

  public:
	MeshPack() = default;

	void Load(IStream&);

	using MeshesVec   = std::vector<std::unique_ptr<L3DMesh>>;
	using TexturesVec = std::vector<std::unique_ptr<graphics::Texture2D>>;

	const MeshesVec& GetMeshes() const { return _meshes; }
	const TexturesVec& GetTextures() const { return _textures; }

	const L3DMesh& GetMesh(int id) const { return *_meshes.at(id); }
	const graphics::Texture2D& GetTexture(int id) const { return *_textures.at(id); }

  private:
	void loadTextures(IStream&);
	void loadMeshes(IStream&);

	MeshesVec _meshes;
	TexturesVec _textures;

	std::unordered_map<std::string, LHBlockHeader> _blocks;
};
} // namespace openblack

#endif // OPENBLACK_MESHPACK_H
