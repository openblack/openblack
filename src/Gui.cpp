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

#include "Gui.h"

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <imgui/examples/imgui_impl_sdl.h>

#include <Game.h>
#include <GameWindow.h>
#include <LHVMViewer.h>
#include <MeshViewer.h>
#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/MeshPack.h>
#include <3D/Sky.h>
#include <3D/Water.h>

using namespace openblack;

std::unique_ptr<Gui> Gui::create(const GameWindow &window, const SDL_GLContext &context, float scale)
{
	IMGUI_CHECKVERSION();
	auto imgui = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui_ImplSDL2_InitForOpenGL(window.GetHandle(), context);
	{
		style.ScaleAllSizes(scale);
		io.FontGlobalScale = scale;
	}

	ImGui_ImplOpenGL3_Init("#version 130");
	auto meshViewer = std::make_unique<MeshViewer>();
	//meshViewer->Open();

	return std::unique_ptr<Gui>(new Gui(imgui, std::move(meshViewer)));
}

Gui::Gui(ImGuiContext* imgui, std::unique_ptr<MeshViewer>&& meshViewer)
	: _imgui(imgui)
	, _meshViewer(std::move(meshViewer))
{
}

Gui::~Gui()
{
	ImGui::DestroyContext(_imgui);
}

void Gui::ProcessSDLEvent(const SDL_Event& event)
{
	ImGui::SetCurrentContext(_imgui);
	ImGui_ImplSDL2_ProcessEvent(&event);
}

void Gui::Loop(Game& game)
{
	ImGui::SetCurrentContext(_imgui);

	_meshViewer->DrawScene();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(game.GetWindow().GetHandle());
	ImGui::NewFrame();

	auto& config = game.GetConfig();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Quit", "Esc"))
			{
				game.ScheduleQuit();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::Checkbox("Wireframe", &config.wireframe);
			ImGui::Checkbox("Water Debug", &config.waterDebug);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Dump Land Textures"))
			{
				game.GetLandIsland().DumpTextures();
			}
			if (ImGui::MenuItem("Mesh Viewer"))
			{
				_meshViewer->Open();
			}
			ImGui::EndMenu();
		}

		const auto& mousePosition = game.GetMousePosition();
		ImGui::Text("%d, %d", mousePosition.x, mousePosition.y);

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}

	_meshViewer->DrawWindow();

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 8.0f, io.DisplaySize.y - 8.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	static const auto cameraPositionOverlayFlags = 0u |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;
	if (ImGui::Begin("Camera position overlay", nullptr, cameraPositionOverlayFlags))
	{
		auto camPos = game.GetCamera().GetPosition();
		auto camRot = game.GetCamera().GetRotation();
		ImGui::Text("Camera Position: (%.1f,%.1f, %.1f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rotation: (%.1f,%.1f, %.1f)", camRot.x, camRot.y, camRot.z);

		if (ImGui::IsMousePosValid())
			ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
		else
			ImGui::Text("Mouse Position: <invalid>");
	}
	ImGui::End();

	if (game.GetLhvm() != nullptr)
		LHVMViewer::Draw(game.GetLhvm());

	if (ImGui::Begin("Land Island"))
	{
		ImGui::Text("Load Land Island:");

		{
			ImGui::BeginGroup();
			if (ImGui::Button("1"))
			{
				game.LoadLandscape("./Data/Landscape/Land1.lnd");
				game.LoadMap("./Scripts/Land1.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("2"))
			{
				game.LoadLandscape("./Data/Landscape/Land2.lnd");
				game.LoadMap("./Scripts/Land2.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("3"))
			{
				game.LoadLandscape("./Data/Landscape/Land3.lnd");
				game.LoadMap("./Scripts/Land3.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("4"))
			{
				game.LoadLandscape("./Data/Landscape/Land4.lnd");
				game.LoadMap("./Scripts/Land4.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("5"))
			{
				game.LoadLandscape("./Data/Landscape/Land5.lnd");
				game.LoadMap("./Scripts/Land5.txt");
			}

			ImGui::SameLine();
			if (ImGui::Button("T"))
			{
				game.LoadLandscape("./Data/Landscape/LandT.lnd");
				game.LoadMap("./Scripts/LandT.txt");
			}

			if (ImGui::Button("2P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_2P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/TwoGods.txt");
			}

			ImGui::SameLine();
			if (ImGui::Button("3P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_3P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/ThreeGods.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("4P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_4P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/FourGods.txt");
			}

			ImGui::EndGroup();
		}

		if (ImGui::SliderFloat("Day", &config.timeOfDay, 0.0f, 1.0f, "%.3f"))
			Game::instance()->GetSky().SetTime(config.timeOfDay);
		ImGui::SliderFloat("Bump", &config.bumpMapStrength, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Small Bump", &config.smallBumpMapStrength, 0.0f, 1.0f, "%.3f");

		ImGui::Separator();

		ImGui::Text("Block Count: %zu", game.GetLandIsland().GetBlocks().size());
		ImGui::Text("Country Count: %zu", game.GetLandIsland().GetCountries().size());

		ImGui::Separator();

		if (ImGui::Button("Dump Textures"))
			game.GetLandIsland().DumpTextures();

		if (ImGui::Button("Dump Heightmap"))
			game.GetLandIsland().DumpMaps();

	}
	ImGui::End();

	if (config.waterDebug)
		game.GetWater().DebugGUI();

	ImGui::Render();
}

void Gui::Draw()
{
	ImGui::SetCurrentContext(_imgui);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
