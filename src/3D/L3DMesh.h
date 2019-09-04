/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Graphics/Mesh.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>
#include <glm/gtc/quaternion.hpp>
#include <map>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
class IStream;

typedef uint32_t SkinId;

enum class L3DMeshFlags : uint32_t
{
	Unknown1                 = 1 << 0,  // 0x1      (31)
	Unknown2                 = 1 << 1,  // 0x2      (30)
	Unknown3                 = 1 << 2,  // 0x4      (29)
	Unknown4                 = 1 << 3,  // 0x8      (28)
	Unknown5                 = 1 << 4,  // 0x10     (27)
	Unknown6                 = 1 << 5,  // 0x20     (26)
	Unknown7                 = 1 << 6,  // 0x40     (25)
	Unknown8                 = 1 << 7,  // 0x80     (24)
	HasBones                 = 1 << 8,  // 0x100    (23)
	Unknown10                = 1 << 9,  // 0x200    (22)
	Unknown11                = 1 << 10, // 0x400    (21)
	Unknown12                = 1 << 11, // 0x800    (20)
	Packed                   = 1 << 12, // 0x1000   (19)
	NoDraw                   = 1 << 13, // 0x2000   (18)
	Unknown15                = 1 << 14, // 0x4000   (17)
	ContainsLandscapeFeature = 1 << 15, // 0x8000   (16)
	Unknown17                = 1 << 16, // 0x10000  (15)
	Unknown18                = 1 << 17, // 0x20000  (14)
	ContainsUV2              = 1 << 18, // 0x40000  (13)
	ContainsNameData         = 1 << 19, // 0x80000  (12)
	ContainsExtraMetrics     = 1 << 20, // 0x100000 (11)
	ContainsEBone            = 1 << 21, // 0x200000 (10)
	ContainsTnLData          = 1 << 22, // 0x400000 (9)
	ContainsNewEP            = 1 << 23, // 0x800000 (8)
	Unknown25                = 1 << 24, // 0x1000000 (7)
	Unknown26                = 1 << 25, // 0x2000000 (6)
	Unknown27                = 1 << 26, // 0x4000000 (5)
	Unknown28                = 1 << 27, // 0x8000000 (4)
	Unknown29                = 1 << 28, // 0x10000000 (3)
	Unknown30                = 1 << 29, // 0x20000000 (2)
	Unknown31                = 1 << 30, // 0x40000000 (1)
	Unknown32                = 1 << 31, // 0x80000000 (0) Possibly contains a nodraw mesh
};

// todo: template this
inline L3DMeshFlags operator&(L3DMeshFlags a, L3DMeshFlags b)
{
	return static_cast<L3DMeshFlags>(static_cast<int>(a) & static_cast<int>(b));
}

class L3DMesh
{
  public:
	L3DMesh()  = default;
	~L3DMesh() = default;

	void LoadFromFile(const std::string& fileName);
	void Load(IStream& stream);
	void LoadFromL3D(const void* data, size_t size);
	void Draw(ShaderProgram* program) const;

  private:
	L3DMeshFlags _flags;

	std::map<SkinId, std::unique_ptr<Texture2D>> _textures;
	// Map submesh index to their required skin IDs
	std::map<int32_t, SkinId> _submeshSkinMap;
	std::vector<std::unique_ptr<Mesh>> _submeshes;

	struct L3DModel_Bone
	{

		int32_t parentBone;  // -1 = root;
		int32_t childBone;   // -1 = no children
		int32_t siblingBone; // -1 = no siblings

		glm::vec3 position;
		glm::quat rotation;
	};

	std::vector<L3DModel_Bone> _bones;
	std::vector<glm::mat4> _boneMatrices;

	void calculateBoneMatrices();

  public:
	bool IsBoned() const { return static_cast<bool>(_flags & L3DMeshFlags::HasBones); }
	bool IsPacked() const { return static_cast<bool>(_flags & L3DMeshFlags::Packed); }
	bool IsNoDraw() const { return static_cast<bool>(_flags & L3DMeshFlags::NoDraw); }
	bool IsContainsLandscapeFeature() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsLandscapeFeature); }
	bool IsContainsUV2() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsUV2); }
	bool IsContainsNameData() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsNameData); }
	bool IsContainsExtraMetrics() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsExtraMetrics); }
	bool IsContainsEBone() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsEBone); }
	bool IsContainsTnLData() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsTnLData); }
	bool IsContainsNewEP() const { return static_cast<bool>(_flags & L3DMeshFlags::ContainsNewEP); }

	//const bool IsContainsNewData() const { return _flags & 0xFC8000; } // ???

	std::vector<L3DModel_Bone>& GetBones() { return _bones; }
};
} // namespace OpenBlack
