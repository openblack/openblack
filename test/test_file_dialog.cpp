/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Common/FileDialog.h"

using namespace openblack::file_dialog;

namespace
{
const Filter k_All {.name = "All files", .patterns = {"*"}};
const Filter k_Saved {.name = "Saved creatures", .patterns = {"*.erc", "*.ERC2"}};

Request OpenRequest()
{
	return {
	    .mode = Mode::Open,
	    .title = "Open a mind",
	    .filters = {k_All, k_Saved},
	    .startFolder = "/games/bw/Scripts/CreatureMind",
	};
}

bool Contains(const std::vector<std::string>& arguments, const std::string& wanted)
{
	return std::ranges::find(arguments, wanted) != arguments.end();
}
} // namespace

TEST(FileDialog, BackendFollowsThePlatform)
{
	EXPECT_EQ(ChooseBackend(Platform::Windows, false, false, false), Backend::Windows);
	EXPECT_EQ(ChooseBackend(Platform::MacOs, true, true, true), Backend::MacOs);
}

TEST(FileDialog, UnixPrefersTheDesktopsOwnHelper)
{
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, true, true, false), Backend::Zenity);
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, true, true, true), Backend::KDialog);
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, true, false, true), Backend::Zenity);
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, false, true, false), Backend::KDialog);
}

TEST(FileDialog, UnixWithoutHelpersFallsBack)
{
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, false, false, false), Backend::Fallback);
	EXPECT_EQ(ChooseBackend(Platform::OtherUnix, false, false, true), Backend::Fallback);
}

TEST(FileDialog, JoinsPatterns)
{
	EXPECT_EQ(JoinPatterns(k_Saved, ";"), "*.erc;*.ERC2");
	EXPECT_EQ(JoinPatterns(k_Saved, " "), "*.erc *.ERC2");
	EXPECT_EQ(JoinPatterns(Filter {.name = "None"}, ";"), "");
}

TEST(FileDialog, ZenityOpensInTheStartFolder)
{
	const auto arguments = ZenityArguments(OpenRequest());
	EXPECT_EQ(arguments.front(), "--file-selection");
	EXPECT_TRUE(Contains(arguments, "--title=Open a mind"));
	EXPECT_TRUE(Contains(arguments, "--filename=/games/bw/Scripts/CreatureMind/"));
	EXPECT_TRUE(Contains(arguments, "--file-filter=All files | *"));
	EXPECT_TRUE(Contains(arguments, "--file-filter=Saved creatures | *.erc *.ERC2"));
	EXPECT_FALSE(Contains(arguments, "--save"));
}

TEST(FileDialog, ZenitySaveSuggestsTheName)
{
	auto request = OpenRequest();
	request.mode = Mode::Save;
	request.defaultName = "mind";
	const auto arguments = ZenityArguments(request);
	EXPECT_TRUE(Contains(arguments, "--save"));
	EXPECT_TRUE(Contains(arguments, "--confirm-overwrite"));
	EXPECT_TRUE(Contains(arguments, "--filename=/games/bw/Scripts/CreatureMind/mind"));
}

TEST(FileDialog, KDialogArguments)
{
	const auto open = KDialogArguments(OpenRequest());
	ASSERT_EQ(open.size(), 5u);
	EXPECT_EQ(open[0], "--title");
	EXPECT_EQ(open[1], "Open a mind");
	EXPECT_EQ(open[2], "--getopenfilename");
	EXPECT_EQ(open[3], "/games/bw/Scripts/CreatureMind");
	EXPECT_EQ(open[4], "*|All files\n*.erc *.ERC2|Saved creatures");

	auto request = OpenRequest();
	request.mode = Mode::Save;
	request.defaultName = "mind";
	request.filters.clear();
	request.startFolder.clear();
	const auto save = KDialogArguments(request);
	ASSERT_EQ(save.size(), 4u);
	EXPECT_EQ(save[2], "--getsavefilename");
	EXPECT_EQ(save[3], "mind");
}

