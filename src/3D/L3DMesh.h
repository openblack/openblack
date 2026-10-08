/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <limits>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include <L3DFile.h>
#include <glm/gtc/quaternion.hpp>

#include "AxisAlignedBoundingBox.h"
#include "Graphics/LightBeams.h"
#include "Graphics/Mesh.h"
#include "Graphics/ShaderProgram.h"
#include "L3DSubMesh.h"
#include "ScreenPick.h"

namespace openblack
{
namespace l3d
{
class L3DFile;
}

constexpr std::array<std::string_view, 32> k_L3DMeshFlagNames {
    "Unknown1",
    "Unknown2",
    "Unknown3",
    "Unknown4",
    "Unknown5",
    "Unknown6",
    "Unknown7",
    "Unknown8",
    "HasBones",
    "Unknown10",
    "Unknown11",
    "HasDoorPosition",
    "Packed",
    "NoDraw",
    "Unknown15",
    "ContainsLandscapeFeature",
    "Unknown17",
    "Unknown18",
    "ContainsUV2",
    "ContainsNameData",
    "ContainsExtraMetrics",
    "ContainsEBone",
    "ContainsTnLData",
    "ContainsNewEP",
    "Unknown25",
    "Unknown26",
    "Unknown27",
    "Unknown28",
    "Unknown29",
    "Unknown30",
    "Unknown31",
    "Unknown32",
};
} // namespace openblack

namespace openblack::graphics
{
class L3DSubMesh;

using SkinId = uint32_t;

// todo: template this
inline l3d::L3DMeshFlags operator&(l3d::L3DMeshFlags a, l3d::L3DMeshFlags b)
{
	return static_cast<l3d::L3DMeshFlags>(static_cast<std::underlying_type<l3d::L3DMeshFlags>::type>(a) &
	                                      static_cast<std::underlying_type<l3d::L3DMeshFlags>::type>(b));
}

class L3DMesh
{
public:
	struct Footprint
	{
		std::unique_ptr<graphics::Texture2D> texture;
		std::unique_ptr<graphics::Mesh> mesh;
	};
	/// The light a window submesh sheds, drawn with one of the mesh's skins
	struct VolumeLight
	{
		SkinId skinID;
		BeamMesh mesh;
	};
	/// A dynamic mesh's vertices and skins can be changed after it is loaded
	explicit L3DMesh(std::string debugName = "", bool dynamic = false) noexcept;
	virtual ~L3DMesh() noexcept;

	bool Load(const l3d::L3DFile& l3d) noexcept;
	bool LoadFromFilesystem(const std::filesystem::path& path) noexcept;
	bool LoadFromFile(const std::filesystem::path& path) noexcept;
	bool LoadFromBuffer(const std::vector<uint8_t>& data) noexcept;
	/// A model made while the game runs from the triangles of another, whose skins it is drawn with (which must outlive
	/// it): one submesh for each group of primitives
	bool LoadMade(const L3DMesh& skinSource, std::span<const std::vector<L3DSubMesh::MadePrimitive>> subMeshes) noexcept;
	/// A dynamic mesh takes its vertices afresh from a file of the same shape
	void UpdateVertices(const l3d::L3DFile& l3d) noexcept;
	/// A dynamic mesh's skin takes its texels afresh
	void UpdateSkin(SkinId skin, std::span<const uint16_t> texels) noexcept;
	[[nodiscard]] bool IsDynamic() const { return _dynamic; }

