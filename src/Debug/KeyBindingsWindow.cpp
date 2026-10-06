/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "KeyBindingsWindow.h"

#include <string>

#include <SDL_events.h>

#include "Input/GameActionMapInterface.h"
#include "Locator.h"

using namespace openblack::debug::gui;
using namespace openblack::input;

namespace
{
[[nodiscard]] ImVec4 StatusColour(BindStatus status)
{
	switch (status)
	{
	case BindStatus::Implemented:
		return {0.45f, 0.85f, 0.45f, 1.0f};
	case BindStatus::NotYetImplemented:
		return {0.95f, 0.55f, 0.35f, 1.0f};
	case BindStatus::OwnedByLeashWork:
	case BindStatus::OwnedByCreatureModeWork:
		return {0.55f, 0.70f, 0.95f, 1.0f};
	}
	return {1.0f, 1.0f, 1.0f, 1.0f};
}
} // namespace

KeyBindingsWindow::KeyBindingsWindow() noexcept
    : Window("Key Bindings", ImVec2(720.0f, 640.0f))
{
}

void KeyBindingsWindow::Draw() noexcept
{
	if (!openblack::Locator::gameActionSystem::has_value())
	{
		ImGui::TextUnformatted("No action map");
		return;
	}
	auto& actions = openblack::Locator::gameActionSystem::value();
	const auto bindings = actions.GetKeyBindings();

	if (ImGui::Button("Load Defaults"))
	{
		actions.ResetKeyBindings();
		_rebinding.reset();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Press tries an action out through the same path as its key");

	for (const auto& [first, second] : FindConflicts(bindings))
	{
		ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "\"%s\" and \"%s\" share a binding", bindings[first].name.data(),
		                   bindings[second].name.data());
	}
	if (_rebinding.has_value())
	{
		ImGui::TextColored({1.0f, 0.9f, 0.3f, 1.0f}, "Press a key for \"%s\" (Escape to cancel, Backspace to clear)",
		                   bindings[*_rebinding].name.data());
	}

	constexpr auto k_TableFlags =
	    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
	if (!ImGui::BeginTable("bindings", 6, k_TableFlags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Category");
	ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Key");
	ImGui::TableSetupColumn("Mouse");
	ImGui::TableSetupColumn("Status");
	ImGui::TableSetupColumn("");
	ImGui::TableHeadersRow();
	for (size_t i = 0; i < bindings.size(); ++i)
	{
		const auto& binding = bindings[i];
		ImGui::PushID(static_cast<int>(i));
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(CategoryName(binding.category).data());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(binding.name.data());
		ImGui::TableNextColumn();
		const auto keyName = binding.key.has_value() ? KeyChordName(*binding.key) : std::string("-");
		if (ImGui::SmallButton(_rebinding == i ? "..." : keyName.c_str()))
		{
			_rebinding = i;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Click to bind a new key");
		}
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(MouseInputName(binding.mouse).data());
		ImGui::TableNextColumn();
		ImGui::TextColored(StatusColour(binding.status), "%s", StatusName(binding.status).data());
		ImGui::TableNextColumn();
		if (ImGui::SmallButton("Press"))
		{
			actions.QueuePress(binding.action);
		}
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void KeyBindingsWindow::Update() noexcept {}

bool KeyBindingsWindow::TakesEvent(const SDL_Event& event) const noexcept
{
	// While a row waits for its key, the key is the window's
	return _rebinding.has_value() && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP);
}

void KeyBindingsWindow::ProcessEventOpen(const SDL_Event& event) noexcept
{
	if (!_rebinding.has_value() || (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) || event.key.repeat != 0 ||
	    !openblack::Locator::gameActionSystem::has_value())
	{
		return;
	}
	auto& actions = openblack::Locator::gameActionSystem::value();
	const auto action = actions.GetKeyBindings()[*_rebinding].action;
	const auto key = event.key.keysym.scancode;
	const auto finish = [this, &actions, action](std::optional<KeyChord> chord) {
		actions.SetKeyBinding(action, chord);
		_rebinding.reset();
		_modifierDown.reset();
	};
	if (event.type == SDL_KEYUP)
	{
		// A modifier key let go of before any other key is bound alone
		if (_modifierDown == key)
		{
			finish(KeyChord {.key = key});
		}
		return;
	}
	if (ModifierOfKey(key) != Modifier::None)
	{
		_modifierDown = key;
		return;
	}
	if (key == SDL_SCANCODE_ESCAPE)
	{
		_rebinding.reset();
		_modifierDown.reset();
		return;
	}
	if (key == SDL_SCANCODE_BACKSPACE)
	{
		finish(std::nullopt);
		return;
	}
	// Any other key takes the modifier held with it
	const auto held = event.key.keysym.mod;
	const auto modifier = (held & KMOD_CTRL) != 0    ? Modifier::Ctrl
	                      : (held & KMOD_SHIFT) != 0 ? Modifier::Shift
	                      : (held & KMOD_ALT) != 0   ? Modifier::Alt
	                                                 : Modifier::None;
	finish(KeyChord {.key = key, .modifier = modifier});
}

void KeyBindingsWindow::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
