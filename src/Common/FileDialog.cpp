/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FileDialog.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <fstream>
#include <ranges>
#include <sstream>
#include <system_error>

#include <SDL.h>

#ifdef _WIN32
#include <shobjidl.h>
#include <windows.h>
#endif
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// Elsewhere than Windows the dialogs are other programs run through the shell, which iOS does not allow
#if !defined(_WIN32) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define OPENBLACK_SHELL_FILE_DIALOGS 1
#else
#define OPENBLACK_SHELL_FILE_DIALOGS 0
#endif

namespace openblack::file_dialog
{
namespace
{
constexpr std::string_view k_PathsFile = "remembered_paths.txt";

std::string Utf8(const std::filesystem::path& path)
{
	const auto text = path.generic_u8string();
	return {text.begin(), text.end()};
}

std::filesystem::path FromUtf8(std::string_view text)
{
	return {std::u8string(text.begin(), text.end())};
}

/// The folder and name a save dialog suggests, or the folder alone
std::filesystem::path Suggested(const Request& request)
{
	if (request.mode == Mode::Save && !request.defaultName.empty())
	{
		return request.startFolder.empty() ? FromUtf8(request.defaultName)
		                                   : request.startFolder / FromUtf8(request.defaultName);
	}
	return request.startFolder;
}

std::string AppleScriptString(std::string_view text)
{
	std::string quoted = "\"";
	for (const auto c : text)
	{
		if (c == '"' || c == '\\')
		{
			quoted.push_back('\\');
		}
		quoted.push_back(c);
	}
	quoted.push_back('"');
	return quoted;
}

std::optional<std::filesystem::path> PreferencesFile()
{
	char* folder = SDL_GetPrefPath("openblack", "openblack");
	if (folder == nullptr)
	{
		return std::nullopt;
	}
	auto path = FromUtf8(folder) / k_PathsFile;
	SDL_free(folder);
	return path;
}

Paths ReadPaths()
{
	const auto file = PreferencesFile();
	if (!file.has_value())
	{
		return {};
	}
	std::ifstream stream(*file, std::ios::binary);
	std::stringstream text;
	text << stream.rdbuf();
	return ParsePaths(text.str());
}

#if OPENBLACK_SHELL_FILE_DIALOGS
bool HasProgram(std::string_view program)
{
	const auto command = "command -v " + ShellQuote(program) + " >/dev/null 2>&1";
	return std::system(command.c_str()) == 0;
}

/// Runs the program with the arguments and returns what it printed, unless it failed or was cancelled
Outcome Run(std::string_view program, const std::vector<std::string>& arguments)
{
	auto command = ShellQuote(program);
	for (const auto& argument : arguments)
	{
		command += ' ' + ShellQuote(argument);
	}
	command += " 2>/dev/null";
	FILE* pipe = popen(command.c_str(), "r");
	if (pipe == nullptr)
	{
		return {.status = Status::Unavailable};
	}
	std::string output;
	std::array<char, 1024> buffer {};
	while (const auto read = std::fread(buffer.data(), 1, buffer.size(), pipe))
	{
		output.append(buffer.data(), read);
	}
	const auto exitCode = pclose(pipe);
	auto path = PrintedPath(output);
	if (exitCode != 0 || path.empty())
	{
		return {.status = Status::Cancelled};
	}
	return {.status = Status::Chosen, .path = std::move(path)};
}
#endif

#ifdef _WIN32
std::wstring Widen(std::string_view text)
{
	if (text.empty())
	{
		return {};
	}
	const auto size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
	std::wstring wide(static_cast<size_t>(size), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
	return wide;
}

/// Releases a COM interface when it goes
template <typename T>
struct ComPtr
{
	T* pointer {nullptr};
	ComPtr() = default;
	ComPtr(const ComPtr&) = delete;
	ComPtr& operator=(const ComPtr&) = delete;
	~ComPtr()
	{
		if (pointer != nullptr)
		{
			pointer->Release();
		}
	}
	T* operator->() const { return pointer; }
};

/// Initialises COM for the thread for as long as it lives, if the thread hadn't already
struct ComApartment
{
	HRESULT result;
	ComApartment()
	    : result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))
	{
	}
	ComApartment(const ComApartment&) = delete;
	ComApartment& operator=(const ComApartment&) = delete;
	~ComApartment()
	{
		if (SUCCEEDED(result))
		{
			CoUninitialize();
		}
	}
	/// A thread already in the multithreaded apartment can still show the dialog
	[[nodiscard]] bool Usable() const { return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE; }
};

Outcome ShowWindows(const Request& request)
{
	const ComApartment apartment;
	if (!apartment.Usable())
	{
		return {.status = Status::Unavailable};
	}
	ComPtr<IFileDialog> dialog;
	const auto& classId = request.mode == Mode::Open ? CLSID_FileOpenDialog : CLSID_FileSaveDialog;
	const auto& interfaceId = request.mode == Mode::Open ? IID_IFileOpenDialog : IID_IFileSaveDialog;
	if (FAILED(
	        CoCreateInstance(classId, nullptr, CLSCTX_INPROC_SERVER, interfaceId, reinterpret_cast<void**>(&dialog.pointer))))
	{
		return {.status = Status::Unavailable};
	}

	FILEOPENDIALOGOPTIONS options = 0;
	dialog->GetOptions(&options);
	options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
	options |= request.mode == Mode::Open ? FOS_FILEMUSTEXIST : FOS_OVERWRITEPROMPT;
	dialog->SetOptions(options);
	dialog->SetTitle(Widen(request.title).c_str());

	// The dialog keeps pointers to the names and patterns until it is shown
	std::vector<std::pair<std::wstring, std::wstring>> texts;
	for (const auto& filter : request.filters)
	{
		texts.emplace_back(Widen(filter.name), Widen(JoinPatterns(filter, ";")));
	}
	std::vector<COMDLG_FILTERSPEC> specs;
	for (const auto& [name, patterns] : texts)
	{
		specs.push_back({.pszName = name.c_str(), .pszSpec = patterns.c_str()});
	}
	if (!specs.empty())
	{
		dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
		dialog->SetFileTypeIndex(1);
	}

	std::error_code error;
	if (!request.startFolder.empty() && std::filesystem::is_directory(request.startFolder, error))
	{
		ComPtr<IShellItem> folder;
		const auto absolute = std::filesystem::absolute(request.startFolder, error).make_preferred();
		if (SUCCEEDED(SHCreateItemFromParsingName(absolute.wstring().c_str(), nullptr, IID_IShellItem,
		                                          reinterpret_cast<void**>(&folder.pointer))))
		{
			dialog->SetFolder(folder.pointer);
		}
	}
	if (request.mode == Mode::Save && !request.defaultName.empty())
	{
		dialog->SetFileName(Widen(request.defaultName).c_str());
	}

	const auto shown = dialog->Show(static_cast<HWND>(request.owner));
	if (shown == HRESULT_FROM_WIN32(ERROR_CANCELLED))
	{
		return {.status = Status::Cancelled};
	}
	ComPtr<IShellItem> item;
	if (FAILED(shown) || FAILED(dialog->GetResult(&item.pointer)))
	{
		return {.status = Status::Unavailable};
	}
	PWSTR chosen = nullptr;
	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &chosen)) || chosen == nullptr)
	{
		return {.status = Status::Cancelled};
	}
	auto path = std::filesystem::path(chosen);
	CoTaskMemFree(chosen);
	return {.status = Status::Chosen, .path = std::move(path)};
}
#endif
} // namespace

