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
typedef uint32_t SkinId;

class L3DModel
{
  public:
	enum class Flags : uint32_t
	{
		Unknown1                 = 1 << 0,
		Unknown2                 = 1 << 1,
		Unknown3                 = 1 << 2,
		Unknown4                 = 1 << 3,
		Unknown5                 = 1 << 4,
		Unknown6                 = 1 << 5,
		Unknown7                 = 1 << 6,
		Unknown8                 = 1 << 7,
		HasBones                 = 1 << 8,
		Unknown10                = 1 << 9,
		Unknown11                = 1 << 10,
		Unknown12                = 1 << 11,
		Unknown13                = 1 << 12,
		NoDraw                   = 1 << 13,
		Unknown15                = 1 << 14,
		ContainsLandscapeFeature = 1 << 15,
		Unknown17                = 1 << 16,
		Unknown18                = 1 << 17,
		ContainsUV2              = 1 << 18,
		ContainsNameData         = 1 << 19,
		ContainsExtraMetrics     = 1 << 20,
		ContainsEBone            = 1 << 21,
		ContainsTnLData          = 1 << 22,
		ContainsNewEP            = 1 << 23,
		Unknown25                = 1 << 24,
		Unknown26                = 1 << 25,
		Unknown27                = 1 << 26,
		Unknown28                = 1 << 27,
		Unknown29                = 1 << 28,
		Unknown30                = 1 << 29,
		Unknown31                = 1 << 30,
	};

	L3DModel()  = default;
	~L3DModel() = default;

	void LoadFromFile(const std::string& fileName);
	void LoadFromL3D(const void* data, size_t size);
	void Draw(ShaderProgram* program) const;

  private:
	uint32_t _flags;

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
	bool IsBoned() const { return _flags & 0x100; }                     // 23
	bool IsNoDraw() const { return _flags & 0x2000; }                   // 18
	bool IsContainsLandscapeFeature() const { return _flags & 0x8000; } // 16
	bool IsContainsUV2() const { return _flags & 0x40000; }             // 13
	bool IsContainsNameData() const { return _flags & 0x80000; }        // 12
	bool IsContainsExtraMetrics() const { return _flags & 0x100000; }   // 11
	bool IsContainsEBone() const { return _flags & 0x200000; }          // 10
	bool IsContainsTnLData() const { return _flags & 0x400000; }        // 9
	bool IsContainsNewEP() const { return _flags & 0x800000; }          // 8

	//const bool IsContainsNewData() const { return _flags & 0xFC8000; } // ???

	std::vector<L3DModel_Bone>& GetBones() { return _bones; }
};
} // namespace OpenBlack
