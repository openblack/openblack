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

#include "Graphics/Mesh.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "L3DSubMesh.h"

#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

namespace openblack
{
namespace l3d
{
class L3DFile;
}

typedef uint32_t SkinId;

constexpr std::array<std::string_view, 32> L3DMeshFlagNames {
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
    "Unknown12",
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

// todo: template this
inline l3d::L3DMeshFlags operator&(l3d::L3DMeshFlags a, l3d::L3DMeshFlags b)
{
	return static_cast<l3d::L3DMeshFlags>(static_cast<std::underlying_type<l3d::L3DMeshFlags>::type>(a) &
	                                      static_cast<std::underlying_type<l3d::L3DMeshFlags>::type>(b));
}

class L3DMesh
{
public:
	L3DMesh(const std::string& debugName = "");
	~L3DMesh() = default;

	void Load(const l3d::L3DFile& l3d);
	void LoadFromFile(const std::string& fileName);
	void LoadFromBuffer(const std::vector<uint8_t>& data);
	void Draw(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const graphics::ShaderProgram& program, uint32_t mesh,
	          uint64_t state, uint32_t rgba = 0) const;
	void Submit(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const graphics::ShaderProgram& program,
	            uint64_t state, uint32_t rgba = 0) const;
	void Submit(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
	            uint32_t instanceCount, const graphics::ShaderProgram& program, uint64_t state, uint32_t rgba = 0) const;

	[[nodiscard]] uint8_t GetNumSubMeshes() const { return _subMeshes.size(); }
	[[nodiscard]] const std::vector<std::unique_ptr<L3DSubMesh>>& GetSubMeshes() const { return _subMeshes; }
	[[nodiscard]] const std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>>& GetSkins() const { return _skins; }

private:
	l3d::L3DMeshFlags _flags;
	std::string _debugName;

	std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>> _skins;
	std::vector<std::unique_ptr<L3DSubMesh>> _subMeshes;

public:
	[[nodiscard]] const std::string& GetDebugName() const { return _debugName; }

	[[nodiscard]] uint32_t GetFlags() const { return static_cast<uint32_t>(_flags); }

	[[nodiscard]] bool IsBoned() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::HasBones); }
	[[nodiscard]] bool IsPacked() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::Packed); }
	[[nodiscard]] bool IsNoDraw() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::NoDraw); }
	[[nodiscard]] bool IsContainsLandscapeFeature() const
	{
		return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsLandscapeFeature);
	}
	[[nodiscard]] bool IsContainsUV2() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsUV2); }
	[[nodiscard]] bool IsContainsNameData() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsNameData); }
	[[nodiscard]] bool IsContainsExtraMetrics() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsExtraMetrics); }
	[[nodiscard]] bool IsContainsEBone() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsEBone); }
	[[nodiscard]] bool IsContainsTnLData() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsTnLData); }
	[[nodiscard]] bool IsContainsNewEP() const { return static_cast<bool>(_flags & l3d::L3DMeshFlags::ContainsNewEP); }
	// const bool IsContainsNewData() const { return _flags & 0xFC8000; } // ???
};
} // namespace openblack
