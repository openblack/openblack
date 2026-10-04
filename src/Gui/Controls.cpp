/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Controls.h"

#include <cmath>

#include <algorithm>
#include <chrono>

#include <SDL_keycode.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include "GameFont.h"

using namespace openblack::gui;

namespace
{
using Justify = DialogPainter::Justify;

/// SetupThing's colour for a control's text: orange under the pointer, white with the focus, grey otherwise
glm::vec4 TextColour(bool hovered, bool focused)
{
	if (hovered)
	{
		return DialogPainter::k_HoverColour;
	}
	return focused ? DialogPainter::k_FocusColour : DialogPainter::k_TextColour;
}

/// SetupBigButton labels and SetupSlider labels are at the mid text size
constexpr int k_LabelSize = DialogPainter::k_MidTextSize;
/// SetupSlider::Drag beside the knob
constexpr float k_SliderStep = 0.1f;
/// SetupList: the gap below each item's text
constexpr int k_ItemGap = 6;
} // namespace

StaticText::StaticText(DialogRect rect, std::u16string text, Layout layout, int size)
    : Control(rect)
    , text(std::move(text))
    , layout(layout)
    , size(size)
{
}

void StaticText::Draw(const DialogPainter& painter, bool /*hovered*/, bool /*focused*/, bool /*pressed*/) const
{
	if (!visible || text.empty())
	{
		return;
	}
	// SetupStaticText::Draw shrinks the text until it fits: wrapped text in the height and on one line in the width
	auto fitted = size;
	while (fitted > 10 && (painter.GetTextWidth(text, fitted) > static_cast<float>(rect.Width()) ||
	                       (layout == Layout::Wrapped &&
	                        painter.GetTextHeight(rect.Width(), text, fitted) > static_cast<float>(rect.Height()))))
	{
		--fitted;
	}

	const auto shadow = DialogPainter::k_ShadowColour;
	const auto white = DialogPainter::k_FocusColour;
	if (layout == Layout::Wrapped)
	{
		painter.DrawTextWrapped({.min = rect.min + 2, .max = rect.max + 2}, true, text, fitted, shadow);
		painter.DrawTextWrapped(rect, true, text, fitted, white);
		return;
	}
	const auto y = rect.Centre().y - (fitted / 2);
	auto x = rect.Centre().x;
	auto justify = Justify::Centre;
	if (layout == Layout::Left)
	{
		x = rect.min.x;
		justify = Justify::Left;
	}
	else if (layout == Layout::Right)
	{
		x = rect.max.x;
		justify = Justify::Right;
	}
	painter.DrawText({x + 2, y + 2}, rect.Width(), justify, text, fitted, shadow);
	painter.DrawText({x, y}, rect.Width(), justify, text, fitted, white);
}

Button::Button(DialogRect rect, std::u16string label, int size)
    : Control(rect)
    , label(std::move(label))
    , size(size)
{
}

void Button::Draw(const DialogPainter& painter, bool hovered, bool focused, bool /*pressed*/) const
{
	if (!visible)
	{
		return;
	}
	// SetupButton::Draw: the second style of box, the third under the pointer, and the label shrunk to fit
	painter.DrawBevelBox(rect, hovered ? 2 : 1, DialogPainter::All, glm::vec4(1.0f));
	auto fitted = size;
	while (fitted > 10 && painter.GetTextWidth(label, fitted) > static_cast<float>(rect.Width()))
	{
		--fitted;
	}
	painter.DrawText({rect.Centre().x, rect.Centre().y - (fitted / 2)}, rect.Width(), Justify::Centre, label, fitted,
	                 TextColour(hovered, focused));
}

BigButton::BigButton(const GameFont& font, glm::ivec2 position, int size, std::u16string label, LabelSide side, Look look)
    : Control({.min = position, .max = position + size})
    , label(std::move(label))
    , _font(font)
    , _side(side)
    , _look(look)
{
}

DialogRect BigButton::GetLabelRect() const
{
	const auto width = static_cast<int>(std::ceil(_font.GetWidth(label, static_cast<float>(_labelSize))));
	const auto top = rect.Centre().y - (_labelSize / 2);
	switch (_side)
	{
	case LabelSide::Right:
		return {.min = {rect.max.x, top}, .max = {rect.max.x + width + 2, top + _labelSize + 2}};
	case LabelSide::Left:
		return {.min = {rect.min.x - width, top}, .max = {rect.min.x + 2, top + _labelSize + 2}};
	case LabelSide::Below:
	default:
		return {.min = {rect.Centre().x - (width / 2), rect.max.y + 2},
		        .max = {rect.Centre().x + (width / 2) + 2, rect.max.y + 4 + _labelSize}};
	}
}

