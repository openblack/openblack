/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Controls.h"

namespace openblack::gui
{

/// A page of the game's menu (a DialogBoxBase and its SetupBox): a see-through box with tabs along its top, of which
/// one is open, and the controls in it.
///
/// The control under the pointer lights up. The mouse button going down on a control gives it the focus, and while the
/// button is held the control follows the pointer every frame. Letting go over the control the button went down on
/// makes it act; while the button is held no other control lights up.
class Dialog
{
public:
	struct Tab
	{
		std::u16string label;
		std::function<void()> onSelect;
	};

	/// MainMenu and the options' box: 780 by 500 in the middle of the dialog space, 40 deeper for its tabs
	static constexpr DialogRect k_Box {.min = {10, 50}, .max = {790, 590}};
	/// AddMainMenuTabs and AddOptionsTabs: five tabs, a fifth of the box's width each
	static constexpr size_t k_TabCount = 5;

	Dialog(std::vector<Tab> tabs, size_t selectedTab);
	/// Controls on their own, without a box or tabs
	Dialog() = default;

	template <typename T, typename... Args>
	T& Add(Args&&... args)
	{
		return static_cast<T&>(*_controls.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)));
	}

	void MouseMove(glm::ivec2 point);
	void MouseDown(glm::ivec2 point);
	/// True when a control acted, which SetupBox answers with the menu button sound
	bool MouseUp(glm::ivec2 point);
	void Wheel(glm::ivec2 point, int steps);
	bool TextInput(std::u16string_view text);
	bool KeyDown(int key);
	/// Every frame: the controls move on, and the one the mouse button is held down on follows the pointer
	void Update(float deltaSeconds);
	/// Forgets the pointer, the button and the focus
	void Reset();

	/// The box, its tabs and its controls. Inactive, behind a question, nothing lights up.
	void Draw(const DialogPainter& painter, bool active) const;

	[[nodiscard]] static DialogRect GetTabRect(size_t index);
	/// Names one of the tabs again
	void SetTabLabel(size_t index, std::u16string label);
	[[nodiscard]] Control* GetFocus() const noexcept { return _focused; }

private:
	[[nodiscard]] Control* HitTest(glm::ivec2 point) const;

	bool _hasBox {false};
	std::vector<std::unique_ptr<Control>> _controls;
	Control* _hovered {nullptr};
	Control* _pressed {nullptr};
	Control* _focused {nullptr};
	glm::ivec2 _pointer {-1, -1};
};

} // namespace openblack::gui
