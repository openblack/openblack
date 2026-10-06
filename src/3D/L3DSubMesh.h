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
#include <string>
#include <vector>

#include <L3DFile.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "AxisAlignedBoundingBox.h"

#include "../Graphics/RenderPass.h"

namespace bgfx
{
struct Memory;
}

namespace openblack::graphics
{
class L3DMesh;
class Mesh;
class ShaderProgram;
class Texture2D;

class L3DSubMesh
{
	struct Primitive
	{
		enum class BlendMode : uint8_t
		{
			Disabled,
			Standard, ///< src_alpha, 1 - src_alpha
			Additive, ///< src_alpha, 1
			JustZ,    ///< only the depth is written
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
		/// Drawn from both sides: the game culls the back faces of the rest, as the material's cull mode says
		bool twoSided;
	};

public:
	explicit L3DSubMesh(graphics::L3DMesh& mesh) noexcept;
	~L3DSubMesh() noexcept;

	bool Load(const l3d::L3DFile& l3d, uint32_t meshIndex) noexcept;
	/// A dynamic mesh's submesh takes its vertices afresh from a file of the same shape
	void UpdateVertices(const l3d::L3DFile& l3d) noexcept;

	[[nodiscard]] openblack::l3d::L3DSubmeshHeader::Flags GetFlags() const { return _flags; }
	[[nodiscard]] bool IsPhysics() const { return _flags.isPhysics; }
	[[nodiscard]] graphics::Mesh& GetMesh() const;
	[[nodiscard]] const AxisAlignedBoundingBox& GetBoundingBox() const { return _boundingBox; }
	[[nodiscard]] const std::vector<Primitive>& GetPrimitives() const { return _primitives; }
	/// The lightmap skin the game multiplies the submesh by, twice over, with the vertices' second texture
	/// coordinates. Submeshes without one are drawn as they are.
	[[nodiscard]] std::optional<uint32_t> GetLightmapSkinID() const { return _lightmapSkinID; }
	/// Whether the vertices carry the lightmap coordinates of the mesh, which all of its submeshes do when any has a
	/// lightmap
	[[nodiscard]] bool HasLightmapCoordinates() const { return _hasLightmapCoordinates; }
	/// The matrix of the table of joints the submesh turns by, about its pivot, when it has one
	struct Joint
	{
		uint32_t index;
		glm::vec3 pivot;
	};
	[[nodiscard]] const std::optional<Joint>& GetJoint() const { return _joint; }
	/// The submesh's name, which the temple's rooms find their scrolls and signs by
	[[nodiscard]] const std::string& GetName() const { return _name; }
	/// The frame of the submesh's name record, from the frame to the mesh, and the submesh's box in it
	struct Frame
	{
		glm::mat4 toMesh;
		glm::vec3 min;
		glm::vec3 max;
	};
	[[nodiscard]] const Frame& GetFrame() const { return _frame; }
	/// How far along a ray, in the mesh's space, it first meets the submesh, which the temple's rooms keep the triangles
	/// of to find
	[[nodiscard]] std::optional<float> Pick(glm::vec3 origin, glm::vec3 direction) const;
	/// Whether some of the vertices are blended towards others where the body's parts meet (see vertex_blend). Each
	/// vertex then names its partner in its bone indices' second and its weight, in 32767ths, in their third.
	[[nodiscard]] bool HasBlends() const { return _hasBlends; }
	/// For a boned submesh, every vertex's position and bone index, a texel each in a row, for the vertex shader to place
	/// a blended vertex's partner by. The texels of a creature's variant mesh follow the same order.
	[[nodiscard]] const Texture2D* GetBlendSource() const { return _blendSource.get(); }

private:
	/// The submesh's vertices as they are drawn, in bgfx memory
	[[nodiscard]] const bgfx::Memory* PackVertices(const l3d::L3DFile& l3d, uint32_t meshIndex) const;
	/// The box about the submesh's vertices, as they are placed by its bones
	void BoundVertices(const l3d::L3DFile& l3d, uint32_t meshIndex);
	/// The blend source texture of count packed vertices, where the renderer can read one in the vertex shader
	void CreateBlendSource(const bgfx::Memory* vertices, uint32_t count);

	graphics::L3DMesh& _l3dMesh;
	/// Which of its file's submeshes it is
	uint32_t _meshIndex {0};

	openblack::l3d::L3DSubmeshHeader::Flags _flags;

	std::unique_ptr<graphics::Mesh> _mesh;
	std::vector<Primitive> _primitives;
	std::optional<uint32_t> _lightmapSkinID;
	bool _hasLightmapCoordinates {false};
	std::optional<Joint> _joint;
	std::string _name;
	Frame _frame {glm::mat4(1.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
	/// The corners of each triangle in turn, of the temple's rooms
	std::vector<glm::vec3> _pickTriangles;

	AxisAlignedBoundingBox _boundingBox;
	bool _hasBlends {false};
	std::unique_ptr<Texture2D> _blendSource;
	uint32_t _blendSourceWidth {0};
};
} // namespace openblack::graphics
