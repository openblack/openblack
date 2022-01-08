/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Graphics/RenderPass.h"
#include "AxisAlignedBoundingBox.h"

#include <L3DFile.h>
#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>

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

	bool Load(const l3d::L3DFile& l3d, uint32_t meshIndex);

	[[nodiscard]] openblack::l3d::L3DSubmeshHeader::Flags GetFlags() const { return _flags; }
	[[nodiscard]] bool isPhysics() const { return _flags.isPhysics; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] AxisAlignedBoundingBox GetBoundingBox() const { return _boundingBox; }
	[[nodiscard]] const std::vector<Primitive>& GetPrimitives() const { return _primitives; }

private:
	L3DMesh& _l3dMesh;

	openblack::l3d::L3DSubmeshHeader::Flags _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;

	AxisAlignedBoundingBox _boundingBox;
};
} // namespace openblack
