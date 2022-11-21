/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Gui.h"

#include <cinttypes>
#include <cmath>

#include <bgfx/embedded_shader.h>
// BGFX has support for WSL to use windows d3d. We disable it here from the BGFX_EMBEDDED_SHADER macro.
#if BX_PLATFORM_LINUX
#undef BGFX_EMBEDDED_SHADER_DXBC
#define BGFX_EMBEDDED_SHADER_DXBC(...)
#undef BGFX_EMBEDDED_SHADER_DX9BC
#define BGFX_EMBEDDED_SHADER_DX9BC(...)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <bx/math.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <SDL.h>
#include <bx/timer.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_user.h>
#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#endif

#include <3D/Camera.h>
#include <3D/Sky.h>
#include <Common/FileSystem.h>
#include <ECS/Components/LivingAction.h>
#include <ECS/Components/Transform.h>
#include <ECS/Components/Villager.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <GameWindow.h>
#include <Locator.h>
#include <Resources/Resources.h>

#include "Console.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "LHVMViewer.h"
#include "LandIsland.h"
#include "MeshViewer.h"
#include "PathFinding.h"
#include "Profiler.h"
#include "TextureViewer.h"

// Turn off formatting because it adds spaces which break the stringifying
// clang-format off
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define IMGUI_SHADER_DIR imgui/
// clang-format on
#define SHADER_NAME vs_imgui_image
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"
#define SHADER_DIR IMGUI_SHADER_DIR
#define SHADER_NAME fs_imgui_image
#include "Graphics/ShaderIncluder.h"

#define SHADER_NAME vs_ocornut_imgui
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"
#define SHADER_NAME fs_ocornut_imgui
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
const std::array<bgfx::EmbeddedShader, 5> k_EmbeddedShaders = {{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END(),
}};
} // namespace

std::unique_ptr<Gui> Gui::Create(const GameWindow* window, graphics::RenderPass viewId, float scale)
{
	IMGUI_CHECKVERSION();
	auto* imgui = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale);
	io.FontGlobalScale = scale;
	io.BackendRendererName = "imgui_impl_bgfx";

	std::vector<std::unique_ptr<Window>> debugWindows;
	debugWindows.emplace_back(new Profiler);
	debugWindows.emplace_back(new MeshViewer);
	debugWindows.emplace_back(new TextureViewer);
	debugWindows.emplace_back(new Console);
	debugWindows.emplace_back(new LandIsland);
	debugWindows.emplace_back(new LHVMViewer);
	debugWindows.emplace_back(new PathFinding);

	auto gui =
	    std::unique_ptr<Gui>(new Gui(imgui, static_cast<bgfx::ViewId>(viewId), std::move(debugWindows), window == nullptr));

	if (window != nullptr)
	{
		if (!ImGui_ImplSDL2_InitForSDLRenderer(window->GetHandle(), nullptr))
		{
			return nullptr;
		}
	}

	return gui;
}

Gui::Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::vector<std::unique_ptr<Window>>&& debugWindows, bool headless)
    : _imgui(imgui)
    , _headless(headless)
    , _program(BGFX_INVALID_HANDLE)
    , _imageProgram(BGFX_INVALID_HANDLE)
    , _texture(BGFX_INVALID_HANDLE)
    , _s_tex(BGFX_INVALID_HANDLE)
    , _u_imageLodEnabled(BGFX_INVALID_HANDLE)
    , _viewId(viewId)
    , _debugWindows(std::move(debugWindows))
{
	CreateDeviceObjectsBgfx();
}

Gui::~Gui()
{
	if (!_headless)
	{
		ImGui_ImplSDL2_Shutdown();
	}
	ImGui::DestroyContext(_imgui);

	if (bgfx::isValid(_u_imageLodEnabled))
	{
		bgfx::destroy(_u_imageLodEnabled);
	}
	if (bgfx::isValid(_s_tex))
	{
		bgfx::destroy(_s_tex);
	}
	if (bgfx::isValid(_texture))
	{
		bgfx::destroy(_texture);
	}
	if (bgfx::isValid(_imageProgram))
	{
		bgfx::destroy(_imageProgram);
	}
	if (bgfx::isValid(_program))
	{
		bgfx::destroy(_program);
	}
}

bool Gui::ProcessEvents(const SDL_Event& event)
{
	ImGui::SetCurrentContext(_imgui);

	for (auto& window : _debugWindows)
	{
		window->WindowProcessEvent(event);
	}

	ImGui_ImplSDL2_ProcessEvent(&event);

	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_TEXTINPUT:
		return io.WantTextInput;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		return io.WantCaptureKeyboard;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			return false;
		}
		break;
	}
	return io.WantCaptureMouse;
}

