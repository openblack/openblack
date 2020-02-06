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

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
{
	if (values.empty())
	{
		return false;
	}
	return Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
{
	if (values.empty())
	{
		return false;
	}
	return ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

} // namespace ImGui