bool BigButton::HitTest(glm::ivec2 point) const
{
	return visible && (rect.Contains(point) || (!label.empty() && GetLabelRect().Contains(point)));
}

void BigButton::Draw(const DialogPainter& painter, bool hovered, bool /*focused*/, bool pressed) const
{
	if (!visible)
	{
		return;
	}
	switch (_look)
	{
	case Look::Square:
		painter.DrawSquare(rect.min, rect.Width(), _checked, hovered, pressed && hovered);
		break;
	case Look::LeftArrow:
	case Look::RightArrow:
		painter.DrawArrow(rect.min, rect.Width(),
		                  _look == Look::LeftArrow ? DialogPainter::Arrow::Left : DialogPainter::Arrow::Right, hovered,
		                  pressed && hovered);
		break;
	}
	DrawLabel(painter, hovered);
}

void BigButton::DrawLabel(const DialogPainter& painter, bool hovered) const
{
	if (label.empty())
	{
		return;
	}
	// SetupBigButton::Draw: a shadow two pixels down and right, then the label in white, orange under the pointer
	const auto colour = hovered ? DialogPainter::k_HoverColour : DialogPainter::k_FocusColour;
	const auto shadow = DialogPainter::k_ShadowColour;
	const auto top = rect.Centre().y - (_labelSize / 2);
	switch (_side)
	{
	case LabelSide::Right:
		painter.DrawText({rect.max.x + 2, top + 2}, 1000, Justify::Left, label, _labelSize, shadow);
		painter.DrawText({rect.max.x, top}, 1000, Justify::Left, label, _labelSize, colour);
		break;
	case LabelSide::Left:
		painter.DrawText({rect.min.x + 2, top + 2}, 1000, Justify::Right, label, _labelSize, shadow);
		painter.DrawText({rect.min.x, top}, 1000, Justify::Right, label, _labelSize, colour);
		break;
	case LabelSide::Below:
		painter.DrawText({rect.Centre().x + 2, rect.max.y + 4}, 1000, Justify::Centre, label, _labelSize, shadow);
		painter.DrawText({rect.Centre().x, rect.max.y + 2}, 1000, Justify::Centre, label, _labelSize, colour);
		break;
	}
}

CheckBox::CheckBox(const GameFont& font, glm::ivec2 position, std::u16string label, bool checked)
    // SetupCheckBox: 25 pixel squares, labelled below
    : BigButton(font, position, 25, std::move(label), LabelSide::Below, Look::Square)
{
	_checked = checked;
}

void CheckBox::Activate(glm::ivec2 point)
{
	_checked = !_checked;
	if (onChange)
	{
		onChange(_checked);
	}
	BigButton::Activate(point);
}

Slider::Slider(DialogRect rect, std::u16string label, float value)
    : Control(rect)
    , label(std::move(label))
    , _value(std::clamp(value, 0.0f, 1.0f))
{
}

void Slider::SetValue(float value) noexcept
{
	_value = std::clamp(value, 0.0f, 1.0f);
}

int Slider::GetKnobLeft(float value) const
{
	// The knob is as wide as the bar is high
	return rect.min.x + static_cast<int>(static_cast<float>(rect.Width() - rect.Height()) * value);
}

void Slider::Draw(const DialogPainter& painter, bool hovered, bool focused, bool /*pressed*/) const
{
	if (!visible)
	{
		return;
	}
	painter.DrawBevelBox(rect, 1, DialogPainter::All, glm::vec4(1.0f));
	const auto knob = GetKnobLeft(_value);
	painter.DrawSquare({knob + 3, rect.min.y + 3}, rect.Height() - 6, false, hovered || focused, true);
	painter.DrawText({rect.Centre().x, rect.Centre().y - (k_LabelSize / 2)}, rect.Width(), Justify::Centre, label, k_LabelSize,
	                 focused ? DialogPainter::k_FocusColour : DialogPainter::k_TextColour);
}

void Slider::MouseDown(glm::ivec2 point)
{
	_downX = point.x;
	_downValue = _value;
}

