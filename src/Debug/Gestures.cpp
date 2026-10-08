/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Gestures.h"

#include <algorithm>
#include <numbers>

#include <GestureFile.h>
#include <fmt/format.h>

#include "Camera/Camera.h"
#include "ECS/Systems/GestureSystemInterface.h"
#include "Gestures/GesturePaths.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using ecs::systems::GestureEvent;

namespace
{
constexpr ImU32 k_PathColour = IM_COL32(255, 255, 255, 160);
constexpr ImU32 k_CornerColour = IM_COL32(255, 220, 60, 255);
constexpr ImU32 k_StartColour = IM_COL32(80, 255, 80, 255);
constexpr ImU32 k_EndColour = IM_COL32(255, 80, 80, 255);
constexpr ImU32 k_TemplateColour = IM_COL32(60, 200, 255, 220);
constexpr ImU32 k_RecognisedColour = IM_COL32(120, 255, 120, 220);
/// How long a recognised gesture's box stays over the screen
constexpr float k_RecognisedShownSeconds = 2.0f;

/// From the screen the path is measured on to ImGui's
float ToDisplay()
{
	return ImGui::GetIO().DisplaySize.y / gesture::k_ReferenceHeight;
}

ImVec2 Display(glm::vec2 point, float scale)
{
	return {point.x * scale, point.y * scale};
}

float Degrees(float radians)
{
	return radians * 180.0f / std::numbers::pi_v<float>;
}

std::string Describe(const gestures::GestureTemplate& gestureTemplate, size_t index)
{
	return fmt::format("{} (template {})", gesture::Name(static_cast<GestureType>(gestureTemplate.Gesture())), index);
}
} // namespace

Gestures::Gestures() noexcept
    : Window("Gestures", ImVec2(460.0f, 520.0f))
{
}

void Gestures::WindowDraw() noexcept
{
	Window::WindowDraw();
	if (IsOpen() || !_overlay || !Locator::gestureSystem::has_value())
	{
		return;
	}
	const auto& system = Locator::gestureSystem::value();
	const auto recognised = system.GetLastRecognised();
	if (system.IsDrawingPath() || (recognised.has_value() && recognised->age < k_RecognisedShownSeconds))
	{
		DrawOverlay();
	}
}

void Gestures::Draw() noexcept
{
	if (!Locator::gestureSystem::has_value())
	{
		ImGui::TextUnformatted("No gesture system");
		return;
	}
	ImGui::Checkbox("Draw the hand's path over the screen", &_overlay);
	if (_overlay)
	{
		DrawOverlay();
	}
	DrawState();
	ImGui::Separator();
	DrawTools();
}

void Gestures::DrawOverlay() const noexcept
{
	const auto& system = Locator::gestureSystem::value();
	const auto& path = system.GetPath();
	auto* list = ImGui::GetForegroundDrawList();
	const auto scale = ToDisplay();
	for (size_t i = 1; i < path.Count(); ++i)
	{
		list->AddLine(Display(path.At(i - 1).screen, scale), Display(path.At(i).screen, scale), k_PathColour, 2.0f);
	}
	for (size_t i = 0; i < path.Count(); ++i)
	{
		const auto& point = path.At(i);
		const auto colour = point.key == gesture::KeyType::Corner  ? k_CornerColour
		                    : point.key == gesture::KeyType::Start ? k_StartColour
		                    : point.key == gesture::KeyType::End   ? k_EndColour
		                                                           : 0u;
		if (colour != 0u)
		{
			list->AddCircleFilled(Display(point.screen, scale), 5.0f, colour);
		}
	}

	// The template that fits the path most closely, laid over the stretch of the path it fits
	const auto templates = system.GetTemplates();
	const auto keys = path.KeyPoints();
	if (const auto closest = gesture::ClosestMatch(templates, keys, system.GetScreenAspect()))
	{
		const auto& entry = templates[closest->templateIndex];
		gesture::ScreenBox box {.min = keys[closest->firstKey].screen, .max = keys[closest->firstKey].screen};
		for (auto i = closest->firstKey; i <= closest->lastKey && i < keys.size(); ++i)
		{
			box.min = glm::min(box.min, keys[i].screen);
			box.max = glm::max(box.max, keys[i].screen);
		}
		const auto extent = std::max(box.max.x - box.min.x, box.max.y - box.min.y);
		const auto points = entry.Points();
		for (size_t i = 1; i < points.size(); ++i)
		{
			const auto mirror = [&closest](float x) { return closest->mirrored ? 1.0f - x : x; };
			const auto from = box.min + (glm::vec2(mirror(points[i - 1].x), points[i - 1].z) * extent);
			const auto to = box.min + (glm::vec2(mirror(points[i].x), points[i].z) * extent);
			list->AddLine(Display(from, scale), Display(to, scale), k_TemplateColour, 2.0f);
		}
		const auto label = fmt::format("{}{}, off by {:.0f} degrees at most", Describe(entry, closest->templateIndex),
		                               closest->mirrored ? " mirrored" : "", Degrees(closest->largestDifference));
		list->AddText(Display({box.min.x, box.max.y + 8.0f}, scale), k_TemplateColour, label.c_str());
	}

	if (const auto recognised = system.GetLastRecognised();
	    recognised.has_value() && recognised->age < k_RecognisedShownSeconds)
	{
		list->AddRect(Display(recognised->box.min, scale), Display(recognised->box.max, scale), k_RecognisedColour, 0.0f, 0,
		              3.0f);
		const auto label =
		    fmt::format("{}: {}", gesture::Name(recognised->request.gesture), gesture::Name(recognised->request.purpose));
		list->AddText(Display({recognised->box.min.x, recognised->box.min.y - 18.0f}, scale), k_RecognisedColour,
		              label.c_str());
	}
}

