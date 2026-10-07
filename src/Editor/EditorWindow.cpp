/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EditorWindow.h"

#include <algorithm>
#include <array>
#include <limits>

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <fmt/format.h>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>

#include "Camera/Camera.h"
#include "Camera/KeyboardMoveSpeed.h"
#include "Debug/CreatureSpawner.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/EditorSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "EditorMath.h"
#include "EditorStyle.h"
#include "Game.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

namespace openblack::editor
{

using Tool = ecs::systems::EditorSystemInterface::Tool;
using CameraMode = ecs::systems::EditorSystemInterface::CameraMode;

namespace
{
/// Radians the camera turns for a pixel dragged
constexpr float k_CameraTurnPerPixel = 0.006f;
/// Radians a thing turns for a pixel dragged across
constexpr float k_RotatePerPixel = 0.01f;

constexpr ImGuiWindowFlags k_PanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

bool IsMouseButton(const SDL_Event& event, uint8_t button)
{
	return (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == button;
}

/// The rotation an archetype gives a thing it places facing an angle: creatures turn one way, the rest the other
glm::mat3 PlacedRotation(PlaceKind kind, float yawRadians)
{
	return YawRotation(kind == PlaceKind::Creature ? yawRadians : -yawRadians);
}

ecs::systems::EditorSystemInterface* System()
{
	return Locator::editorSystem::has_value() ? &Locator::editorSystem::value() : nullptr;
}
} // namespace

EditorWindow::EditorWindow(debug::gui::CreatureSpawner& spawner, debug::gui::Window& scenarios) noexcept
    : Window("Editor", ImVec2(800.0f, 600.0f))
    , _spawner(spawner)
    , _scenarios(scenarios)
{
}

EditorWindow::~EditorWindow() noexcept = default;

void EditorWindow::Open() noexcept
{
	Window::Open();
	if (auto* system = System())
	{
		system->SetOpen(true);
	}
	_outliner.Invalidate();
}

void EditorWindow::Close() noexcept
{
	Window::Close();
	_placement.item.reset();
	_drag = Drag::None;
	_pickAt.reset();
	_placeAt.reset();
	if (auto* system = System())
	{
		system->SetOpen(false);
	}
}

std::optional<EditorContext> EditorWindow::Context() noexcept
{
	auto* system = System();
	if (system == nullptr || !Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	if (!_names.has_value())
	{
		_names = NameTables::Load();
		if (!_names.has_value())
		{
			return std::nullopt;
		}
	}
	return EditorContext {
	    .system = *system,
	    .registry = Locator::entitiesRegistry::value(),
	    .names = *_names,
	    .spawner = _spawner,
	    .placement = _placement,
	};
}

void EditorWindow::UpdateAlways() noexcept
{
	_log.Attach();
	// The creature spawner's window now opens the editor on its creature, as the scenarios' buttons ask it to
	if (_spawner.IsOpen())
	{
		const auto picked = _spawner.GetSelected();
		_spawner.Close();
		Open();
		if (auto* system = System(); system != nullptr && picked.has_value())
		{
			system->GetSelection().Select(*picked);
		}
	}
	// The scenarios' window, which opens with the testbed, is the editor's tab while the editor is open
	if (IsOpen() && _scenarios.IsOpen())
	{
		_scenarios.Close();
		_showTab = BottomTab::Scenarios;
		_showBottom = true;
	}
	if (auto* system = System(); system != nullptr && system->IsOpen() != IsOpen())
	{
		system->SetOpen(IsOpen());
	}
}

glm::vec2 EditorWindow::MouseOnScreen() noexcept
{
	if (!Locator::windowing::has_value())
	{
		return {0.5f, 0.5f};
	}
	glm::ivec2 mouse {};
	SDL_GetMouseState(&mouse.x, &mouse.y);
	return glm::vec2(mouse) / glm::max(glm::vec2(Locator::windowing::value().GetSize()), glm::vec2(1.0f));
}

std::optional<glm::vec3> EditorWindow::LandAt(glm::vec2 screen) noexcept
{
	if (!Locator::camera::has_value() || !Locator::dynamicsSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& camera = Locator::camera::value();
	if (const auto hit = camera.RaycastScreenCoordToLand(screen, false))
	{
		return hit->position;
	}
	// Off the land, the sea's level
	glm::vec3 origin;
	glm::vec3 direction;
	camera.DeprojectScreenToWorld(screen, origin, direction);
	return RayLevel(origin, direction, 0.0f);
}

std::optional<entt::entity> EditorWindow::Pick(glm::vec2 screen) const noexcept
{
	if (!Locator::camera::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	glm::vec3 origin;
	glm::vec3 direction;
	Locator::camera::value().DeprojectScreenToWorld(screen, origin, direction);
	const auto& registry = Locator::entitiesRegistry::value();
	std::optional<entt::entity> nearest;
	auto best = std::numeric_limits<float>::max();
	float bestVolume = std::numeric_limits<float>::max();
	registry.Each<const ecs::components::Transform, const ecs::components::Mesh>(
	    [&](entt::entity entity, const ecs::components::Transform&, const ecs::components::Mesh&) {
		    if (registry.AllOf<ecs::components::Hand>(entity))
		    {
			    return;
		    }
		    const auto bounds = WorldBoundsOf(registry, entity);
		    if (!bounds.has_value() || bounds->Contains(origin))
		    {
			    return;
		    }
		    const auto along = RayBox(origin, direction, *bounds);
		    if (!along.has_value())
		    {
			    return;
		    }
		    // Of things the ray meets about as soon, the smaller is the one meant, as a villager by a house
		    const auto size = bounds->Size();
		    const auto volume = size.x * size.y * size.z;
		    constexpr float k_Tie = 2.0f;
		    if (*along < best - k_Tie || (*along < best + k_Tie && volume < bestVolume))
		    {
			    best = std::min(best, *along);
			    bestVolume = volume;
			    nearest = entity;
		    }
	    });
	return nearest;
}

void EditorWindow::PlaceAt(EditorContext& context, glm::vec2 screen) noexcept
{
	if (!_placement.item.has_value())
	{
		return;
	}
	const auto hit = LandAt(screen);
	if (!hit.has_value())
	{
		_placement.last = "Nowhere to place it there";
		return;
	}
	auto position = SnapPoint(*hit, context.system.GetSnapping());
	position.y = LandHeight({position.x, position.z});
	const auto& item = *_placement.item;
	const auto entity = item.kind == PlaceKind::Creature ? _spawner.SpawnAt(position, _placement.yawRadians)
	                                                     : Place(item, position, _placement.yawRadians);
	if (entity == entt::null)
	{
		_placement.last = fmt::format("Couldn't place {}", context.names.NameOf(item.kind, item.type));
		return;
	}
	_placement.last =
	    fmt::format("Placed {} at {:.0f}, {:.0f}", context.names.NameOf(item.kind, item.type), position.x, position.z);
	context.system.GetSelection().Select(entity);
	_outliner.Invalidate();
}

void EditorWindow::Update() noexcept
{
	_spawner.HostedUpdate();
	auto context = Context();
	if (!context.has_value())
	{
		return;
	}
	auto& system = context->system;
	auto& selection = system.GetSelection();

	if (_placeAt.has_value())
	{
		PlaceAt(*context, *_placeAt);
		_placeAt.reset();
	}
	if (_pickAt.has_value())
	{
		const auto picked = Pick(*_pickAt);
		const auto tool = system.GetTool();
		if (picked.has_value())
		{
			selection.Select(*picked);
		}
		else if (tool == Tool::Select)
		{
			selection.Clear();
		}
		// Moving and turning start on what was clicked, or on what was already picked
		const auto target = selection.Get();
		if (target.has_value() && (tool == Tool::Move || tool == Tool::Rotate))
		{
			const auto& transform = context->registry.Get<ecs::components::Transform>(*target);
			if (tool == Tool::Move)
			{
				const auto land = LandAt(*_pickAt);
				_moveOffset = land.has_value() ? transform.position - *land : glm::vec3(0.0f);
				_moveOffset.y = 0.0f;
				_drag = Drag::Move;
			}
			else
			{
				_rotateFrom = YawOf(transform.rotation);
				_rotated = 0.0f;
				_dragAcross = 0.0f;
				_drag = Drag::Rotate;
			}
		}
		_pickAt.reset();
	}

	const auto selected = selection.Get();
	if (!selected.has_value() || !context->registry.AllOf<ecs::components::Transform>(*selected))
	{
		if (_drag == Drag::Move || _drag == Drag::Rotate)
		{
			_drag = Drag::None;
		}
		return;
	}
	if (_drag == Drag::Move)
	{
		if (const auto land = LandAt(MouseOnScreen()))
		{
			auto position = SnapPoint(*land + _moveOffset, system.GetSnapping());
			position.y = LandHeight({position.x, position.z});
			MoveTo(*selected, position);
		}
	}
	else if (_drag == Drag::Rotate)
	{
		const auto wanted = SnapAngle(_rotateFrom + (_dragAcross * k_RotatePerPixel), system.GetSnapping()) - _rotateFrom;
		if (wanted != _rotated)
		{
			Turn(*selected, wanted - _rotated);
			_rotated = wanted;
		}
	}
}

bool EditorWindow::IsEditorKey(const SDL_Event& event) const noexcept
{
	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
	{
		return false;
	}
	const auto* system = Locator::editorSystem::has_value() ? &Locator::editorSystem::value() : nullptr;
	const bool picked = system != nullptr && !system->GetSelection().Empty();
	const bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
	switch (event.key.keysym.sym)
	{
	case SDLK_DELETE:
	case SDLK_g:
	case SDLK_o:
		return picked;
	case SDLK_d:
		return picked && ctrl;
	case SDLK_w:
	case SDLK_e:
	case SDLK_r:
	case SDLK_h:
	case SDLK_PERIOD:
		return true;
	case SDLK_LEFTBRACKET:
	case SDLK_RIGHTBRACKET:
		return _placement.item.has_value();
	case SDLK_EQUALS:
	case SDLK_KP_PLUS:
	case SDLK_MINUS:
	case SDLK_KP_MINUS:
	case SDLK_0:
	case SDLK_KP_0:
		return ctrl;
	case SDLK_ESCAPE:
		// Escape is only the editor's while there is something of it to let go of; otherwise the game has it
		return _placement.item.has_value() || _drag != Drag::None || picked ||
		       (system != nullptr && system->GetCameraMode() != CameraMode::Free);
	default:
		return false;
	}
}

void EditorWindow::HandleKey(const SDL_Event& event) noexcept
{
	if (event.type != SDL_KEYDOWN || !IsEditorKey(event))
	{
		return;
	}
	auto context = Context();
	if (!context.has_value())
	{
		return;
	}
	auto& system = context->system;
	auto& selection = system.GetSelection();
	const auto turn = glm::radians(k_PlaceTurnDegrees);
	switch (event.key.keysym.sym)
	{
	case SDLK_DELETE:
		if (const auto entity = selection.Get())
		{
			selection.Clear();
			Remove(*entity);
			_outliner.Invalidate();
		}
		break;
	case SDLK_d:
		if (const auto entity = selection.Get())
		{
			if (const auto copy = Duplicate(*entity, {8.0f, 0.0f}))
			{
				selection.Select(*copy);
				_outliner.Invalidate();
			}
		}
		break;
	case SDLK_g:
		system.FrameSelection();
		break;
	case SDLK_o:
	{
		const auto wanted = (event.key.keysym.mod & KMOD_SHIFT) != 0 ? CameraMode::Follow : CameraMode::Orbit;
		system.SetCameraMode(system.GetCameraMode() == wanted ? CameraMode::Free : wanted);
		break;
	}
	case SDLK_w:
		system.SetTool(Tool::Select);
		break;
	case SDLK_e:
		system.SetTool(Tool::Move);
		break;
	case SDLK_r:
		system.SetTool(Tool::Rotate);
		break;
	case SDLK_h:
		system.SetTool(Tool::GameHand);
		break;
	case SDLK_PERIOD:
		system.StepTurn();
		break;
	case SDLK_LEFTBRACKET:
		_placement.yawRadians = WrapAngle(_placement.yawRadians - turn);
		break;
	case SDLK_RIGHTBRACKET:
		_placement.yawRadians = WrapAngle(_placement.yawRadians + turn);
		break;
	case SDLK_EQUALS:
	case SDLK_KP_PLUS:
		system.SetCameraMoveSpeed(StepKeyboardMoveSpeed(system.GetCameraMoveSpeed(), 1));
		break;
	case SDLK_MINUS:
	case SDLK_KP_MINUS:
		system.SetCameraMoveSpeed(StepKeyboardMoveSpeed(system.GetCameraMoveSpeed(), -1));
		break;
	case SDLK_0:
	case SDLK_KP_0:
		system.SetCameraMoveSpeed(k_KeyboardMoveSpeedDefault);
		break;
	case SDLK_ESCAPE:
		if (_placement.item.has_value())
		{
			_placement.item.reset();
		}
		else if (_drag != Drag::None)
		{
			_drag = Drag::None;
		}
		else if (system.GetCameraMode() != CameraMode::Free)
		{
			system.SetCameraMode(CameraMode::Free);
		}
		else
		{
			selection.Clear();
		}
		break;
	default:
		break;
	}
}

bool EditorWindow::TakesEvent(const SDL_Event& event) const noexcept
{
	if (_spawner.HostedTakesEvent(event))
	{
		return true;
	}
	const auto& io = ImGui::GetIO();
	const auto* system = Locator::editorSystem::has_value() ? &Locator::editorSystem::value() : nullptr;
	if (system == nullptr)
	{
		return false;
	}
	const bool placing = _placement.item.has_value();
	const bool cameraOnThing = system->GetCameraMode() != CameraMode::Free;
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (event.type == SDL_MOUSEBUTTONUP && _drag != Drag::None)
		{
			return true;
		}
		if (io.WantCaptureMouse)
		{
			return false;
		}
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			return placing || system->GetTool() != Tool::GameHand;
		}
		if (event.button.button == SDL_BUTTON_RIGHT)
		{
			return placing || cameraOnThing;
		}
		return false;
	case SDL_MOUSEWHEEL:
		return !io.WantCaptureMouse && (placing || cameraOnThing);
	case SDL_MOUSEMOTION:
		return _drag != Drag::None;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		return !io.WantCaptureKeyboard && !io.WantTextInput && IsEditorKey(event);
	default:
		return false;
	}
}

void EditorWindow::ProcessEventAlways(const SDL_Event& event) noexcept
{
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2 && event.key.repeat == 0 && !ImGui::GetIO().WantTextInput)
	{
		Toggle();
	}
}

void EditorWindow::ProcessEventOpen(const SDL_Event& event) noexcept
{
	_spawner.HostedProcessEvent(event);
	auto* system = System();
	if (system == nullptr)
	{
		return;
	}
	// A drag ends wherever the button comes up
	if (event.type == SDL_MOUSEBUTTONUP && _drag != Drag::None)
	{
		_drag = Drag::None;
		return;
	}
	if (!TakesEvent(event) || !Locator::windowing::has_value())
	{
		return;
	}
	const auto size = glm::max(glm::vec2(Locator::windowing::value().GetSize()), glm::vec2(1.0f));
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	{
		const auto screen = glm::vec2(static_cast<float>(event.button.x), static_cast<float>(event.button.y)) / size;
		if (IsMouseButton(event, SDL_BUTTON_LEFT))
		{
			if (_placement.item.has_value())
			{
				_placeAt = screen;
			}
			else
			{
				_pickAt = screen;
			}
		}
		else if (IsMouseButton(event, SDL_BUTTON_RIGHT))
		{
			if (_placement.item.has_value())
			{
				_placement.item.reset();
			}
			else
			{
				_drag = Drag::Camera;
				_dragLast = {event.button.x, event.button.y};
			}
		}
		break;
	}
	case SDL_MOUSEMOTION:
	{
		const glm::ivec2 at {event.motion.x, event.motion.y};
		const auto delta = glm::vec2(at - _dragLast);
		_dragLast = at;
		if (_drag == Drag::Camera)
		{
			system->TurnCamera({-delta.x * k_CameraTurnPerPixel, delta.y * k_CameraTurnPerPixel});
		}
		else if (_drag == Drag::Rotate)
		{
			_dragAcross += delta.x;
		}
		break;
	}
	case SDL_MOUSEWHEEL:
		if (_placement.item.has_value())
		{
			const auto turn = glm::radians(k_PlaceTurnDegrees) * static_cast<float>(event.wheel.y > 0 ? 1 : -1);
			_placement.yawRadians = WrapAngle(_placement.yawRadians + turn);
		}
		else
		{
			system->ZoomCamera(static_cast<float>(event.wheel.y));
		}
		break;
	case SDL_KEYDOWN:
		HandleKey(event);
		break;
	default:
		break;
	}
	// The rotate drag counts from where the button went down
	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		_dragLast = {event.button.x, event.button.y};
	}
}

