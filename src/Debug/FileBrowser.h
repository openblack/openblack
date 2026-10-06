/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Common/FileDialog.h"

namespace openblack::debug::gui
{

/// A file browser drawn with the debug interface, for where the platform has no file dialog to show: it lists a folder's
/// folders and the files that match the picked filter, to step into, pick or name a file in
class FileBrowser
{
public:
	/// Opens the browser for the request, as the platform's dialog would be opened
	void Open(const file_dialog::Request& request);
	/// Draws the browser while it is open. Gives the path once one is chosen; cancelling closes it with none.
	[[nodiscard]] std::optional<std::filesystem::path> Draw();
	[[nodiscard]] bool IsOpen() const { return _open; }

private:
	void List();

	file_dialog::Request _request;
	bool _open {false};
	bool _popupShown {false};
	std::filesystem::path _folder;
	std::vector<std::filesystem::path> _folders;
	std::vector<std::filesystem::path> _files;
	size_t _filter {0};
	std::string _name;
	std::string _error;
};

} // namespace openblack::debug::gui
