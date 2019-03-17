/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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
#ifndef OPENBLACK_SKINNEDMODEL_H
#define OPENBLACK_SKINNEDMODEL_H

#include <Graphics/ShaderProgram.h>
#include <Graphics/Mesh.h>
#include <Graphics/Texture2D.h>

#include <map>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	class SkinnedModel
	{
	public:
		SkinnedModel() = default;
		~SkinnedModel() = default;

		void LoadFromFile(std::string& fileName);
		void LoadFromL3D(void* data, size_t size);
		void Draw(ShaderProgram* program);
	private:
		std::map<uint32_t, std::unique_ptr<Texture2D>> _textures;
		std::map<uint32_t, uint32_t> _submeshSkinMap;
		std::vector<std::unique_ptr<Mesh>> _submeshes;

		struct SkinnedModel_Bone {

			int32_t parentBone; // -1 = root;
			int32_t childBone; // -1 = no children
			int32_t siblingBone; // -1 = no siblings

			glm::vec3 rotXAxis;
			glm::vec3 rotYAxis;
			glm::vec3 rotZAxis;
			glm::vec3 position;
		};

		std::vector<SkinnedModel_Bone> _bones;
		std::vector<glm::mat4> _boneMatrices;

		void calculateBoneMatrices();
	};
}

#endif
