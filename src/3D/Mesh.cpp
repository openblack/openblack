#include "Mesh.h"

#include <stdint.h>
#include <stdexcept>

using namespace OpenBlack;

struct L3DHeader {
	uint32_t modelSize;
	uint32_t skinOffset;
	uint32_t numMeshes;
	uint32_t meshListOffset; // L3D_Mesh
	uint32_t unknown_1;
	uint32_t unknown_2;
	uint32_t unknown_3;
	uint32_t unknown_4;
	uint32_t unknown_5;
	uint32_t unknown_6;
	uint32_t unknown_7;
	uint32_t unknown_8;
	uint32_t unknown_9;
	uint32_t numSkins;
	uint32_t skinListOffset;
	uint32_t unknown_10;
	uint32_t unknown_11;
	uint32_t pSkinName;
};

struct L3D_Mesh {
	uint8_t unknown_1;
	uint8_t unknown_2; // 20: nodraw
	uint8_t unknown_3;
	uint8_t unknown_4; // e0: transparent

	uint32_t numSubMeshes;
	uint32_t subMeshOffset; // L3D_SubMesh
	uint32_t numBones;
	uint32_t bonesOffset;
};

struct L3D_SubMesh {
	uint32_t unknown_1;
	uint32_t unknown_2;
	int32_t skinID;
	uint32_t unknown_3;

	uint32_t numVerticies;
	uint32_t verticiesOffset;
	uint32_t numTriangles;
	uint32_t trianglesOffset;
	uint32_t boneVertLUTSize;
	uint32_t boneVertLUTOffset;
	uint32_t numVertexBlends;
	uint32_t vertexBlendsOffset;
};

struct L3D_Vertex {
	float position[3];
	float texCoords[2];
	float normal[3];
};

struct L3D_Triangle {
	uint16_t indices[3];
};

Mesh::Mesh() {

}

Mesh::~Mesh() {
	delete m_submeshSizes;

	// add if
	glDeleteBuffers(m_submeshCount, m_vertexBuffers);
	glDeleteBuffers(m_submeshCount, m_indexBuffers);
}

void Mesh::LoadFromL3D(void* data_, size_t size) {
	uint8_t* buffer = static_cast<uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0') {
		throw std::runtime_error("Invalid L3D file");
	}

	L3DHeader* header = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	for (int m = 0; m < header->numMeshes; m++)
	{
		L3D_Mesh* mesh = (L3D_Mesh*)(buffer + meshOffsets[m]);

		m_vertexBuffers = new GLuint[mesh->numSubMeshes];
		m_indexBuffers = new GLuint[mesh->numSubMeshes];

		glGenBuffers(mesh->numSubMeshes, m_vertexBuffers);
		glGenBuffers(mesh->numSubMeshes, m_indexBuffers);
		m_submeshSizes = new GLsizei[mesh->numSubMeshes];
		m_submeshSizesV = new GLsizei[mesh->numSubMeshes];
		m_submeshCount = mesh->numSubMeshes;

		uint32_t* submeshOffsets = (uint32_t*)(buffer + mesh->subMeshOffset);
		for (int sm = 0; sm < mesh->numSubMeshes; sm++)
		{
			L3D_SubMesh* subMesh = (L3D_SubMesh*)(buffer + submeshOffsets[sm]);

			glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffers[sm]);
			glBufferData(GL_ARRAY_BUFFER, subMesh->numVerticies * sizeof(L3D_Vertex), (buffer + subMesh->verticiesOffset), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffers[sm]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, subMesh->numTriangles * 3 * 2, (buffer + subMesh->trianglesOffset), GL_STATIC_DRAW);

			m_submeshSizes[sm] = subMesh->numTriangles * 3;
		}

		// stop idk how we should handle more then 1 mesh yet!
		return;
	}
}

void Mesh::Draw() {
	for (int i = 0; i < m_submeshCount; i++) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffers[i]);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, 0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 32, (void*)12);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 32, (void*)20);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffers[i]);

		glDrawElements(GL_TRIANGLES, m_submeshSizes[i], GL_UNSIGNED_SHORT, 0);
	}
}