bool Gui::CreateFontsTextureBgfx()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width;
	int height;
	// Load as RGBA 32-bits (75% of the memory is wasted, but
	// default font is so small) because it is more likely to
	// be compatible with user's existing shaders. If your
	// ImTextureId represent a higher-level concept than just
	// a GL texture id, consider calling GetTexDataAsAlpha8()
	// instead to save on GPU memory.
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	_texture = bgfx::createTexture2D(static_cast<uint16_t>(width), static_cast<uint16_t>(height), false, 1,
	                                 bgfx::TextureFormat::BGRA8, 0, bgfx::copy(pixels, width * height * 4));

	return true;
}

bool Gui::CreateDeviceObjectsBgfx()
{
	// Create shaders
	bgfx::RendererType::Enum type = bgfx::getRendererType();
	_program = bgfx::createProgram(bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "vs_ocornut_imgui"),
	                               bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "fs_ocornut_imgui"), true);
	_imageProgram = bgfx::createProgram(bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "vs_imgui_image"),
	                                    bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "fs_imgui_image"), true);

	// Create buffers
	_u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);

	_layout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();

	_s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	CreateFontsTextureBgfx();

	return true;
}

void Gui::NewFrame()
{
	ImGui::SetCurrentContext(_imgui);
	ImGui_ImplSDL2_NewFrame();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	ImGui::NewFrame();
}

bool Gui::Loop(Game& game, const Renderer& renderer)
{
	for (auto& window : _debugWindows)
	{
		window->WindowUpdate(game, renderer);
	}
	NewFrame();
	if (ShowMenu(game))
	{
		// Exit option selected
		return true;
	}
	for (auto& window : _debugWindows)
	{
		window->WindowDraw(game);
	}
	ShowVillagerNames(game);
	ShowCameraPositionOverlay(game);

	ImGui::Render();

	return false;
}

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] numVertices Number of vertices.
/// @param[in] layout Vertex layout.
/// @param[in] numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t numVertices, const bgfx::VertexLayout& layout, uint32_t numIndices)
{
	return numVertices == bgfx::getAvailTransientVertexBuffer(numVertices, layout) &&
	       (0 == numIndices || numIndices == bgfx::getAvailTransientIndexBuffer(numIndices));
}

