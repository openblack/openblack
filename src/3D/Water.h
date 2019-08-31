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

#include <3D/Camera.h>
#include <Graphics/Mesh.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/FrameBuffer.h>
#include <glm/glm.hpp>

namespace OpenBlack
{

class ReflectionCamera: public Camera
{
  public:
	ReflectionCamera():
	    ReflectionCamera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)) {}
	ReflectionCamera(glm::vec3 position, glm::vec3 rotation, glm::vec3 reflectionPlane):
	    Camera(position, rotation), _reflectionPlane(reflectionPlane) {}
	glm::mat4 GetViewProjectionMatrix() const;

  private:
	glm::vec3 _reflectionPlane;
	void reflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const;
};

class Water
{
  public:
	Water();
	~Water() = default;

	void Draw(ShaderProgram& program);

	void BeginReflection(const Camera& sceneCamera);
	void EndReflection();

	void DebugGUI();

	ReflectionCamera& GetReflectionCamera() { return _reflectionCamera; }

	void ReflectMatrix(glm::mat4x4& m, const glm::vec4& plane);

  private:
	struct WaterVertex
	{
		glm::vec2 position;
	};

	void createMesh();

	std::unique_ptr<Mesh> _mesh;
	std::unique_ptr<ShaderProgram> _shaderProgram;
	std::unique_ptr<FrameBuffer> _reflectionFrameBuffer;

	ReflectionCamera _reflectionCamera;
};

} // namespace OpenBlack