void Slider::Drag(glm::ivec2 point)
{
	// SetupSlider::Drag: the knob follows the pointer from where it was grabbed, or steps a tenth towards it
	const auto knob = GetKnobLeft(_downValue);
	const auto range = static_cast<float>(rect.Width() - rect.Height());
	const bool onKnob = _downX >= knob && _downX < knob + rect.Height();
	const auto step = _downX < knob ? -k_SliderStep : k_SliderStep;
	const auto moved = onKnob ? static_cast<float>(point.x - _downX) / range : step;
	const auto value = std::clamp(_downValue + moved, 0.0f, 1.0f);
	if (value != _value)
	{
		_value = value;
		if (onChange)
		{
			onChange(_value);
		}
	}
}

List::List(const GameFont& font, DialogRect rect, int size, bool centred, bool scrollBar, bool showSelection)
    : Control(rect)
    , _font(font)
    , _size(size)
    , _centred(centred)
    , _scrollBar(scrollBar)
    , _showSelection(showSelection)
{
}

int List::GetBoxRight() const
{
	return _scrollBar ? rect.max.x - 2 - GetScrollBarWidth() : rect.max.x;
}

int List::GetContentHeight() const
{
	int height = 0;
	for (const auto h : _heights)
	{
		height += h;
	}
	return height;
}

void List::SetItems(std::vector<Item> items)
{
	_items = std::move(items);
	_heights.clear();
	const auto width = static_cast<float>(GetBoxRight() - rect.min.x - 8);
	for (const auto& item : _items)
	{
		const auto lines = std::max<size_t>(1, _font.Wrap(item.text, static_cast<float>(_size), width).size());
		// SetupList::UpdateHeights measures the item's text with GatheringText::DrawText, which leaves a gap below it
		// TODO(raffclar): the gap is measured from the original's screens, not worked out from its code
		_heights.push_back((static_cast<int>(lines) * _size) + k_ItemGap);
	}
	if (_selection && *_selection >= _items.size())
	{
		_selection.reset();
	}
	ScrollTo(_scroll);
}

std::optional<size_t> List::ItemAt(int y) const
{
	auto top = rect.min.y - static_cast<int>(_scroll);
	for (size_t i = 0; i < _items.size(); ++i)
	{
		if (y >= top && y < top + _heights[i])
		{
			return i;
		}
		top += _heights[i];
	}
	return std::nullopt;
}

void List::ScrollTo(float scroll)
{
	const auto most = static_cast<float>(std::max(0, GetContentHeight() - rect.Height()));
	_scroll = std::clamp(scroll, 0.0f, most);
}

void List::Draw(const DialogPainter& painter, bool hovered, bool focused, bool /*pressed*/) const
{
	if (!visible)
	{
		return;
	}
	const auto boxRight = GetBoxRight();
	painter.DrawBevelBox({.min = rect.min, .max = {boxRight, rect.max.y}}, 1, DialogPainter::All, glm::vec4(1.0f));

	const auto hoveredItem = hovered && _pointer.x < boxRight ? ItemAt(_pointer.y) : std::nullopt;
	const auto alpha = painter.GetAlpha();
	auto top = rect.min.y - static_cast<int>(_scroll);
	for (size_t i = 0; i < _items.size(); ++i)
	{
		const auto bottom = top + _heights[i];
		if (bottom > rect.min.y && top < rect.max.y)
		{
			const auto& item = _items[i];
			const auto selected = _showSelection && _selection == i;
			if (selected)
			{
				painter.DrawBevelBox(
				    {.min = {rect.min.x, std::max(top, rect.min.y)}, .max = {boxRight, std::min(bottom, rect.max.y)}}, 0,
				    DialogPainter::All, glm::vec4(1.0f));
			}
			const auto colour = selected ? DialogPainter::k_FocusColour : TextColour(hoveredItem == i, focused);

			// SetupList::Draw fades the lines the box cuts through
			const auto left = rect.min.x + 4;
			const auto right = boxRight - 4;
			const auto lines = painter.GetLines(right - left, item.text, _size);
			for (size_t l = 0; l < lines.size(); ++l)
			{
				const auto lineTop = top + 2 + (static_cast<int>(l) * _size);
				const auto inside = std::min(lineTop + _size, rect.max.y) - std::max(lineTop, rect.min.y);
				if (inside <= 0)
				{
					continue;
				}
				painter.SetAlpha(alpha * static_cast<float>(inside) / static_cast<float>(_size));
				const auto lineWidth = painter.GetTextWidth(lines[l], _size);
				const auto lineLeft = _centred ? ((left + right) / 2) - (static_cast<int>(lineWidth) / 2) : left;
				painter.DrawText({lineLeft, lineTop}, right - left, Justify::Left, lines[l], _size, colour);

				// DialogBoxKeyBinding's mice, in the gap of five spaces before the button's name
				if (l == 0 && item.mouseCell && lineTop >= rect.min.y && lineTop + _size <= rect.max.y)
				{
					const auto bracket = lines[l].find(u'(');
					const auto prefix =
					    bracket != std::u16string_view::npos && bracket >= 6 ? lines[l].substr(0, bracket - 6) : lines[l];
					const auto x = lineLeft + static_cast<int>(painter.GetTextWidth(prefix, _size));
					painter.DrawMouse({.min = {x, lineTop}, .max = {x + _size, lineTop + _size}}, *item.mouseCell,
					                  item.mouseMirrored);
				}
			}
			painter.SetAlpha(alpha);
		}
		top = bottom;
	}

	if (_scrollBar)
	{
		const auto barLeft = boxRight + 2;
		painter.DrawBevelBox({.min = {barLeft, rect.min.y}, .max = {barLeft + GetScrollBarWidth(), rect.max.y}}, 1,
		                     DialogPainter::All, glm::vec4(1.0f));
		const auto track = static_cast<float>(rect.Height() - 6);
		const auto content = static_cast<float>(std::max(GetContentHeight(), rect.Height()));
		const auto thumbTop = static_cast<int>(track * _scroll / content);
		const auto thumbBottom = static_cast<int>(track * (_scroll + static_cast<float>(rect.Height())) / content);
		painter.DrawBevelBox({.min = {barLeft + 3, rect.min.y + 3 + thumbTop},
		                      .max = {barLeft + GetScrollBarWidth() - 3, rect.min.y + 3 + thumbBottom}},
		                     0, DialogPainter::All, glm::vec4(1.0f));
	}
}