Platform CurrentPlatform()
{
#if defined(_WIN32)
	return Platform::Windows;
#elif defined(__APPLE__)
	return Platform::MacOs;
#else
	return Platform::OtherUnix;
#endif
}

Backend ChooseBackend(Platform platform, bool hasZenity, bool hasKDialog, bool kdeDesktop)
{
	switch (platform)
	{
	case Platform::Windows:
		return Backend::Windows;
	case Platform::MacOs:
		return Backend::MacOs;
	case Platform::OtherUnix:
		if (hasKDialog && (kdeDesktop || !hasZenity))
		{
			return Backend::KDialog;
		}
		return hasZenity ? Backend::Zenity : Backend::Fallback;
	}
	return Backend::Fallback;
}

std::string JoinPatterns(const Filter& filter, std::string_view separator)
{
	std::string joined;
	for (const auto& pattern : filter.patterns)
	{
		if (!joined.empty())
		{
			joined += separator;
		}
		joined += pattern;
	}
	return joined;
}

std::vector<std::string> ZenityArguments(const Request& request)
{
	std::vector<std::string> arguments {"--file-selection", "--title=" + request.title};
	if (request.mode == Mode::Save)
	{
		arguments.emplace_back("--save");
		arguments.emplace_back("--confirm-overwrite");
	}
	const auto suggested = Suggested(request);
	if (!suggested.empty())
	{
		// A trailing separator tells zenity to open the folder rather than pick a file in it
		auto start = Utf8(suggested);
		if (request.mode == Mode::Open || request.defaultName.empty())
		{
			start += '/';
		}
		arguments.push_back("--filename=" + start);
	}
	for (const auto& filter : request.filters)
	{
		arguments.push_back("--file-filter=" + filter.name + " | " + JoinPatterns(filter, " "));
	}
	return arguments;
}

