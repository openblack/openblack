#include <array>
#include <string>
#include <vector>

namespace ImGui
{

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);
bool Combo(const char* label, int* currIndex, const std::string_view* values, int valueCount);
template <typename Index_t, std::size_t valueCount>
inline bool Combo(const char* label, Index_t* currIndex, const std::array<std::string_view, valueCount>& values)
{
	int currIndexInt = static_cast<int>(*currIndex);
	if (Combo(label, &currIndexInt, values.data(), static_cast<int>(valueCount)))
	{
		*currIndex = static_cast<Index_t>(currIndexInt);
	}
	return false;
}
bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values);
bool ListBox(const char* label, int* currIndex, const std::string_view* values, int valueCount);
template <typename Index_t, std::size_t valueCount>
inline bool ListBox(const char* label, Index_t* currIndex, const std::array<std::string_view, valueCount>& values)
{
	int currIndexInt = static_cast<int>(*currIndex);
	if (ListBox(label, &currIndexInt, values.data(), static_cast<int>(valueCount)))
	{
		*currIndex = static_cast<Index_t>(currIndexInt);
	}
	return false;
}
void AddUnderLine(ImColor col_);
bool TextButtonColored(ImVec4 color, const char* name_);

} // namespace ImGui