void List::MouseDown(glm::ivec2 point)
{
	_draggingThumb = false;
	if (_scrollBar && point.x >= GetBoxRight())
	{
		const auto track = static_cast<float>(rect.Height() - 6);
		const auto content = static_cast<float>(std::max(GetContentHeight(), rect.Height()));
		const auto thumbTop = rect.min.y + 3 + static_cast<int>(track * _scroll / content);
		const auto thumbBottom =
		    rect.min.y + 3 + static_cast<int>(track * (_scroll + static_cast<float>(rect.Height())) / content);
		if (point.y >= thumbTop && point.y < thumbBottom)
		{
			_draggingThumb = true;
			_grabY = point.y;
			_grabScroll = _scroll;
		}
		else
		{
			// A page up or down
			ScrollTo(_scroll + static_cast<float>(point.y < thumbTop ? -rect.Height() : rect.Height()));
		}
		return;
	}
	if (const auto item = ItemAt(point.y))
	{
		_selection = item;
	}
}

void List::Drag(glm::ivec2 point)
{
	if (_draggingThumb)
	{
		const auto track = static_cast<float>(rect.Height() - 6);
		const auto content = static_cast<float>(std::max(GetContentHeight(), rect.Height()));
		ScrollTo(_grabScroll + (static_cast<float>(point.y - _grabY) * content / track));
	}
}

void List::Wheel(int steps)
{
	ScrollTo(_scroll - static_cast<float>(steps * _size));
}

EditBox::EditBox(DialogRect rect, std::u16string text, size_t maxLength)
    : Control(rect)
    , _text(std::move(text))
    , _maxLength(maxLength)
    , _caret(_text.size())
{
	if (_text.size() > _maxLength)
	{
		_text.resize(_maxLength);
		_caret = _maxLength;
	}
}

void EditBox::Draw(const DialogPainter& painter, bool hovered, bool focused, bool /*pressed*/) const
{
	if (!visible)
	{
		return;
	}
	constexpr auto k_Size = DialogPainter::k_MidTextSize;
	painter.DrawBevelBox(rect, 1, DialogPainter::All, glm::vec4(1.0f));
	const glm::ivec2 position {rect.min.x + 4, rect.Centre().y - (k_Size / 2)};
	painter.DrawText(position, rect.Width() - 8, Justify::Left, _text, k_Size, TextColour(hovered && !focused, focused));
	if (focused)
	{
		// A caret blinking twice a second
		const auto now = std::chrono::steady_clock::now().time_since_epoch();
		if ((std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 250) % 2 == 0)
		{
			const auto x =
			    position.x + static_cast<int>(painter.GetTextWidth(std::u16string_view(_text).substr(0, _caret), k_Size));
			painter.DrawLine({x, position.y + 2}, {x, position.y + k_Size - 2}, DialogPainter::k_FocusColour);
		}
	}
}

