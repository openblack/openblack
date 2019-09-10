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

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

using namespace openblack::graphics;

namespace openblack
{
namespace graphics
{

class Mesh
{
  public:
	Mesh(VertexBuffer*, const VertexDecl& decl, GLuint type = GL_TRIANGLES);
	Mesh(VertexBuffer*, IndexBuffer*, const VertexDecl& decl, GLuint type = GL_TRIANGLES);
	~Mesh();

	std::shared_ptr<VertexBuffer> GetVertexBuffer();
	std::shared_ptr<IndexBuffer> GetIndexBuffer();

	GLuint GetType() const noexcept;

	void Draw();
	void Draw(uint32_t count, uint32_t startIndex);

  protected:
	std::shared_ptr<VertexBuffer> _vertexBuffer;
	std::shared_ptr<IndexBuffer> _indexBuffer;

  private:
	GLuint _vao;
	GLuint _type;

	VertexDecl _vertexDecl;

	void bindVertexDecl();
};

} // namespace graphics
} // namespace openblack
