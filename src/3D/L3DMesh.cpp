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

#include "L3DMesh.h"

#include "3D/MeshPack.h"
#include "Common/FileSystem.h"
#include "Common/IStream.h"
#include "Game.h"

#include <glm/gtc/type_ptr.hpp>
#include <l3d_file.h>
#include <spdlog/spdlog.h>

#include <array>
#include <stdexcept>

using namespace openblack;
using namespace openblack::graphics;

struct LH3D_BoneVert
{
	uint16_t nVertices;
	uint16_t boneIndex;
};

struct L3DModel_Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 norm;
	uint32_t bone;
};

L3DMesh::L3DMesh(const std::string& debugName): _flags(static_cast<L3DMeshFlags>(0)), _debugName(debugName) {}

void L3DMesh::Load(const l3d::L3DFile& l3d)
{
	_flags = static_cast<L3DMeshFlags>(l3d.GetHeader().flags);
	for (auto& skin : l3d.GetSkins())
	{
		_skins[skin.id] = std::make_unique<Texture2D>(_debugName.c_str());
		_skins[skin.id]->Create(skin.width, skin.height, 1, Format::RGBA4, Wrapping::ClampEdge, skin.texels.data(),
		                        skin.texels.size() * sizeof(skin.texels[0]));
	}

	_subMeshes.resize(l3d.GetSubmeshHeaders().size());
	for (size_t i = 0; i < _subMeshes.size(); ++i)
	{
		_subMeshes[i] = std::make_unique<L3DSubMesh>(*this);
		_subMeshes[i]->Load(l3d, i);
	}
	// TODO(bwrsandman): store vertex and index buffers at mesh level
	bgfx::frame();
}

void L3DMesh::LoadFromFile(const std::string& fileName)
{
	spdlog::debug("Loading L3DMesh from file: {}", fileName);
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(Game::instance()->GetFileSystem().FindPath(fileName).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open l3d mesh from filesystem {}: {}", fileName, err.what());
		return;
	}

	Load(l3d);
}

void L3DMesh::LoadFromBuffer(const std::vector<uint8_t>& data)
{
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(data);
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open l3d mesh from buffer: {}", err.what());
		return;
	}

	Load(l3d);
}

void L3DMesh::Draw(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const ShaderProgram& program, uint32_t mesh,
                   uint64_t state, uint32_t rgba) const
{
	if (_subMeshes.empty())
	{
		spdlog::warn("Mesh {} has no submeshes to draw", _debugName);
		return;
	}
	if (mesh >= _subMeshes.size())
	{
		spdlog::warn("tried to draw submesh out of range ({}/{})", mesh, _subMeshes.size());
	}

	_subMeshes[mesh]->Submit(viewId, modelMatrix, program, state, rgba, false);
}

void L3DMesh::Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const ShaderProgram& program, uint64_t state,
                     uint32_t rgba) const
{
	for (auto it = _subMeshes.begin(); it != _subMeshes.end(); ++it)
	{
		const L3DSubMesh& submesh = *it->get();
		submesh.Submit(viewId, modelMatrix, program, state, rgba, std::next(it) != _subMeshes.end());
	}
}

void L3DMesh::Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
                     uint32_t instanceCount, const ShaderProgram& program, uint64_t state, uint32_t rgba) const
{
	for (auto it = _subMeshes.begin(); it != _subMeshes.end(); ++it)
	{
		const L3DSubMesh& submesh = *it->get();
		submesh.Submit(viewId, instanceBuffer, instanceStart, instanceCount, program, state, rgba,
		               std::next(it) != _subMeshes.end());
	}
}