void EditBox::MouseDown(glm::ivec2 /*point*/)
{
	_caret = _text.size();
}

bool EditBox::TextInput(std::u16string_view text)
{
	for (const auto c : text)
	{
		if (_text.size() >= _maxLength || c < u' ')
		{
			break;
		}
		_text.insert(_caret++, 1, c);
	}
	if (onChange)
	{
		onChange(_text);
	}
	return true;
}

bool EditBox::KeyDown(int key)
{
	switch (key)
	{
	case SDLK_BACKSPACE:
		if (_caret > 0)
		{
			_text.erase(--_caret, 1);
		}
		break;
	case SDLK_DELETE:
		if (_caret < _text.size())
		{
			_text.erase(_caret, 1);
		}
		break;
	case SDLK_LEFT:
		_caret = _caret > 0 ? _caret - 1 : 0;
		return true;
	case SDLK_RIGHT:
		_caret = std::min(_caret + 1, _text.size());
		return true;
	case SDLK_HOME:
		_caret = 0;
		return true;
	case SDLK_END:
		_caret = _text.size();
		return true;
	default:
		return false;
	}
	if (onChange)
	{
		onChange(_text);
	}
	return true;
}

namespace
{
// SetupPicture::Draw: the ring of symbols comes up over half a second and goes over a second
constexpr float k_RingOpenSeconds = 0.5f;
constexpr float k_RingCloseSeconds = 1.0f;
// A third of the symbols are on the inner ring, the others on the outer
constexpr int k_InnerCount = (SymbolPicture::k_SymbolCount / 3) + 1;
// The disc is cut into 32 slices, its edges fading out over three pixels
constexpr int k_DiscSlices = 32;
constexpr float k_DiscEdge = 3.0f;
// As the ring comes up the inner ring turns into place one way and the outer the other, from a quarter turn
constexpr float k_RingTurn = 0.006159986f;
constexpr int k_Opaque = 255;
// Symbols can only be picked once the ring is all the way up
constexpr int k_Pickable = 0xFD;

constexpr float k_TwoPi = glm::two_pi<float>();
} // namespace

SymbolPicture::SymbolPicture(DialogRect rect, int symbol)
    : Control(rect)
    , _symbol(std::clamp(symbol, 0, k_SymbolCount - 1))
{
}

std::pair<glm::vec2, float> SymbolPicture::GetRingSymbol(int symbol, float opening) const
{
	// The symbols' step is three quarters of half the picture: the inner ring a step out from the picture's edge and
	// the outer three, each symbol a step either way of its centre
	const auto halfSize = rect.Height() / 2;
	const auto step = static_cast<int>(static_cast<float>(halfSize) * 0.75f);
	const auto alpha = static_cast<int>(opening * static_cast<float>(k_Opaque));
	const auto turn = static_cast<float>(k_Opaque - alpha) * k_RingTurn;
	const auto inner = symbol < k_InnerCount;
	const auto count = inner ? k_InnerCount : k_SymbolCount - k_InnerCount;
	const auto index = inner ? symbol : symbol - k_InnerCount;
	const auto radius = static_cast<float>(halfSize + (inner ? step : step * 3));
	const auto angle = (static_cast<float>(index) * k_TwoPi / static_cast<float>(count)) + (inner ? turn : -turn);
	const auto centre = glm::vec2(rect.Centre()) + glm::vec2(std::sin(angle), -std::cos(angle)) * radius;
	return {glm::floor(centre), static_cast<float>((step * alpha) >> 8)};
}

DialogRect SymbolPicture::GetRingRect(int symbol) const
{
	const auto [centre, half] = GetRingSymbol(symbol, 1.0f);
	return {.min = glm::ivec2(centre - half), .max = glm::ivec2(centre + half)};
}

std::optional<int> SymbolPicture::RingSymbolAt(glm::ivec2 point) const
{
	if (static_cast<int>(GetRingOpening() * static_cast<float>(k_Opaque)) <= k_Pickable)
	{
		return std::nullopt;
	}
	for (int s = 0; s < k_SymbolCount; ++s)
	{
		// Within the circle around the symbol
		const auto [centre, half] = GetRingSymbol(s, GetRingOpening());
		const auto offset = glm::vec2(point) - centre;
		if (glm::dot(offset, offset) < half * half)
		{
			return s;
		}
	}
	return std::nullopt;
}

