#include <3D/Water.h>

using namespace OpenBlack;

Water::Water()
{
	createMesh();
}

void Water::createMesh()
{
	VertexDecl decl(1);
	decl[0] = VertexAttrib(0, 3, GL_FLOAT, false, false, sizeof(WaterVertex), nullptr); // position

	std::vector<WaterVertex> verts(4);
	verts[0] = WaterVertex({ glm::vec3(-12440.0f, 0.0f, -12440.0f) });
	verts[1] = WaterVertex({ glm::vec3(17560.0f,  0.0f, -12440.0f) });
	verts[2] = WaterVertex({ glm::vec3(17560.0f, 0.0f, 17560.0f) });
	verts[3] = WaterVertex({ glm::vec3(-12440.0f, 0.0f, 17560.0f) });

	std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

	VertexBuffer* vertexBuffer = new VertexBuffer(verts.data(), verts.size(), sizeof(WaterVertex));
	IndexBuffer* indexBuffer = new IndexBuffer(indices.data(), indices.size(), GL_UNSIGNED_SHORT);

	_mesh = std::make_unique<Mesh>(vertexBuffer, indexBuffer, decl, GL_TRIANGLES);
}


void Water::Draw()
{
	_mesh->Draw();
}