void Gui::RenderDrawDataBgfx(ImDrawData* drawData)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fbWidth = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	int fbHeight = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fbWidth <= 0 || fbHeight <= 0)
	{
		return;
	}

	bgfx::setViewMode(_viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps* caps = bgfx::getCaps();
	{
		glm::mat4 ortho;
		float x = drawData->DisplayPos.x;
		float y = drawData->DisplayPos.y;
		float width = drawData->DisplaySize.x;
		float height = drawData->DisplaySize.y;

		bx::mtxOrtho(glm::value_ptr(ortho), x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(_viewId, nullptr, glm::value_ptr(ortho));
		bgfx::setViewRect(_viewId, 0, 0, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
	}

	const ImVec2 clipPos = drawData->DisplayPos;         // (0,0) unless using multi-viewports
	const ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii)
	{
		bgfx::TransientVertexBuffer tvb;
		bgfx::TransientIndexBuffer tib;

		const ImDrawList* drawList = drawData->CmdLists[ii];
		const auto numVertices = static_cast<uint32_t>(drawList->VtxBuffer.size());
		const auto numIndices = static_cast<uint32_t>(drawList->IdxBuffer.size());

		if (!checkAvailTransientBuffers(numVertices, _layout, numIndices))
		{
			// not enough space in transient buffer just quit drawing the rest...
			break;
		}

		bgfx::allocTransientVertexBuffer(&tvb, numVertices, _layout);
		bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

		auto* verts = reinterpret_cast<ImDrawVert*>(tvb.data);
		bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

		auto* indices = reinterpret_cast<ImDrawIdx*>(tib.data);
		bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

		bgfx::Encoder* encoder = bgfx::begin();

		for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
		{
			if (cmd->UserCallback != nullptr)
			{
				cmd->UserCallback(drawList, cmd);
			}
			else if (0 != cmd->ElemCount)
			{
				uint64_t state = 0                      //
				                 | BGFX_STATE_WRITE_RGB //
				                 | BGFX_STATE_WRITE_A   //
				                 | BGFX_STATE_MSAA      //
				    ;

				bgfx::TextureHandle th = _texture;
				bgfx::ProgramHandle program = _program;

				if (cmd->TextureId != nullptr)
				{
					union
					{
						ImTextureID ptr;
						struct
						{
							bgfx::TextureHandle handle;
							uint8_t flags;
							uint8_t mip;
						} s;
					} texture = {cmd->TextureId};
					state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
					             ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
					             : BGFX_STATE_NONE;
					th = texture.s.handle;
					if (0 != texture.s.mip)
					{
						const glm::vec4 lodEnabled = {static_cast<float>(texture.s.mip), 1.0f, 0.0f, 0.0f};
						bgfx::setUniform(_u_imageLodEnabled, glm::value_ptr(lodEnabled));
						program = _imageProgram;
					}
				}
				else
				{
					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
				}

				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clipRect;
				clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
				clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
				clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
				clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

				if (clipRect.x < fbWidth && clipRect.y < fbHeight && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
				{
					const auto xx = static_cast<uint16_t>(bx::max(clipRect.x, 0.0f));
					const auto yy = static_cast<uint16_t>(bx::max(clipRect.y, 0.0f));
					encoder->setScissor(xx, yy, static_cast<uint16_t>(bx::min(clipRect.z, 65535.0f) - xx),
					                    static_cast<uint16_t>(bx::min(clipRect.w, 65535.0f) - yy));

					encoder->setState(state);
					encoder->setTexture(0, _s_tex, th);
					encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
					encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
					encoder->submit(_viewId, program);
				}
			}
		}

		bgfx::end(encoder);
	}
}

void Gui::Draw()
{
	ImGui::SetCurrentContext(_imgui);

	RenderDrawDataBgfx(ImGui::GetDrawData());
}

bool Gui::ShowMenu(Game& game)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Load Island"))
		{
			auto& resources = Locator::resources::value();
			auto& levelsManager = resources.GetLevels();
			auto campaigns = std::vector<const Level*>();
			auto playgrounds = std::vector<const Level*>();
			levelsManager.Each([&campaigns, &playgrounds](entt::id_type /*id*/, const Level& level) {
				if (level.IsCampaign())
				{
					campaigns.emplace_back(&level);
				}
				else
				{
					playgrounds.emplace_back(&level);
				}
			});

			auto menuItem = [&game](const auto& label, const std::filesystem::path& path) {
				if (ImGui::MenuItem(label.data()))
				{
					game.LoadMap(path);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", path.generic_string().c_str());
				}
			};

			ImGui::MenuItem("Story Islands", nullptr, false, false);
			for (auto& level : campaigns)
			{
				menuItem(level->GetName(), level->GetScriptPath());
			}
			ImGui::Separator();
			ImGui::MenuItem("Playground Islands", nullptr, false, false);
			for (auto& level : playgrounds)
			{
				menuItem(level->GetName(), level->GetScriptPath());
			}

			ImGui::EndMenu();
		}

		auto& config = game.GetConfig();

		if (ImGui::BeginMenu("World"))
		{
			if (ImGui::SliderFloat("Time of Day", &config.timeOfDay, 0.0f, 24.0f, "%.3f"))
			{
				Game::Instance()->SetTime(fmodf(config.timeOfDay, 24.0f));
			}

			ImGui::Text("Sky Type Index %f", game.GetSky().GetCurrentSkyType());
			ImGui::SliderFloat("Sky alignment", &config.skyAlignment, -1.0f, 1.0f, "%.3f");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug"))
		{
			if (ImGui::BeginMenu("Windows"))
			{
				for (auto& window : _debugWindows)
				{
					if (ImGui::MenuItem(window->GetName().c_str()))
					{
						window->Open();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Villager Names"))
			{
				ImGui::Checkbox("Show", &config.showVillagerNames);
				ImGui::Checkbox("Show States", &config.debugVillagerStates);
				ImGui::Checkbox("Debug", &config.debugVillagerNames);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::Checkbox("Sky", &config.drawSky);
				ImGui::Checkbox("Water", &config.drawWater);
				ImGui::Checkbox("Island", &config.drawIsland);
				ImGui::Checkbox("Entities", &config.drawEntities);
				ImGui::Checkbox("Sprites", &config.drawSprites);
				ImGui::Checkbox("TestModel", &config.drawTestModel);
				ImGui::Checkbox("Debug Cross", &config.drawDebugCross);
				ImGui::Checkbox("Wireframe", &config.wireframe);
				ImGui::Checkbox("Bounding Boxes", &config.drawBoundingBoxes);
				ImGui::Checkbox("Footpaths", &config.drawFootpaths);
				ImGui::Checkbox("Streams", &config.drawStreams);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Game Speed"))
			{
				float multiplier = game.GetGameSpeed();
				ImGui::Text("Scaled game duration: %.3fms (%.3f Hz)", multiplier * Game::k_TurnDuration.count(),
				            1000.0f / (multiplier * Game::k_TurnDuration.count()));
				if (ImGui::MenuItem("Slow"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierSlow);
				}
				if (ImGui::MenuItem("Normal"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierNormal);
				}
				if (ImGui::MenuItem("Fast"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierFast);
				}
				if (ImGui::SliderFloat("Multiplier", &multiplier, 1.0f / 10.0f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
				{
					game.SetGameSpeed(multiplier);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Capture"))
		{
			if (ImGui::Button("Capture"))
			{
				game.RequestScreenshot(_screenshotFilename);
			}
			ImGui::SameLine();
			ImGui::InputText("Screenshot", &_screenshotFilename);
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Quit", "Esc"))
		{
			return true;
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		_menuBarSize = ImGui::GetWindowSize();

		ImGui::EndMainMenuBar();
	}
	return false;
}

void Gui::RenderArrow(const std::string& name, const ImVec2& pos, const ImVec2& size) const
{
	// clang-format off
	static const auto boxOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoInputs
	;
	// clang-format on
	ImGui::SetNextWindowPos(ImVec2(pos.x + size.x / 2, pos.y + size.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin((name + " Frame").c_str(), nullptr, boxOverlayFlags))
	{
		std::string strId = name + " Arrow";
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (!window->SkipItems)
		{
			const ImGuiID id = window->GetID(strId.c_str());
			const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
			if (ImGui::ItemAdd(bb, id))
			{
				// Render
				const ImU32 color = ImGui::GetColorU32(ImGuiCol_WindowBg, 0.35f);
				const ImVec2 tipDimensions(size.x / 2.0f, size.x / 3.0f * 2.0f);
				const float shaftCenterX = window->DC.CursorPos.x + size.x / 2.0f;
				const ImVec2 shaftSize(size.x / 5.0f, bb.Max.y - bb.Min.y - tipDimensions.y);
				const ImRect shaft(shaftCenterX - shaftSize.x, bb.Min.y, shaftCenterX + shaftSize.x, bb.Min.y + shaftSize.y);
				ImGui::RenderRectFilledRangeH(window->DrawList, shaft, color, 0.0f, 1.0f, 0.0f);
				ImGui::RenderArrowPointingAt(window->DrawList, ImVec2(bb.Max.x - size.x / 2.0f, bb.Max.y), tipDimensions,
				                             ImGuiDir_Down, color);
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowBorderSize
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowRounding
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
}

bool boxIntersect(const glm::vec4& box1, const glm::vec4& box2)
{
	// where z is width and y is height
	return box1.x - box2.x < box2.z && //
	       box2.x - box1.x < box1.z && //
	       box1.y - box2.y < box2.w && //
	       box2.y - box1.y < box1.w;
}

bool fitBox(float minY, const std::vector<glm::vec4>& coveredAreas, glm::vec4& box)
{
	// Where z is width and y is height
	bool restart = true;
	while (restart)
	{
		restart = false;
		for (const auto area : coveredAreas)
		{
			// If there is an overlap, try moving it up
			while (boxIntersect(area, box))
			{
				restart = true; // Restart because we now might intersect with earlier boxes
				box.y = area.y - 1.0f - box.w;
				if (box.y - box.w < minY)
				{
					return false;
				}
			}
			if (restart)
			{
				break;
			}
		}
	}
	return true;
}

std::optional<glm::uvec4> Gui::RenderVillagerName(const std::vector<glm::vec4>& coveredAreas, const std::string& name,
                                                  const std::string& text, const glm::vec4& color, const ImVec2& pos,
                                                  float arrowLength, std::function<void(void)> debugCallback) const
{
	// clang-format off
	const auto boxOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| (debugCallback ? ImGuiWindowFlags_AlwaysVerticalScrollbar : ImGuiWindowFlags_NoScrollbar)
		| ImGuiWindowFlags_NoCollapse
		| (debugCallback ? 0 : ImGuiWindowFlags_NoInputs)
	;
	// clang-format on

	const auto& style = ImGui::GetStyle();

	ImVec4 textColor;
	{
		glm::vec4 adjustedColor = color;
		if (adjustedColor.r < 0.04f && adjustedColor.g < 0.04f && adjustedColor.b < 0.04f)
		{
			adjustedColor = glm::vec4(1.0f, 1.0f, 1.0f, adjustedColor.a);
		}
		adjustedColor = glm::vec4((glm::vec3(adjustedColor) + glm::vec3(1.0f)) / 2.0f, 1.0f);
		textColor = ImVec4(adjustedColor.r, adjustedColor.g, adjustedColor.b, adjustedColor.a);
	}

	const std::string fullText = name + "\n" + text;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	glm::vec4 boxExtent;
	{
		const auto textSize = ImGui::CalcTextSize(fullText.c_str());
		ImVec2 boxSize(textSize.x + 2 * style.WindowPadding.x, textSize.y + 2 * style.WindowPadding.y);
		if (debugCallback)
		{
			boxSize.x += style.ScrollbarSize;
			boxSize.y += ImGui::CalcTextSize("Debug").y + 2 * style.FramePadding.y + style.ItemSpacing.y;
		}
		const float halfWidth = boxSize.x / 2.0f;
		const auto clampedX = std::clamp(pos.x, halfWidth, ImGui::GetIO().DisplaySize.x - halfWidth);
		const ImVec2 boxPos(clampedX + halfWidth, std::max(pos.y - arrowLength, .0f));
		boxExtent = glm::uvec4(boxPos.x, boxPos.y, boxSize.x, boxSize.y);
	}

	float originalY = boxExtent.y;
	bool fits = fitBox(_menuBarSize.y, coveredAreas, boxExtent);

	if (fits)
	{
		ImGui::SetNextWindowPos(ImVec2(boxExtent.x, boxExtent.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		if (!debugCallback)
		{
			ImGui::SetNextWindowSize(ImVec2(boxExtent.z, boxExtent.w));
		}
		ImGui::SetNextWindowBgAlpha(0.35f);

		if (ImGui::Begin(("Villager overlay #" + name).c_str(), nullptr, boxOverlayFlags))
		{
			ImGui::TextColored(textColor, "%s", fullText.c_str());
			if (debugCallback)
			{
				if (ImGui::CollapsingHeader("Debug"))
				{
					debugCallback();
				}
			}
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();

	if (!fits)
	{
		return std::nullopt;
	}

	if (originalY == boxExtent.y)
	{
		RenderArrow("Villager overlay arrow #" + name, ImVec2(pos.x, pos.y - arrowLength), ImVec2(15, arrowLength));
	}

	return std::make_optional<glm::uvec4>(boxExtent);
}

void Gui::ShowVillagerNames(const Game& game)
{
	using namespace ecs::components;
	using namespace ecs::systems;

	const auto& config = game.GetConfig();
	if (!config.showVillagerNames)
	{
		return;
	}

	const auto& displaySize = ImGui::GetIO().DisplaySize;

	uint32_t i = 0;
	const auto& camera = game.GetCamera();
	const glm::vec4 viewport =
	    glm::vec4(ImGui::GetStyle().WindowPadding.x, 0, displaySize.x - ImGui::GetStyle().WindowPadding.x, displaySize.y);
	std::vector<glm::vec4> coveredAreas;
	coveredAreas.reserve(game.GetEntityRegistry().Size<Villager>());
	game.GetEntityRegistry().Each<const Transform, Villager, LivingAction>([this, &i, &coveredAreas, &camera, config, viewport](
	                                                                           const Transform& transform, Villager& villager,
	                                                                           LivingAction& action) {
		++i;
		const float height = 2.0f * transform.scale.y; // TODO(bwrsandman): get from bounding box max y
		glm::vec3 screenPoint;
		if (!camera.ProjectWorldToScreen(transform.position + glm::vec3(0.0f, height, 0.0f), viewport, screenPoint))
		{
			return;
		}

		// 3.5 was measured in vanilla but it is possible that it is configurable
		float maxDistance = 3.5f;
		const glm::vec3 relativePosition = (camera.GetPosition() - transform.position) / 100.0f;
		if (glm::dot(relativePosition, relativePosition) > maxDistance * maxDistance)
		{
			return;
		}

		// TODO(bwrsandman): Get owner player and associated color
		glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		// Female villagers have a lighter colour
		if (villager.sex == Villager::Sex::FEMALE)
		{
			color += glm::vec4((glm::vec3(1.0f) - glm::vec3(color)) * glm::vec3(144.0f / 255.0f), color.a);
			color = glm::saturate(color);
		}

		const std::string name = "Villager #" + std::to_string(i);
		const std::string stateHelpText = "TODO: STATE HELP TEXT";
		std::string details =
		    fmt::format("{}\nA:{} L:{}%, H:{}%", stateHelpText, villager.age, villager.health, villager.hunger);
		const auto& actionSystem = Locator::livingActionSystem::value();
		if (config.debugVillagerStates)
		{
			details += fmt::format(
			    "\n"
			    "Top State:      {}\n"
			    "Final State:    {}\n"
			    "Previous State: {}\n"
			    "Turns until next state change: {}\n"
			    "Turns since last state change: {}",
			    k_VillagerStateStrings.at(static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Top))),
			    k_VillagerStateStrings.at(
			        static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Final))),
			    k_VillagerStateStrings.at(
			        static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Previous))),
			    action.turnsUntilStateChange, action.turnsSinceStateChange);
		}

		std::function<void(void)> debugCallback;
		if (config.debugVillagerNames)
		{
			debugCallback = [&villager, &action, &actionSystem] {
				if (villager.abode == entt::null)
				{
					ImGui::Text("Homeless");
				}
				ImGui::InputInt("Health", reinterpret_cast<int*>(&villager.health));
				ImGui::InputInt("Age", reinterpret_cast<int*>(&villager.age));
				ImGui::InputInt("Hunger", reinterpret_cast<int*>(&villager.hunger));
				ImGui::Combo("Life Stage", &villager.lifeStage, Villager::k_LifeStageStrs);
				ImGui::Combo("Sex", &villager.sex, Villager::k_SexStrs);
				ImGui::Combo("Tribe", &villager.tribe, k_TribeStrs);
				ImGui::Combo("Villager Number", &villager.number, k_VillagerRoleStrs);
				ImGui::Combo("Task", &villager.task, Villager::k_TaskStrs);

				size_t index = 0;
				for (const auto& str : LivingAction::k_IndexStrings)
				{
					if (ImGui::BeginCombo(str.data(), k_VillagerStateStrings
					                                      .at(static_cast<size_t>(actionSystem.VillagerGetState(
					                                          action, static_cast<LivingAction::Index>(index))))
					                                      .data()))
					{
						size_t n = 0;
						for (const auto& stateStr : k_VillagerStateStrings)
						{
							const bool isSelected = static_cast<size_t>(actionSystem.VillagerGetState(
							                            action, static_cast<LivingAction::Index>(index))) == n;
							if (ImGui::Selectable(stateStr.data(), isSelected))
							{
								actionSystem.VillagerSetState(action, static_cast<LivingAction::Index>(index),
								                              static_cast<VillagerStates>(n), true);
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
							++n;
						}
						ImGui::EndCombo();
					}
					++index;
				}
			};
		}

		const auto area = RenderVillagerName(coveredAreas, name, details, color,
		                                     ImVec2(screenPoint.x, viewport.w - screenPoint.y), 100.0f, debugCallback);
		if (area.has_value())
		{
			coveredAreas.emplace_back(area.value());
		}
	});
}

void Gui::ShowCameraPositionOverlay(const Game& game)
{
	// clang-format off
	static const auto cameraPositionOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		;
	// clang-format on

	const auto& displaySize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(displaySize.x - 8.0f, displaySize.y - 8.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	if (ImGui::Begin("Camera position overlay", nullptr, cameraPositionOverlayFlags))
	{
		const auto camPos = game.GetCamera().GetPosition();
		const auto camRot = glm::degrees(game.GetCamera().GetRotation());
		ImGui::Text("Camera Position: (%.1f,%.1f, %.1f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rotation: (%.1f,%.1f, %.1f)", camRot.x, camRot.y, camRot.z);

		if (ImGui::IsMousePosValid())
		{
			const auto& mousePos = ImGui::GetIO().MousePos;
			ImGui::Text("Mouse Position: (%.1f,%.1f)", mousePos.x, mousePos.y);
		}
		else
		{
			ImGui::Text("Mouse Position: <invalid>");
		}

		const auto& handPosition = game.GetEntityRegistry().Get<ecs::components::Transform>(game.GetHand()).position;
		ImGui::Text("Hand Position: (%.1f,%.1f,%.1f)", handPosition.x, handPosition.y, handPosition.z);

		ImGui::Text("Game Turn: %u (%.3f ms)%s", game.GetTurn(), game.GetDeltaTime().count(),
		            game.IsPaused() ? " - paused" : "");
	}
	ImGui::End();
}