bool SymbolPicture::HitTest(glm::ivec2 point) const
{
	if (!visible)
	{
		return false;
	}
	if (rect.Contains(point))
	{
		return true;
	}
	// While the button is held the whole disc is the picture's
	const auto halfSize = rect.Height() / 2;
	const auto outer = static_cast<float>(halfSize + (static_cast<int>(static_cast<float>(halfSize) * 0.75f) * 4)) + k_DiscEdge;
	const auto offset = glm::vec2(point - rect.Centre());
	return _ringOpen && glm::dot(offset, offset) <= outer * outer;
}

void SymbolPicture::Draw(const DialogPainter& painter, bool hovered, bool focused, bool /*pressed*/) const
{
	if (!visible)
	{
		return;
	}
	const auto shadow = DialogPainter::k_ShadowColour;
	painter.DrawSymbol({.min = rect.min + 2, .max = rect.max + 2}, _symbol, shadow);
	painter.DrawSymbol(rect, _symbol, TextColour(hovered, focused));

	const auto opening = GetRingOpening();
	const auto alpha = static_cast<int>(opening * static_cast<float>(k_Opaque));
	if (alpha <= 5)
	{
		return;
	}

	// The disc: black at half the ring's opacity from the picture's edge to a step beyond the outer ring
	const auto centre = glm::vec2(rect.Centre());
	const auto halfSize = rect.Height() / 2;
	const auto step = static_cast<int>(static_cast<float>(halfSize) * 0.75f);
	const auto outer = static_cast<float>(halfSize + (step * 4));
	const auto inner = static_cast<float>(halfSize);
	// The disc is half as opaque as the symbols, an integer halving like the game's
	const int halfAlpha = alpha / 2;
	const auto disc = glm::vec4(0.0f, 0.0f, 0.0f, static_cast<float>(halfAlpha) / 255.0f);
	const auto clear = glm::vec4(0.0f);
	for (int i = 0; i < k_DiscSlices; ++i)
	{
		const auto a0 = static_cast<float>(i) * k_TwoPi / static_cast<float>(k_DiscSlices);
		const auto a1 = static_cast<float>(i + 1) * k_TwoPi / static_cast<float>(k_DiscSlices);
		const auto d0 = glm::vec2(std::cos(a0), std::sin(a0));
		const auto d1 = glm::vec2(std::cos(a1), std::sin(a1));
		const auto band = [&](float r0, float r1, glm::vec4 c0, glm::vec4 c1) {
			painter.DrawShape({centre + d0 * r0, centre + d1 * r0, centre + d1 * r1, centre + d0 * r1}, {c0, c0, c1, c1});
		};
		band(outer, inner, disc, disc);
		band(outer + k_DiscEdge, outer, clear, disc);
		band(inner, inner - k_DiscEdge, disc, clear);
	}

	// The symbols, growing and turning into place, the one under the pointer orange
	const auto saved = painter.GetAlpha();
	painter.SetAlpha(saved * static_cast<float>(alpha) / 256.0f);
	const auto under = RingSymbolAt(_pointer);
	for (int s = 0; s < k_SymbolCount; ++s)
	{
		const auto [symbolCentre, half] = GetRingSymbol(s, opening);
		const auto min = glm::ivec2(symbolCentre - half);
		const auto max = glm::ivec2(symbolCentre + half);
		painter.DrawSymbol({.min = min + 2, .max = max + 2}, s, shadow);
		painter.DrawSymbol({.min = min, .max = max}, s,
		                   under == s ? DialogPainter::k_HoverColour : DialogPainter::k_FocusColour);
	}
	painter.SetAlpha(saved);
}

void SymbolPicture::MouseDown(glm::ivec2 /*point*/)
{
	_ringOpen = true;
	_ring.SetDestination(1.0f, k_RingOpenSeconds);
}

void SymbolPicture::Release()
{
	_ringOpen = false;
	_ring.SetDestination(0.0f, k_RingCloseSeconds);
}

void SymbolPicture::Activate(glm::ivec2 point)
{
	// SetupPicture::MouseUp picks the symbol under the pointer
	if (const auto symbol = RingSymbolAt(point); _ringOpen && symbol)
	{
		_symbol = *symbol;
		if (onChange)
		{
			onChange(_symbol);
		}
	}
	Release();
	Control::Activate(point);
}
