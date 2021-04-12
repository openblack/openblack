// From https://gist.github.com/JSandusky/af0e94011aee31f7b05ed2257d347637

#include "imgui_bitfield.h"

#include <imgui_internal.h>

bool ImGuiBitField::BitField(const char* label, unsigned* bits, unsigned* hoverIndex)
{
	unsigned val = *bits;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	unsigned oldFlags = window->Flags;
	ImGuiContext* g = ImGui::GetCurrentContext();
	const ImGuiStyle& style = ImGui::GetStyle();
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, 0x0, true);
	const ImVec2 smallLabelSize = ImVec2(label_size.x * 0.5f, label_size.y * 0.5f);

	const float spacingUnit = 2.0f;

	bool anyPressed = false;
	ImVec2 currentPos = window->DC.CursorPos;
	for (unsigned i = 0; i < 32; ++i)
	{
		const void* lbl = (void*)(label + i);
		const ImGuiID localId = window->GetID(lbl);
		if (i == 16)
		{
			currentPos.x = window->DC.CursorPos.x;
			currentPos.y += smallLabelSize.y + style.FramePadding.y * 2 + spacingUnit /*little bit of space*/;
		}
		if (i == 8 || i == 24)
			currentPos.x += smallLabelSize.y;

		const ImRect check_bb(currentPos, { currentPos.x + smallLabelSize.y + style.FramePadding.y * 2, currentPos.y + smallLabelSize.y + style.FramePadding.y * 2 });

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(check_bb, localId, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
		if (pressed)
			*bits ^= (1 << i);

		if (hovered && hoverIndex)
			*hoverIndex = i;

		ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
		if (*bits & (1 << i))
		{
			const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
			const float pad = ImMax(spacingUnit, (float)(int)(check_sz / 4.0f));
			window->DrawList->AddRectFilled(
				{ check_bb.Min.x + pad, check_bb.Min.y + pad },
				{ check_bb.Max.x - pad, check_bb.Max.y - pad }, ImGui::GetColorU32(ImGuiCol_CheckMark));
		}

		anyPressed |= pressed;
		currentPos.x = check_bb.Max.x + spacingUnit;
	}

	const ImRect matrix_bb(window->DC.CursorPos,
	                       { window->DC.CursorPos.x + (smallLabelSize.y + style.FramePadding.y * 2) * 16 /*# of checks in a row*/ + smallLabelSize.y /*space between sets of 8*/ + 15 * spacingUnit /*spacing between each check*/,
	                         window->DC.CursorPos.y + ((smallLabelSize.y + style.FramePadding.y * 2) * 2 /*# of rows*/ + spacingUnit /*spacing between rows*/) });

	ImGui::ItemSize(matrix_bb, style.FramePadding.y);

	ImRect total_bb = matrix_bb;

	if (label_size.x > 0)
		ImGui::SameLine(0, style.ItemInnerSpacing.x);

	const ImRect text_bb({ window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y }, { window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y });
	if (label_size.x > 0)
	{
		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), matrix_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(matrix_bb.Min, text_bb.Min), ImMax(matrix_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	if (label_size.x > 0.0f)
		ImGui::RenderText(text_bb.GetTL(), label);

	window->Flags = oldFlags;
	return anyPressed;
}