std::vector<std::string> KDialogArguments(const Request& request)
{
	std::vector<std::string> arguments {"--title", request.title,
	                                    request.mode == Mode::Open ? "--getopenfilename" : "--getsavefilename"};
	const auto suggested = Suggested(request);
	arguments.push_back(suggested.empty() ? std::string(".") : Utf8(suggested));
	std::string filters;
	for (const auto& filter : request.filters)
	{
		if (!filters.empty())
		{
			filters += '\n';
		}
		filters += JoinPatterns(filter, " ") + "|" + filter.name;
	}
	if (!filters.empty())
	{
		arguments.push_back(filters);
	}
	return arguments;
}

std::string AppleScript(const Request& request)
{
	std::string script =
	    request.mode == Mode::Open ? "POSIX path of (choose file with prompt " : "POSIX path of (choose file name with prompt ";
	script += AppleScriptString(request.title);
	if (request.mode == Mode::Save && !request.defaultName.empty())
	{
		script += " default name " + AppleScriptString(request.defaultName);
	}
	if (!request.startFolder.empty())
	{
		script += " default location (POSIX file " + AppleScriptString(Utf8(request.startFolder)) + ")";
	}
	script += ")";
	return script;
}

std::string ShellQuote(std::string_view argument)
{
	std::string quoted = "'";
	for (const auto c : argument)
	{
		if (c == '\'')
		{
			quoted += "'\\''";
		}
		else
		{
			quoted.push_back(c);
		}
	}
	quoted.push_back('\'');
	return quoted;
}

std::filesystem::path PrintedPath(std::string_view output)
{
	const auto end = output.find_first_of("\r\n");
	return FromUtf8(output.substr(0, end));
}

bool Matches(const Filter& filter, std::string_view fileName)
{
	const auto lower = [](std::string_view text) {
		std::string result(text);
		std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return result;
	};
	const auto name = lower(fileName);
	return std::ranges::any_of(filter.patterns, [&name, &lower](const std::string& pattern) {
		if (pattern == "*" || pattern == "*.*")
		{
			return true;
		}
		const auto wanted = lower(pattern);
		if (wanted.starts_with('*'))
		{
			return name.ends_with(std::string_view(wanted).substr(1));
		}
		return name == wanted;
	});
}

std::filesystem::path StartFolder(const std::optional<std::filesystem::path>& remembered, const std::filesystem::path& fallback,
                                  const std::function<bool(const std::filesystem::path&)>& exists)
{
	if (remembered.has_value() && !remembered->empty() && exists(*remembered))
	{
		return *remembered;
	}
	if (!fallback.empty() && exists(fallback))
	{
		return fallback;
	}
	return {};
}

Paths ParsePaths(std::string_view text)
{
	Paths paths;
	for (const auto line : text | std::views::split('\n'))
	{
		auto entry = std::string_view(line.begin(), line.end());
		if (entry.ends_with('\r'))
		{
			entry.remove_suffix(1);
		}
		const auto equals = entry.find('=');
		if (equals == std::string_view::npos || equals == 0 || equals + 1 == entry.size())
		{
			continue;
		}
		paths[std::string(entry.substr(0, equals))] = FromUtf8(entry.substr(equals + 1));
	}
	return paths;
}

std::string FormatPaths(const Paths& paths)
{
	std::string text;
	for (const auto& [name, path] : paths)
	{
		text += name + "=" + Utf8(path) + "\n";
	}
	return text;
}

std::optional<std::filesystem::path> RememberedPath(std::string_view name)
{
	const auto paths = ReadPaths();
	const auto found = paths.find(name);
	return found != paths.end() ? std::optional(found->second) : std::nullopt;
}

void RememberPath(std::string_view name, const std::filesystem::path& path)
{
	const auto file = PreferencesFile();
	if (!file.has_value() || name.empty() || name.find_first_of("=\n") != std::string_view::npos)
	{
		return;
	}
	auto paths = ReadPaths();
	paths[std::string(name)] = path;
	std::ofstream stream(*file, std::ios::binary | std::ios::trunc);
	stream << FormatPaths(paths);
}

Outcome Show(const Request& request)
{
#ifdef _WIN32
	return ShowWindows(request);
#elif !OPENBLACK_SHELL_FILE_DIALOGS
	static_cast<void>(request);
	return {.status = Status::Unavailable};
#else
	const auto* desktop = std::getenv("XDG_CURRENT_DESKTOP");
	const bool kde = desktop != nullptr && std::string_view(desktop).find("KDE") != std::string_view::npos;
	const auto platform = CurrentPlatform();
	const bool otherUnix = platform == Platform::OtherUnix;
	switch (ChooseBackend(platform, otherUnix && HasProgram("zenity"), otherUnix && HasProgram("kdialog"), kde))
	{
	case Backend::MacOs:
		return Run("osascript", {"-e", AppleScript(request)});
	case Backend::Zenity:
		return Run("zenity", ZenityArguments(request));
	case Backend::KDialog:
		return Run("kdialog", KDialogArguments(request));
	default:
		return {.status = Status::Unavailable};
	}
#endif
}

} // namespace openblack::file_dialog