TEST(FileDialog, AppleScriptEscapesText)
{
	auto request = OpenRequest();
	request.title = "Pick \"one\"";
	EXPECT_EQ(AppleScript(request), "POSIX path of (choose file with prompt \"Pick \\\"one\\\"\" default location (POSIX file "
	                                "\"/games/bw/Scripts/CreatureMind\"))");
	request.mode = Mode::Save;
	request.defaultName = "mind";
	request.startFolder.clear();
	EXPECT_EQ(AppleScript(request), "POSIX path of (choose file name with prompt \"Pick \\\"one\\\"\" default name \"mind\")");
}

TEST(FileDialog, ShellQuoting)
{
	EXPECT_EQ(ShellQuote("plain"), "'plain'");
	EXPECT_EQ(ShellQuote("it's $HOME"), "'it'\\''s $HOME'");
	EXPECT_EQ(ShellQuote(""), "''");
}

TEST(FileDialog, PrintedPathIsTheFirstLine)
{
	EXPECT_EQ(PrintedPath("/home/me/mind\n"), std::filesystem::path("/home/me/mind"));
	EXPECT_EQ(PrintedPath("/home/me/mind\r\nmore"), std::filesystem::path("/home/me/mind"));
	EXPECT_TRUE(PrintedPath("").empty());
	EXPECT_TRUE(PrintedPath("\n").empty());
}

TEST(FileDialog, FiltersMatchNames)
{
	EXPECT_TRUE(Matches(k_All, "KhazarCreature"));
	EXPECT_TRUE(Matches(k_Saved, "C4a4f6e63.erc"));
	EXPECT_TRUE(Matches(k_Saved, "SHOUTING.ERC"));
	EXPECT_TRUE(Matches(k_Saved, "x.erc2"));
	EXPECT_FALSE(Matches(k_Saved, "KhazarCreature"));
	EXPECT_FALSE(Matches(k_Saved, "erc"));
	EXPECT_TRUE(Matches(Filter {.name = "One", .patterns = {"Grio"}}, "grio"));
	EXPECT_FALSE(Matches(Filter {.name = "None"}, "anything"));
}

TEST(FileDialog, StartFolderPrefersTheRememberedOne)
{
	const std::set<std::filesystem::path> existing {"/remembered", "/game"};
	const auto exists = [&existing](const std::filesystem::path& folder) { return existing.contains(folder); };
	EXPECT_EQ(StartFolder(std::filesystem::path("/remembered"), "/game", exists), std::filesystem::path("/remembered"));
	EXPECT_EQ(StartFolder(std::filesystem::path("/gone"), "/game", exists), std::filesystem::path("/game"));
	EXPECT_EQ(StartFolder(std::nullopt, "/game", exists), std::filesystem::path("/game"));
	EXPECT_TRUE(StartFolder(std::nullopt, "/missing", exists).empty());
	EXPECT_TRUE(StartFolder(std::filesystem::path(), "", exists).empty());
}

TEST(FileDialog, RememberedPathsRoundTrip)
{
	const Paths paths {{"creature-mind-folder", std::filesystem::path(u8"/home/m\u00e9/minds")},
	                   {"creature-mind-file", "C:/games/bw/Scripts/CreatureMind/KhazarCreature"}};
	const auto text = FormatPaths(paths);
	EXPECT_EQ(ParsePaths(text), paths);
}

TEST(FileDialog, ParsingSkipsBrokenLines)
{
	const auto paths = ParsePaths("good=/a\r\nno equals\n=/nameless\nempty=\nlast=/b=c");
	ASSERT_EQ(paths.size(), 2u);
	EXPECT_EQ(paths.at("good"), std::filesystem::path("/a"));
	EXPECT_EQ(paths.at("last"), std::filesystem::path("/b=c"));
}