void Gestures::DrawState() noexcept
{
	const auto& system = Locator::gestureSystem::value();
	const auto templates = system.GetTemplates();
	ImGui::Text("%zu templates, %zu points recorded, %zu key points", templates.size(), system.GetPath().Count(),
	            system.GetPath().KeyPoints().size());
	ImGui::Text("Leash picker: %s", system.IsLeashPickerOpen() ? "open" : "closed");
	if (system.GetCircleSecondsLeft() > 0.0f)
	{
		ImGui::Text("Circle remembered for %.1f s more", static_cast<double>(system.GetCircleSecondsLeft()));
	}

	ImGui::SeparatorText("Waiting for");
	if (system.GetRequests().empty())
	{
		ImGui::TextDisabled("nothing");
	}
	for (const auto& request : system.GetRequests())
	{
		ImGui::BulletText("%s: %s", gesture::Name(request.gesture).data(), gesture::Name(request.purpose).data());
	}

	ImGui::SeparatorText("Last recognised");
	if (const auto recognised = system.GetLastRecognised())
	{
		ImGui::Text("%s (%s), %.1f s ago", gesture::Name(recognised->request.gesture).data(),
		            gesture::Name(recognised->request.purpose).data(), static_cast<double>(recognised->age));
		ImGui::Text("by template %zu%s, turns off by %.0f degrees at most", recognised->match.templateIndex,
		            recognised->match.mirrored ? " mirrored" : "",
		            static_cast<double>(Degrees(recognised->match.largestDifference)));
		if (recognised->event.has_value() && recognised->event->kind == GestureEvent::Kind::Circle)
		{
			const auto& centre = recognised->event->centre;
			ImGui::Text("circle at %.1f, %.1f, %.1f, radius %.1f", static_cast<double>(centre.x), static_cast<double>(centre.y),
			            static_cast<double>(centre.z), static_cast<double>(recognised->event->radius));
		}
	}
	else
	{
		ImGui::TextDisabled("nothing yet");
	}
}

void Gestures::DrawTools() noexcept
{
	auto& system = Locator::gestureSystem::value();
	// Draw a gesture through the recogniser, from its first template that is recognised
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::BeginCombo("##gesture", gesture::Name(static_cast<GestureType>(_gesture)).data()))
	{
		for (int i = 1; i <= 23; ++i)
		{
			if (ImGui::Selectable(gesture::Name(static_cast<GestureType>(i)).data(), i == _gesture))
			{
				_gesture = i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Draw it"))
	{
		const auto aspect = system.GetScreenAspect();
		const glm::vec2 middle {gesture::k_ReferenceHeight * aspect * 0.5f, gesture::k_ReferenceHeight * 0.5f};
		if (auto path =
		        gesture::TraceGesture(system.GetTemplates(), static_cast<GestureType>(_gesture), middle, 320.0f, aspect))
		{
			system.DrawPath(std::move(*path), _holdAction);
		}
	}
	ImGui::SameLine();
	ImGui::Checkbox("Holding Action", &_holdAction);
	if (ImGui::Button("Forget the path"))
	{
		system.ForgetPath();
	}

	// Straight to the miracles, as if drawn
	ImGui::SeparatorText("Tell the miracles");
	const auto focus = Locator::camera::has_value() ? Locator::camera::value().GetFocus() : glm::vec3(0.0f);
	if (ImGui::Button("Circle at the camera's focus"))
	{
		system.Inject({.kind = GestureEvent::Kind::Circle, .gesture = GestureType::Circle, .centre = focus, .radius = 30.0f});
	}
	ImGui::SameLine();
	if (ImGui::Button("Power up 0"))
	{
		system.Inject({.kind = GestureEvent::Kind::PowerUp, .centre = focus, .powerUpLevel = 0});
	}
	ImGui::SameLine();
	if (ImGui::Button("Power up 1"))
	{
		system.Inject({.kind = GestureEvent::Kind::PowerUp, .centre = focus, .powerUpLevel = 1});
	}
	ImGui::SameLine();
	if (ImGui::Button("Scribble"))
	{
		system.Inject({.kind = GestureEvent::Kind::Scribble, .gesture = GestureType::Scribble, .centre = focus});
	}
}