void EditorWindow::WindowDraw() noexcept
{
	if (!IsOpen())
	{
		return;
	}
	auto context = Context();
	if (!context.has_value())
	{
		return;
	}
	const auto* viewport = ImGui::GetMainViewport();
	const auto origin = viewport->WorkPos;
	const auto size = viewport->WorkSize;
	const auto& imguiStyle = ImGui::GetStyle();
	const auto oneRowHeight = ImGui::GetFrameHeight() + (imguiStyle.WindowPadding.y * 2.0f);
	const auto toolbarHeight = std::max(_toolbarHeight, oneRowHeight);

	ImGui::SetNextWindowPos(origin);
	// The toolbar is as tall as its rows need
	ImGui::SetNextWindowSize(ImVec2(size.x, 0.0f));
	if (ImGui::Begin("Editor##Toolbar", nullptr,
	                 k_PanelFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
	{
		DrawToolbar(*context);
		_toolbarHeight = ImGui::GetWindowHeight();
	}
	ImGui::End();

	_bottomHeight = std::clamp(_bottomHeight, 120.0f, std::max(120.0f, size.y - toolbarHeight - 120.0f));
	const auto bottom = _showBottom ? _bottomHeight : 0.0f;
	const auto sideHeight = std::max(100.0f, size.y - toolbarHeight - bottom);
	const auto top = origin.y + toolbarHeight;

	if (_showOutliner)
	{
		_leftWidth = std::clamp(_leftWidth, 160.0f, size.x * 0.4f);
		ImGui::SetNextWindowPos(ImVec2(origin.x, top));
		ImGui::SetNextWindowSize(ImVec2(_leftWidth, sideHeight));
		if (ImGui::Begin("Outliner##Editor", &_showOutliner, k_PanelFlags))
		{
			_leftWidth = ImGui::GetWindowWidth();
			_outliner.Draw(*context);
		}
		ImGui::End();
	}
	if (_showInspector)
	{
		_rightWidth = std::clamp(_rightWidth, 220.0f, size.x * 0.5f);
		ImGui::SetNextWindowPos(ImVec2(origin.x + size.x - _rightWidth, top));
		ImGui::SetNextWindowSize(ImVec2(_rightWidth, sideHeight));
		if (ImGui::Begin("Inspector##Editor", &_showInspector, k_PanelFlags))
		{
			_rightWidth = ImGui::GetWindowWidth();
			_inspector.Draw(*context);
		}
		ImGui::End();
	}
	if (_showBottom)
	{
		ImGui::SetNextWindowPos(ImVec2(origin.x, origin.y + size.y - _bottomHeight));
		ImGui::SetNextWindowSize(ImVec2(size.x, _bottomHeight));
		if (ImGui::Begin("##EditorBottom", &_showBottom, k_PanelFlags | ImGuiWindowFlags_NoTitleBar))
		{
			_bottomHeight = ImGui::GetWindowHeight();
			DrawBottom(*context);
		}
		ImGui::End();
	}
	DrawOverlays(*context);
	// The creature tools' file browser, when it stands in for the platform's file dialog
	_spawner.DrawDialogs();
}

void EditorWindow::DrawToolbar(EditorContext& context) noexcept
{
	auto& system = context.system;
	const auto separator = [] {
		ImGui::SameLine();
		ImGui::TextColored(style::k_Muted, "|");
		ImGui::SameLine();
	};
	// Starts the next group on a new row when it would not fit on this one
	const auto separatorOrWrap = [](float groupWidth) {
		ImGui::SameLine();
		const auto separatorWidth = ImGui::CalcTextSize("|").x + (ImGui::GetStyle().ItemSpacing.x * 2.0f);
		if (separatorWidth + groupWidth > ImGui::GetContentRegionAvail().x)
		{
			ImGui::NewLine();
			return;
		}
		ImGui::TextColored(style::k_Muted, "|");
		ImGui::SameLine();
	};

	if (Locator::time::has_value())
	{
		auto& time = Locator::time::value();
		const bool paused = time.IsPaused();
		if (ImGui::Button(paused ? "Play" : "Pause", ImVec2(ImGui::GetFontSize() * 4.0f, 0.0f)))
		{
			time.SetPaused(!paused);
		}
		ImGui::SetItemTooltip("Plays or pauses the game (P)");
		ImGui::SameLine();
		ImGui::BeginDisabled(!paused || system.IsStepping());
		if (ImGui::Button("Step"))
		{
			system.StepTurn();
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Plays one game turn, a tenth of a second (.)");
		ImGui::SameLine();
		ImGui::TextColored(style::k_Muted, "turn %u", time.GetTurn());
	}
	if (auto* game = Game::Instance())
	{
		ImGui::SameLine();
		auto speed = game->GetGameSpeed();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0f);
		if (ImGui::SliderFloat("##Speed", &speed, 0.1f, 10.0f, "speed %.2f", ImGuiSliderFlags_Logarithmic))
		{
			game->SetGameSpeed(speed);
		}
		ImGui::SetItemTooltip("The game's speed: how long a game turn takes, as a multiple of its tenth of a second");
	}
	separator();

	constexpr std::array<std::pair<const char*, Tool>, 4> k_Tools {{
	    {"Select (W)", Tool::Select},
	    {"Move (E)", Tool::Move},
	    {"Rotate (R)", Tool::Rotate},
	    {"Hand (H)", Tool::GameHand},
	}};
	for (const auto& [label, tool] : k_Tools)
	{
		if (ImGui::RadioButton(label, system.GetTool() == tool))
		{
			system.SetTool(tool);
		}
		ImGui::SameLine();
	}
	auto& snapping = system.GetSnapping();
	ImGui::Checkbox("Snap", &snapping.enabled);
	if (snapping.enabled)
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
		ImGui::DragFloat("##SnapMove", &snapping.move, 0.5f, 0.5f, 100.0f, "%.1f u");
		ImGui::SetItemTooltip("Moves snap to this many units of the land");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
		ImGui::DragFloat("##SnapAngle", &snapping.angleDegrees, 1.0f, 1.0f, 90.0f, "%.0f deg");
		ImGui::SetItemTooltip("Turns snap to this many degrees");
	}
	separatorOrWrap(_cameraGroupWidth);

	ImGui::BeginGroup();
	constexpr std::array<std::pair<const char*, CameraMode>, 3> k_Cameras {{
	    {"Free", CameraMode::Free},
	    {"Orbit (O)", CameraMode::Orbit},
	    {"Follow (Shift+O)", CameraMode::Follow},
	}};
	const bool picked = !system.GetSelection().Empty();
	for (const auto& [label, mode] : k_Cameras)
	{
		ImGui::BeginDisabled(mode != CameraMode::Free && !picked);
		if (ImGui::RadioButton(label, system.GetCameraMode() == mode))
		{
			system.SetCameraMode(mode);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
	}
	ImGui::BeginDisabled(!picked);
	if (ImGui::Button("View (G)"))
	{
		system.FrameSelection();
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Orbit and follow: drag with the right button to turn, the wheel to draw in and out");
	ImGui::SameLine();
	auto moveSpeed = system.GetCameraMoveSpeed();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.0f);
	if (ImGui::SliderFloat("##CameraMoveSpeed", &moveSpeed, k_KeyboardMoveSpeedMin, k_KeyboardMoveSpeedMax, "move %.2fx",
	                       ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
	{
		system.SetCameraMoveSpeed(moveSpeed);
	}
	ImGui::SetItemTooltip("How fast the movement keys move the camera over the land while the editor is open "
	                      "(Ctrl+Plus and Ctrl+Minus to step, Ctrl+0 to reset)");
	ImGui::SameLine();
	ImGui::BeginDisabled(system.GetCameraMoveSpeed() == k_KeyboardMoveSpeedDefault);
	if (ImGui::Button("1x##CameraMoveSpeedReset"))
	{
		system.SetCameraMoveSpeed(k_KeyboardMoveSpeedDefault);
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Back to the game's own camera speed (Ctrl+0)");
	ImGui::EndGroup();
	_cameraGroupWidth = ImGui::GetItemRectSize().x;

	const auto closeX = ImGui::GetWindowWidth() - (ImGui::GetFontSize() * 7.0f);
	separatorOrWrap(_panelGroupWidth);
	ImGui::BeginGroup();
	ImGui::Checkbox("Outliner", &_showOutliner);
	ImGui::SameLine();
	ImGui::Checkbox("Inspector", &_showInspector);
	ImGui::SameLine();
	ImGui::Checkbox("Tabs", &_showBottom);
	ImGui::EndGroup();
	// The close button keeps to the right edge, and is measured without the gap before it
	_panelGroupWidth = ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize("Close (F2)").x +
	                   (ImGui::GetStyle().FramePadding.x * 2.0f);
	ImGui::SameLine();
	ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), closeX));
	if (ImGui::Button("Close (F2)"))
	{
		Close();
	}
}

void EditorWindow::DrawBottom(EditorContext& context) noexcept
{
	if (!ImGui::BeginTabBar("EditorTabs"))
	{
		return;
	}
	const auto show = _showTab;
	_showTab.reset();
	const auto flags = [show](BottomTab tab) { return show == tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None; };
	if (ImGui::BeginTabItem("Palette", nullptr, flags(BottomTab::Palette)))
	{
		_palette.Draw(context);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Scenarios", nullptr, flags(BottomTab::Scenarios)))
	{
		ImGui::BeginChild("Scenarios");
		_scenarios.DrawContents();
		ImGui::EndChild();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Scripts", nullptr, flags(BottomTab::Scripts)))
	{
		_scripts.Draw();
		ImGui::EndTabItem();
	}
	const auto errors = _log.Errors();
	const auto logLabel = errors > 0 ? fmt::format("Log ({} errors)###Log", errors) : std::string("Log###Log");
	if (ImGui::BeginTabItem(logLabel.c_str(), nullptr, flags(BottomTab::Log)))
	{
		_log.Draw();
		ImGui::EndTabItem();
	}
	// Taller for reading code, back down for the palette
	if (ImGui::TabItemButton(_bottomHeight < ImGui::GetMainViewport()->WorkSize.y * 0.5f ? "Taller" : "Shorter",
	                         ImGuiTabItemFlags_Trailing))
	{
		const auto height = ImGui::GetMainViewport()->WorkSize.y;
		_bottomHeight = _bottomHeight < height * 0.5f ? height * 0.65f : height * 0.3f;
	}
	ImGui::EndTabBar();
}

void EditorWindow::DrawBox(const AxisAlignedBoundingBox& box, uint32_t colour, float thickness) noexcept
{
	if (!Locator::camera::has_value())
	{
		return;
	}
	const auto& camera = Locator::camera::value();
	const auto display = ImGui::GetIO().DisplaySize;
	const glm::vec4 viewport {0.0f, 0.0f, display.x, display.y};
	std::array<std::optional<ImVec2>, 8> corners {};
	for (size_t i = 0; i < corners.size(); ++i)
	{
		const glm::vec3 corner {
		    (i & 1) != 0 ? box.maxima.x : box.minima.x,
		    (i & 2) != 0 ? box.maxima.y : box.minima.y,
		    (i & 4) != 0 ? box.maxima.z : box.minima.z,
		};
		glm::vec3 screen;
		if (camera.ProjectWorldToScreen(corner, viewport, screen))
		{
			corners.at(i) = ImVec2(screen.x, screen.y);
		}
	}
	constexpr std::array<std::pair<size_t, size_t>, 12> k_Edges {{
	    {0, 1},
	    {2, 3},
	    {4, 5},
	    {6, 7},
	    {0, 2},
	    {1, 3},
	    {4, 6},
	    {5, 7},
	    {0, 4},
	    {1, 5},
	    {2, 6},
	    {3, 7},
	}};
	auto* drawList = ImGui::GetBackgroundDrawList();
	for (const auto& [from, to] : k_Edges)
	{
		if (corners.at(from).has_value() && corners.at(to).has_value())
		{
			drawList->AddLine(*corners.at(from), *corners.at(to), colour, thickness);
		}
	}
}

void EditorWindow::DrawOverlays(EditorContext& context) noexcept
{
	if (const auto selected = context.system.GetSelection().Get())
	{
		if (const auto bounds = WorldBoundsOf(context.registry, *selected))
		{
			DrawBox(*bounds, style::k_SelectionBox, 2.0f);
		}
	}
	if (!_placement.item.has_value() || ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}
	// The ghost of what is being placed, under the mouse
	const auto land = LandAt(MouseOnScreen());
	if (!land.has_value())
	{
		return;
	}
	const auto& item = *_placement.item;
	auto position = SnapPoint(*land, context.system.GetSnapping());
	position.y = LandHeight({position.x, position.z});
	const auto scale =
	    item.kind == PlaceKind::Creature
	        ? ecs::archetypes::CreatureArchetype::DrawnScale(static_cast<CreatureType>(item.type), _spawner.GetScale())
	        : 1.0f;
	const auto rotation = PlacedRotation(item.kind, _placement.yawRadians);
	auto box = AxisAlignedBoundingBox {.minima = glm::vec3(-2.0f, 0.0f, -2.0f), .maxima = glm::vec3(2.0f, 4.0f, 2.0f)};
	if (const auto mesh = MeshOf(item))
	{
		if (const auto meshBox = MeshBox(*mesh))
		{
			box = *meshBox;
		}
	}
	DrawBox(WorldBox(box, position, rotation, glm::vec3(scale)), style::k_GhostBox, 1.5f);

	// Which way it faces
	const auto& camera = Locator::camera::value();
	const auto display = ImGui::GetIO().DisplaySize;
	const glm::vec4 viewport {0.0f, 0.0f, display.x, display.y};
	const auto ahead = rotation * glm::vec3(0.0f, 0.0f, std::max(box.Size().z * scale * 0.75f, 3.0f));
	glm::vec3 from;
	glm::vec3 to;
	if (camera.ProjectWorldToScreen(position, viewport, from) && camera.ProjectWorldToScreen(position + ahead, viewport, to))
	{
		auto* drawList = ImGui::GetBackgroundDrawList();
		drawList->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), style::k_GhostBox, 2.5f);
		drawList->AddCircleFilled(ImVec2(to.x, to.y), 4.0f, style::k_GhostBox);
		const auto label =
		    fmt::format("{}  {:.0f} deg", context.names.NameOf(item.kind, item.type), glm::degrees(_placement.yawRadians));
		drawList->AddText(ImVec2(from.x + 12.0f, from.y + 4.0f), style::k_GhostBox, label.c_str());
	}
}

} // namespace openblack::editor
