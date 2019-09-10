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

#include <3D/Water.h>
#include <imgui/imgui.h>

using namespace openblack;

glm::mat4 ReflectionCamera::GetViewProjectionMatrix() const
{
	glm::mat4 mRotation = getRotationMatrix();
	glm::mat4 mView     = mRotation * glm::translate(glm::mat4(1.0f), -_position);

	const glm::vec4 reflectionPlane = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	// M''camera = Mreflection * Mcamera * Mflip
	glm::mat4x4 reflectionMatrix;
	reflectMatrix(reflectionMatrix, reflectionPlane);

	return GetProjectionMatrix() * mView * reflectionMatrix;
}

/*
              | 1-2Nx2   -2NxNy  -2NxNz  -2NxD |
Mreflection = |  -2NxNy 1-2Ny2   -2NyNz  -2NyD |
              |  -2NxNz  -2NyNz 1-2Nz2   -2NzD |
              |    0       0       0       1   |
*/
void ReflectionCamera::reflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const
{
	m[0][0] = (1.0f - 2.0f * plane[0] * plane[0]);
	m[1][0] = (-2.0f * plane[0] * plane[1]);
	m[2][0] = (-2.0f * plane[0] * plane[2]);
	m[3][0] = (-2.0f * plane[3] * plane[0]);

	m[0][1] = (-2.0f * plane[1] * plane[0]);
	m[1][1] = (1.0f - 2.0f * plane[1] * plane[1]);
	m[2][1] = (-2.0f * plane[1] * plane[2]);
	m[3][1] = (-2.0f * plane[3] * plane[1]);

	m[0][2] = (-2.0f * plane[2] * plane[0]);
	m[1][2] = (-2.0f * plane[2] * plane[1]);
	m[2][2] = (1.0f - 2.0f * plane[2] * plane[2]);
	m[3][2] = (-2.0f * plane[3] * plane[2]);

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;
}


Water::Water()
{
	// _shaderProgram = std::make_unique<ShaderProgram>();

	// _waterShader = resourceCaches.shaderProgram->Request("water.program");

	_reflectionFrameBuffer = std::make_unique<FrameBuffer>(1024, 1024, GL_RGBA);

	createMesh();
}

void Water::createMesh()
{
	VertexDecl decl;
	decl.reserve(1);
	decl.emplace_back(2, VertexAttrib::Type::Float); // position

	static const glm::vec2 points[] = {
		glm::vec2(-1.0f, 1.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(1.0f, -1.0f),
		glm::vec2(-1.0f, -1.0f),
	};

	static const uint16_t indices[6] = { 2, 1, 0, 0, 3, 2 };

	VertexBuffer* vertexBuffer = new VertexBuffer(points, 4, decl);
	IndexBuffer* indexBuffer   = new IndexBuffer(indices, 6, IndexBuffer::Type::Uint16);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, GL_TRIANGLES);
}

void Water::Draw(ShaderProgram& program) const
{
	program.SetUniformValue("sReflection", 0);
	glActiveTexture(GL_TEXTURE0);
	_reflectionFrameBuffer->GetTexture()->Bind();

	_mesh->Draw();
}

void Water::BeginReflection(const Camera& sceneCamera)
{
	_reflectionCamera = ReflectionCamera(
	    sceneCamera.GetPosition(),
	    sceneCamera.GetRotation(),
	    glm::vec3(0.0f, 1.0f, 0.0f)
	);
	_reflectionCamera.SetProjectionMatrix(sceneCamera.GetProjectionMatrix());

	glViewport(0, 0, _reflectionFrameBuffer->GetWidth(), _reflectionFrameBuffer->GetHeight());

	_reflectionFrameBuffer->Bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Water::EndReflection()
{
	_reflectionFrameBuffer->Unbind();
	// restore original view port
}

void Water::DebugGUI()
{
	ImGui::Begin("Water Debug");
	ImGui::Image((void*)(intptr_t)_reflectionFrameBuffer->GetTexture()->GetNativeHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
}

/*
              | 1-2Nx2   -2NxNy  -2NxNz  -2NxD |
Mreflection = |  -2NxNy 1-2Ny2   -2NyNz  -2NyD |
              |  -2NxNz  -2NyNz 1-2Nz2   -2NzD |
              |    0       0       0       1   |
*/
void Water::ReflectMatrix(glm::mat4x4& m, const glm::vec4& plane)
{
	m[0][0] = (1.0f - 2.0f * plane[0] * plane[0]);
	m[1][0] = (-2.0f * plane[0] * plane[1]);
	m[2][0] = (-2.0f * plane[0] * plane[2]);
	m[3][0] = (-2.0f * plane[3] * plane[0]);

	m[0][1] = (-2.0f * plane[1] * plane[0]);
	m[1][1] = (1.0f - 2.0f * plane[1] * plane[1]);
	m[2][1] = (-2.0f * plane[1] * plane[2]);
	m[3][1] = (-2.0f * plane[3] * plane[1]);

	m[0][2] = (-2.0f * plane[2] * plane[0]);
	m[1][2] = (-2.0f * plane[2] * plane[1]);
	m[2][2] = (1.0f - 2.0f * plane[2] * plane[2]);
	m[3][2] = (-2.0f * plane[3] * plane[2]);

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;
}
