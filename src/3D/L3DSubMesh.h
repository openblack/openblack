/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <optional>
#include <vector>

#include <L3DFile.h>

#include "AxisAlignedBoundingBox.h"

#include "../Graphics/RenderPass.h"

namespace openblack::graphics
{
class L3DMesh;
class Mesh;
class ShaderProgram;

class L3DSubMesh
{
	struct Primitive
	{
		enum class BlendMode : uint8_t
		{
			Disabled,
			Standard, ///< src_alpha, 1 - src_alpha
			Additive, ///< src_alpha, 1
		};

		uint32_t skinID;
		uint32_t indicesOffset;
		uint32_t indicesCount;
		bool depthWrite;
		bool alphaTest;
		BlendMode blend;
		bool modulateAlpha;  ///< Multiply ouput alpha by a uniform
		bool thresholdAlpha; ///< Dismiss fragments below a certain threshold
		float alphaCutoutThreshold;
	};

public:
	explicit L3DSubMesh(graphics::L3DMesh& mesh) noexcept;
	~L3DSubMesh() noexcept;

	bool Load(const l3d::L3DFile& l3d, uint32_t meshIndex) noexcept;

	[[nodiscard]] openblack::l3d::L3DSubmeshHeader::Flags GetFlags() const { return _flags; }
	[[nodiscard]] bool IsPhysics() const { return _flags.isPhysics; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] const AxisAlignedBoundingBox& GetBoundingBox() const { return _boundingBox; }
	[[nodiscard]] const std::vector<Primitive>& GetPrimitives() const { return _primitives; }
	/// The skin LH3DMesh::DrawLightMap multiplies the submesh by, twice over, with the vertices' second texture
	/// coordinates. Submeshes without one are drawn as they are.
	[[nodiscard]] std::optional<uint32_t> GetLightmapSkinID() const { return _lightmapSkinID; }
	/// Whether the vertices carry the lightmap coordinates of the mesh, which all of its submeshes do when any has a
	/// lightmap
	[[nodiscard]] bool HasLightmapCoordinates() const { return _hasLightmapCoordinates; }

private:
	graphics::L3DMesh& _l3dMesh;

	openblack::l3d::L3DSubmeshHeader::Flags _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;
	std::optional<uint32_t> _lightmapSkinID;
	bool _hasLightmapCoordinates {false};

	AxisAlignedBoundingBox _boundingBox;
};
} // namespace openblack::graphics
