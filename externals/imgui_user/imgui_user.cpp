#include "imgui.h"

#include <string>
#include <vector>

// https://eliasdaler.github.io/using-imgui-with-sfml-pt2/#using-imgui-with-stl

namespace ImGui
{
static auto vector_getter = [](void* vec, int idx, const char** out_text) {
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size()))
	{
		return false;
	}
	*out_text = vector.at(idx).c_str();
	return true;
};

struct string_view_array_getter_user_data_t
{
	const std::string_view* data;
	int count;
};

static auto string_view_array_getter = [](void* userData, int idx, const char** out_text) {
	auto arr = static_cast<string_view_array_getter_user_data_t*>(userData);
	if (idx < 0 || idx >= arr->count)
	{
		return false;
	}
	*out_text = arr->data[idx].data();
	return true;
};

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
{
	if (values.empty())
	{
		return false;
	}
	return Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

bool Combo(const char* label, int* currIndex, const std::string_view* values, int valueCount)
{
	if (valueCount <= 0)
	{
		return false;
	}
	string_view_array_getter_user_data_t userData = {
	    values,
	    valueCount,
	};
	return Combo(label, currIndex, string_view_array_getter, static_cast<void*>(&userData), valueCount);
}

bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
{
	if (values.empty())
	{
		return false;
	}
	return ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

bool ListBox(const char* label, int* currIndex, const std::string_view* values, int valueCount)
{
	if (valueCount <= 0)
	{
		return false;
	}
	string_view_array_getter_user_data_t userData = {
	    values,
	    valueCount,
	};
	return ListBox(label, currIndex, string_view_array_getter, static_cast<void*>(&userData), valueCount);
}

void AddUnderLine(ImColor col_)
{
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	min.y = max.y;
	ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

bool TextButtonColored(ImVec4 color, const char* name_)
{
	bool pressed = false;
	ImGui::TextColored(color, "%s", name_);
	if (ImGui::IsItemHovered())
	{
		if (ImGui::IsMouseClicked(0))
			pressed = true;

		AddUnderLine(color);
	}

	return pressed;
}

} // namespace ImGui
