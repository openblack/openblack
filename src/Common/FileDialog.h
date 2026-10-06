/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// Asks the player for a file to open or a place to save one, with the platform's own dialog: the shell's file dialog
/// on Windows, zenity or kdialog on Linux and other Unix desktops, and AppleScript's chooser on macOS. Where none of
/// those is available the caller is told so and can show a file browser of its own. The dialog is modal: it returns
/// once the player has chosen or cancelled, and the game simply waits for it.
namespace openblack::file_dialog
{

/// A kind of file the dialog offers, as a name and the patterns its files match, e.g. "*.erc" or "*" for any file
struct Filter
{
	std::string name;
	std::vector<std::string> patterns;
};

enum class Mode : uint8_t
{
	Open,
	Save,
};

struct Request
{
	Mode mode {Mode::Open};
	std::string title;
	/// The first is picked to begin with
	std::vector<Filter> filters;
	/// Where the dialog starts, if it exists
	std::filesystem::path startFolder;
	/// The file name a save dialog suggests
	std::string defaultName;
	/// The platform's handle of the window the dialog belongs to (a HWND on Windows), or null
	void* owner {nullptr};
};

enum class Status : uint8_t
{
	Chosen,
	Cancelled,
	/// No dialog could be shown on this platform: the caller can show its own browser
	Unavailable,
};

struct Outcome
{
	Status status {Status::Unavailable};
	std::filesystem::path path;
};

/// What can show a dialog
enum class Backend : uint8_t
{
	Windows,
	MacOs,
	Zenity,
	KDialog,
	/// None of the platform's: the caller's own browser
	Fallback,
};

enum class Platform : uint8_t
{
	Windows,
	MacOs,
	OtherUnix,
};

[[nodiscard]] Platform CurrentPlatform();

/// The dialog to use on a platform, given which helper programs can be found. On Linux zenity is preferred under GNOME
/// and other desktops, kdialog under KDE, either if only one of them is there.
[[nodiscard]] Backend ChooseBackend(Platform platform, bool hasZenity, bool hasKDialog, bool kdeDesktop);

/// The patterns joined by the separator, "*.erc;*.chl"
[[nodiscard]] std::string JoinPatterns(const Filter& filter, std::string_view separator);

/// zenity's and kdialog's arguments for the request, without the program's own name
[[nodiscard]] std::vector<std::string> ZenityArguments(const Request& request);
[[nodiscard]] std::vector<std::string> KDialogArguments(const Request& request);
/// The AppleScript that asks for the file and prints its POSIX path
[[nodiscard]] std::string AppleScript(const Request& request);

/// A command line argument quoted for a POSIX shell
[[nodiscard]] std::string ShellQuote(std::string_view argument);

/// The path a helper program printed: its first line, without the line ending. Empty if it printed nothing.
[[nodiscard]] std::filesystem::path PrintedPath(std::string_view output);

/// Whether a file's name matches one of the filter's patterns ("*" matches all, "*.erc" any name ending in .erc,
/// regardless of case, and a pattern without a star the name itself)
[[nodiscard]] bool Matches(const Filter& filter, std::string_view fileName);

/// The folder the dialog starts in: the remembered one if it still exists, else the fallback if it exists, else none
[[nodiscard]] std::filesystem::path StartFolder(const std::optional<std::filesystem::path>& remembered,
                                                const std::filesystem::path& fallback,
                                                const std::function<bool(const std::filesystem::path&)>& exists);

/// Remembered folders and files by name, kept as "name=path" lines
using Paths = std::map<std::string, std::filesystem::path, std::less<>>;
[[nodiscard]] Paths ParsePaths(std::string_view text);
[[nodiscard]] std::string FormatPaths(const Paths& folders);

/// A folder or file remembered under the name, such as the folder a file was last chosen from, kept for this user of the
/// computer from one run to the next (in the user's preferences folder, not with the game)
[[nodiscard]] std::optional<std::filesystem::path> RememberedPath(std::string_view name);
void RememberPath(std::string_view name, const std::filesystem::path& path);

/// Shows the platform's dialog. Blocks until the player closes it.
[[nodiscard]] Outcome Show(const Request& request);

} // namespace openblack::file_dialog
