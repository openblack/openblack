/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Dialog.h"

#include <algorithm>
#include <string_view>
#include <utility>

using namespace openblack::gui;

namespace
{
/// SetupTabButton: a tab along the top of the box
class TabControl final: public Control
{
public:
	TabControl(DialogRect rect, std::u16string label, bool selected, bool first, bool last, std::function<void()> select)
	    : Control(rect)
	    , _label(std::move(label))
	    , _selected(selected)
	    , _first(first)
	    , _last(last)
	{
		onClick = std::move(select);
	}

	void Draw(const DialogPainter& painter, bool hovered, bool /*focused*/, bool /*pressed*/) const override
	{
		const auto alpha = painter.GetAlpha();
		painter.DrawTab(rect, _label, _selected, _first, _last, hovered);
		painter.SetAlpha(alpha);
	}

	void SetLabel(std::u16string label) { _label = std::move(label); }

	// Tabs without a label are only the line along the top of the box
	[[nodiscard]] bool HitTest(glm::ivec2 point) const override { return !_label.empty() && Control::HitTest(point); }
	[[nodiscard]] bool IsInteractive() const override { return !_label.empty(); }

private:
	std::u16string _label;
	bool _selected;
	bool _first;
	bool _last;
};
} // namespace

Dialog::Dialog(std::vector<Tab> tabs, size_t selectedTab)
    : _hasBox(true)
{
	tabs.resize(k_TabCount);
	for (size_t i = 0; i < k_TabCount; ++i)
	{
		auto onSelect = i == selectedTab ? std::function<void()>() : std::move(tabs[i].onSelect);
		Add<TabControl>(GetTabRect(i), std::move(tabs[i].label), i == selectedTab, i == 0, i + 1 == k_TabCount,
		                std::move(onSelect));
	}
}

void Dialog::SetTabLabel(size_t index, std::u16string label)
{
	if (_hasBox && index < k_TabCount)
	{
		static_cast<TabControl&>(*_controls.at(index)).SetLabel(std::move(label));
	}
}

DialogRect Dialog::GetTabRect(size_t index)
{
	const auto width = static_cast<float>(k_Box.Width()) * 0.2f;
	const auto left = static_cast<float>(k_Box.min.x) + (width * static_cast<float>(index));
	// AddMainMenuTabs: 0x104 less half the box's height up, 40 high
	return {.min = {static_cast<int>(left), 10}, .max = {static_cast<int>(left + width), 50}};
}

Control* Dialog::HitTest(glm::ivec2 point) const
{
	// The control the mouse button is held down on keeps the pointer, as SetupPicture's ring over its neighbours
	if (_pressed != nullptr && _pressed->HitTest(point))
	{
		return _pressed;
	}
	// The last drawn is on top
	for (auto it = _controls.rbegin(); it != _controls.rend(); ++it)
	{
		if ((*it)->IsInteractive() && (*it)->HitTest(point))
		{
			return it->get();
		}
	}
	return nullptr;
}

void Dialog::MouseMove(glm::ivec2 point)
{
	_pointer = point;
	for (const auto& control : _controls)
	{
		control->MouseMove(point);
	}
	_hovered = HitTest(point);
	// While the button is down only the control it went down on lights up
	if (_pressed != nullptr && _hovered != _pressed)
	{
		_hovered = nullptr;
	}
}

void Dialog::MouseDown(glm::ivec2 point)
{
	MouseMove(point);
	_pressed = HitTest(point);
	_hovered = _pressed;
	if (_pressed != nullptr)
	{
		_focused = _pressed;
		_pressed->MouseDown(point);
		_pressed->Drag(point);
	}
}

bool Dialog::MouseUp(glm::ivec2 point)
{
	MouseMove(point);
	auto* released = HitTest(point);
	auto* pressed = std::exchange(_pressed, nullptr);
	_hovered = released;
	if (auto* picture = dynamic_cast<SymbolPicture*>(pressed); picture != nullptr && released != pressed)
	{
		picture->Release();
	}
	if (pressed == nullptr || released != pressed)
	{
		return false;
	}
	pressed->Activate(point);
	return true;
}

void Dialog::Wheel(glm::ivec2 point, int steps)
{
	if (auto* control = HitTest(point))
	{
		control->Wheel(steps);
	}
}

bool Dialog::TextInput(std::u16string_view text)
{
	return _focused != nullptr && _focused->TextInput(text);
}

bool Dialog::KeyDown(int key)
{
	return _focused != nullptr && _focused->KeyDown(key);
}

void Dialog::Update(float deltaSeconds)
{
	for (const auto& control : _controls)
	{
		control->Update(deltaSeconds);
	}
	if (_pressed != nullptr)
	{
		_pressed->Drag(_pointer);
	}
}

void Dialog::Reset()
{
	_hovered = nullptr;
	_pressed = nullptr;
	_focused = nullptr;
	if (auto it =
	        std::ranges::find_if(_controls, [](const auto& c) { return dynamic_cast<SymbolPicture*>(c.get()) != nullptr; });
	    it != _controls.end())
	{
		static_cast<SymbolPicture&>(**it).Release();
	}
}

void Dialog::Draw(const DialogPainter& painter, bool active) const
{
	// A box with tabs opens onto the selected one along its top
	if (_hasBox)
	{
		painter.DrawBackground(k_Box, glm::vec3(1.0f), false, DialogPainter::Bottom);
	}
	const auto alpha = painter.GetAlpha();
	for (const auto onTop : {false, true})
	{
		for (const auto& control : _controls)
		{
			if (control->IsOnTop() != onTop)
			{
				continue;
			}
			const auto* c = control.get();
			control->Draw(painter, active && _hovered == c, active && _focused == c, active && _pressed == c);
			painter.SetAlpha(alpha);
		}
	}
}
