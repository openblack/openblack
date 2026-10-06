/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FileBrowser.h"

#include <algorithm>
#include <array>
#include <system_error>

#include <imgui.h>

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
constexpr const char* k_PopupName = "Choose a file##FileBrowser";
} // namespace

void FileBrowser::Open(const file_dialog::Request& request)
{
	_request = request;
	_open = true;
	_popupShown = false;
	_filter = 0;
	_name = request.defaultName;
	_error.clear();
	std::error_code error;
	_folder = request.startFolder.empty() ? std::filesystem::current_path(error) : request.startFolder;
	List();
}

void FileBrowser::List()
{
	_folders.clear();
	_files.clear();
	std::error_code error;
	_folder = std::filesystem::absolute(_folder, error);
	for (const auto& entry : std::filesystem::directory_iterator(_folder, error))
	{
		std::error_code entryError;
		if (entry.is_directory(entryError))
		{
			_folders.push_back(entry.path());
		}
		else if (entry.is_regular_file(entryError) &&
		         (_request.filters.empty() ||
		          file_dialog::Matches(_request.filters.at(_filter), entry.path().filename().string())))
		{
			_files.push_back(entry.path());
		}
	}
	_error = error ? error.message() : std::string {};
	std::ranges::sort(_folders);
	std::ranges::sort(_files);
}

std::optional<std::filesystem::path> FileBrowser::Draw()
{
	if (!_open)
	{
		return std::nullopt;
	}
	if (!_popupShown)
	{
		ImGui::OpenPopup(k_PopupName);
		_popupShown = true;
	}
	ImGui::SetNextWindowSize(ImVec2(560.0f, 420.0f), ImGuiCond_Appearing);
	std::optional<std::filesystem::path> chosen;
	bool stillOpen = true;
	if (!ImGui::BeginPopupModal(k_PopupName, &stillOpen))
	{
		_open = false;
		return std::nullopt;
	}
	ImGui::TextUnformatted(_request.title.c_str());
	ImGui::TextWrapped("%s", _folder.generic_string().c_str());
	if (ImGui::Button("Up") && _folder.has_parent_path() && _folder.parent_path() != _folder)
	{
		_folder = _folder.parent_path();
		List();
	}
	if (!_request.filters.empty())
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::BeginCombo("Show", _request.filters.at(_filter).name.c_str()))
		{
			for (size_t i = 0; i < _request.filters.size(); ++i)
			{
				if (ImGui::Selectable(_request.filters[i].name.c_str(), i == _filter))
				{
					_filter = i;
					List();
				}
			}
			ImGui::EndCombo();
		}
	}
	if (!_error.empty())
	{
		ImGui::TextUnformatted(_error.c_str());
	}

	const float footer = ImGui::GetFrameHeightWithSpacing() * 2.0f;
	if (ImGui::BeginChild("entries", ImVec2(0.0f, -footer), ImGuiChildFlags_Borders))
	{
		std::optional<std::filesystem::path> enter;
		for (const auto& folder : _folders)
		{
			if (ImGui::Selectable(("[" + folder.filename().string() + "]").c_str(), false,
			                      ImGuiSelectableFlags_AllowDoubleClick) &&
			    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				enter = folder;
			}
		}
		for (const auto& file : _files)
		{
			const auto name = file.filename().string();
			if (ImGui::Selectable(name.c_str(), name == _name, ImGuiSelectableFlags_AllowDoubleClick))
			{
				_name = name;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					chosen = file;
				}
			}
		}
		if (enter.has_value())
		{
			_folder = *enter;
			List();
		}
	}
	ImGui::EndChild();

	std::array<char, 260> name {};
	std::copy_n(_name.begin(), std::min(_name.size(), name.size() - 1), name.begin());
	if (ImGui::InputText("File name", name.data(), name.size()))
	{
		_name = name.data();
	}
	const bool canChoose =
	    !_name.empty() &&
	    (_request.mode == file_dialog::Mode::Save ||
	     std::ranges::any_of(_files, [this](const std::filesystem::path& file) { return file.filename().string() == _name; }));
	ImGui::BeginDisabled(!canChoose);
	if (ImGui::Button(_request.mode == file_dialog::Mode::Open ? "Open" : "Save"))
	{
		chosen = _folder / _name;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		stillOpen = false;
	}
	if (chosen.has_value() || !stillOpen)
	{
		_open = false;
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
	return chosen;
}
