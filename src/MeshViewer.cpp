#include "MeshViewer.h"

#include <iostream>
#include <sstream>

#include <SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/oldLH3DIsland.h>

#include <Common/OSFile.h>

#include <Graphics/GameShaders.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/MeshPack.h>

#include <AllMeshes.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

MeshViewer::MeshViewer() : m_currentMesh(0) { }
MeshViewer::~MeshViewer() { }

void MeshViewer::GUI() {
	ImGui::Begin("Mesh Viewer");

	ImGui::PushItemWidth(-1);
	ImGui::ListBox("", &m_currentMesh, gG3DStringList, IM_ARRAYSIZE(gG3DStringList), 8);

	ImGui::Separator();

	L3DModel* model = GetCurrentModel();
	if (model != nullptr) {
		ImGui::Text("# Submeshes: %d", m_meshPack->Models[m_currentMesh]->GetSubMeshCount());
	}

	ImGui::End();
}

void MeshViewer::Render() {
	if (m_meshPack == nullptr) {
		return;
	}

	L3DModel* model = GetCurrentModel();
	if (model == nullptr) {
		return;
	}

	model->Draw();
}

void MeshViewer::LoadPack(std::string filePath)
{
	OSFile* allMeshesFile = new OSFile();
	allMeshesFile->Open(filePath.c_str(), LH_FILE_MODE::Read);
	m_meshPack = new MeshPack(allMeshesFile);
	allMeshesFile->Close();
}

L3DModel* MeshViewer::GetCurrentModel()
{
	if (m_currentMesh < 0 || m_currentMesh >= m_meshPack->GetMeshCount()) {
		return nullptr;
	}

	return m_meshPack->Models[m_currentMesh];
}