	[[nodiscard]] uint8_t GetNumSubMeshes() const { return static_cast<uint8_t>(_subMeshes.size()); }
	[[nodiscard]] const std::vector<std::unique_ptr<L3DSubMesh>>& GetSubMeshes() const { return _subMeshes; }
	[[nodiscard]] const std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>>& GetSkins() const
	{
		return _skinSource != nullptr ? _skinSource->GetSkins() : _skins;
	}
	/// The skins' ids in the order the file lists them
	[[nodiscard]] const std::vector<SkinId>& GetSkinOrder() const
	{
		return _skinSource != nullptr ? _skinSource->GetSkinOrder() : _skinOrder;
	}
	/// Where each skin is solid, at 64 by 64 places over it, which the cursor is tested against on a model picked through
	/// its texture's holes
	[[nodiscard]] const screen_pick::AlphaMask* GetSkinMask(SkinId skin) const
	{
		if (_skinSource != nullptr)
		{
			return _skinSource->GetSkinMask(skin);
		}
		const auto found = _skinMasks.find(skin);
		return found != _skinMasks.end() ? &found->second : nullptr;
	}
	[[nodiscard]] const std::vector<Footprint>& GetFootprints() const { return _footprints; }
	[[nodiscard]] const std::vector<VolumeLight>& GetVolumeLights() const { return _volumeLights; }
	[[nodiscard]] const std::vector<uint32_t>& GetBoneParents() const { return _bonesParents; }
	[[nodiscard]] const std::vector<glm::mat4>& GetBoneMatrices() const { return _bonesDefaultMatrices; }
	[[nodiscard]] const std::optional<glm::vec3>& GetDoorPos() const { return _doorPos; }
	/// The top of the chimney the smoke rises from, in the mesh
	[[nodiscard]] const std::optional<glm::vec3>& GetChimneyPos() const { return _chimneyPos; }
	[[nodiscard]] const std::vector<glm::mat4>& GetExtraMetrics() const { return _extraMetrics; }
	/// The triangles of its physics submeshes in the model's space, as the game's physics collides with them
	[[nodiscard]] const std::vector<std::array<glm::vec3, 3>>& GetPhysicsTriangles() const { return _physicsTriangles; }
	/// Every submesh's vertex positions in the model's space and its primitives' triangles, each primitive's indices
	/// counting from its own first vertex, as the game picks a point on a model's surface
	struct SurfacePrimitive
	{
		uint32_t vertexBase;
		uint32_t indexBase;
		uint32_t numTriangles;
	};
	struct Surface
	{
		std::vector<glm::vec3> positions;
		std::vector<uint16_t> indices;
		std::vector<SurfacePrimitive> primitives;
	};
	[[nodiscard]] const std::vector<Surface>& GetSurfaces() const { return _surfaces; }
	[[nodiscard]] AxisAlignedBoundingBox GetBoundingBox() const { return _boundingBox; }
	/// How far along a ray, in the mesh's space, it first meets a submesh drawn of the mesh (as the game picks the
	/// triangle under the mouse while it draws), other than the submeshes left undrawn. Only the temple's rooms keep
	/// their triangles to be picked.
	struct PickHit
	{
		float distance;
		uint32_t subMesh;
	};
	[[nodiscard]] std::optional<PickHit> Pick(glm::vec3 origin, glm::vec3 direction, bool onlyJoints = false,
	                                          bool withoutJoints = false, std::span<const uint32_t> hidden = {}) const;

private:
	l3d::L3DMeshFlags _flags;
	std::string _debugName;
	bool _dynamic;

	std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>> _skins;
	std::vector<SkinId> _skinOrder;
	/// A model made while the game runs is drawn with another's skins
	const L3DMesh* _skinSource {nullptr};
	std::unordered_map<SkinId, screen_pick::AlphaMask> _skinMasks;
	std::vector<Footprint> _footprints; ///< If ContainsLandscapeFeature() is true
	std::vector<VolumeLight> _volumeLights;
	std::vector<std::unique_ptr<L3DSubMesh>> _subMeshes;
	std::vector<uint32_t> _bonesParents;
	std::vector<glm::mat4> _bonesDefaultMatrices;
	std::optional<glm::vec3> _doorPos;
	std::optional<glm::vec3> _chimneyPos;
	std::vector<glm::mat4> _extraMetrics;
	/// Bounding box if no physics mesh was found
	std::vector<std::array<glm::vec3, 3>> _physicsTriangles;
	std::vector<Surface> _surfaces;
	AxisAlignedBoundingBox _boundingBox {
	    {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
	    {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()},
	};
	std::string _nameData;

public:
	[[nodiscard]] const std::string& GetDebugName() const { return _debugName; }
	[[nodiscard]] const std::string& GetNameData() const { return _nameData; }

	[[nodiscard]] uint32_t GetFlags() const { return static_cast<uint32_t>(_flags); }

	[[nodiscard]] bool IsBoned() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::HasBones); }
	[[nodiscard]] bool HasDoorPosition() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::HasDoorPosition); }
	[[nodiscard]] bool HasChimney() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::HasChimney); }
	[[nodiscard]] bool IsPacked() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::Packed); }
	[[nodiscard]] bool IsNoDraw() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::NoDraw); }
	[[nodiscard]] bool ContainsLandscapeFeature() const
	{
		return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsLandscapeFeature);
	}
	[[nodiscard]] bool IsContainsUV2() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsUV2); }
	[[nodiscard]] bool IsContainsNameData() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsNameData); }
	[[nodiscard]] bool ContainsExtraMetrics() const
	{
		return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsExtraMetrics);
	}
	[[nodiscard]] bool IsContainsEBone() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsEBone); }
	[[nodiscard]] bool IsContainsTnLData() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsTnLData); }
	[[nodiscard]] bool IsContainsNewEP() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsNewEP); }
	// const bool IsContainsNewData() const { return _flags & 0xFC8000; } // ???
};
} // namespace openblack::graphics
