/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Graphics/RenderPass.h"
#include "AxisAlignedBoundingBox.h"

#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <l3d_file.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace openblack
{
class L3DMesh;
struct L3DMeshSubmitDesc;

namespace graphics
{
class Mesh;
class ShaderProgram;
} // namespace graphics

class L3DSubMesh
{
	struct Primitive
	{
		uint32_t skinID;
		uint32_t indicesOffset;
		uint32_t indicesCount;
	};

public:
	explicit L3DSubMesh(L3DMesh& mesh);
	~L3DSubMesh();

	void Load(const l3d::L3DFile& l3d, uint32_t meshIndex);
	void Submit(const L3DMeshSubmitDesc& desc, graphics::RenderPass viewId, const graphics::ShaderProgram& program,
	            uint64_t state, uint32_t rgba = 0, bool preserveState = false) const;

	[[nodiscard]] openblack::l3d::L3DSubmeshHeader::Flags GetFlags() const { return _flags; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] AxisAlignedBoundingBox GetBoundingBox() const { return _boundingBox; }

private:
	L3DMesh& _l3dMesh;

	openblack::l3d::L3DSubmeshHeader::Flags _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;

	AxisAlignedBoundingBox _boundingBox;
};
} // namespace openblack
