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

#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>
#include <vector>

using namespace openblack::Graphics;

namespace openblack
{
namespace Graphics
{

struct VertexAttrib
{
	GLuint index;         ///< Index of the vertex attribute.
	GLint size;           ///< Number of components per vertex attribute, must be 1, 2, 3, 4.
	GLenum type;          ///< Data type of each attribute component in the array.
	GLsizei stride;       ///< Byte offset between consecutive vertex attributes.
	const GLvoid* offset; ///< Offset of the first component of the first generic vertex attribute.
	bool normalized;
	bool integer;

	VertexAttrib() {}
	VertexAttrib(GLuint i, GLint s, GLenum t, GLsizei st = 0, const GLvoid* of = 0):
	    index(i), size(s), type(t), stride(st), offset(of), normalized(false), integer(false) {}
	VertexAttrib(GLuint i, GLint s, GLenum t, bool intg, bool norm, GLsizei st = 0, const GLvoid* of = 0):
	    index(i), size(s), type(t), stride(st), offset(of), normalized(norm), integer(intg) {}
};

typedef std::vector<VertexAttrib> VertexDecl;

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

  protected:
	std::shared_ptr<VertexBuffer> _vertexBuffer;
	std::shared_ptr<IndexBuffer> _indexBuffer;

  private:
	GLuint _vao;
	GLuint _type;

	VertexDecl _vertexDecl;

	void bindVertexDecl();
};

} // namespace Graphics
} // namespace openblack
