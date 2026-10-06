/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/// Tests on the game's compiled scripts (Scripts/Quests/challenge.chl under OPENBLACK_GAME_PATH):
///
/// - decompiling every script and compiling the result again must give back the same instructions;
/// - compiling the original script sources, when OPENBLACK_CHL_SOURCES points at them and OPENBLACK_CHL_HEADERS at the
///   script headers, must give back the game's file byte for byte.
///
/// Each skips when its data is missing.

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <ChlCompiler.h>
#include <ChlConstants.h>
#include <LHVMDecompiler.h>
#include <LHVMFile.h>
#include <gtest/gtest.h>

using namespace openblack::lhvm;
using namespace openblack::lhvm::chl;

namespace
{

std::optional<std::filesystem::path> ChallengePath()
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		return std::nullopt;
	}
	auto path = std::filesystem::path(gamePath) / "Scripts" / "Quests" / "challenge.chl";
	if (!std::filesystem::exists(path))
	{
		return std::nullopt;
	}
	return path;
}

std::string ReadText(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	std::ostringstream text;
	text << stream.rdbuf();
	return text.str();
}

/// The files the scripts came from, in program order, with the globals each declares and the scripts it starts
struct FileLayout
{
	std::string name;
	std::vector<std::string> globals;
	std::vector<std::string> autostart;
	std::vector<size_t> scripts;
};

std::vector<FileLayout> LayoutOf(const LHVMFile& program)
{
	std::vector<FileLayout> files;
	const auto& scripts = program.GetScripts();
	const auto& globals = program.GetVariablesNames();
	size_t declared = 0;
	for (size_t i = 0; i < scripts.size(); ++i)
	{
		if (files.empty() || files.back().name != scripts[i].filename)
		{
			auto& file = files.emplace_back();
			file.name = scripts[i].filename;
			// The globals declared before this file's scripts were compiled
			for (; declared < std::min<size_t>(scripts[i].variablesOffset, globals.size()); ++declared)
			{
				file.globals.push_back(globals[declared]);
			}
		}
		files.back().scripts.push_back(i);
	}
	if (declared < globals.size())
	{
		auto& file = files.emplace_back();
		file.name = "TrailingGlobals.txt";
		file.globals.assign(globals.begin() + static_cast<std::ptrdiff_t>(declared), globals.end());
	}
	for (const auto id : program.GetAutostart())
	{
		const auto script = std::ranges::find(scripts, id, &VMScript::scriptId);
		const auto file = std::ranges::find(files, script->filename, &FileLayout::name);
		file->autostart.push_back(script->name);
	}
	return files;
}

/// A script's instructions without line numbers, its jumps relative to its start
std::vector<std::array<uint32_t, 4>> Normalised(const LHVMFile& program, size_t index)
{
	const auto& scripts = program.GetScripts();
	const auto& code = program.GetInstructions();
	const auto start = scripts[index].instructionAddress;
	uint32_t end = static_cast<uint32_t>(code.size());
	for (const auto& script : scripts)
	{
		if (script.instructionAddress > start)
		{
			end = std::min(end, script.instructionAddress);
		}
	}
	std::vector<std::array<uint32_t, 4>> result;
	for (auto ip = start; ip < end; ++ip)
	{
		const auto& instruction = code[ip];
		auto data = instruction.data.uintVal;
		if (instruction.code == Opcode::Wait || instruction.code == Opcode::Jmp || instruction.code == Opcode::Except)
		{
			data -= start;
		}
		result.push_back({static_cast<uint32_t>(instruction.code), static_cast<uint32_t>(instruction.mode),
		                  static_cast<uint32_t>(instruction.type), data});
	}
	return result;
}

/// Number of scripts of `compiled` whose instructions equal those of the script of the same name in `original`
size_t IdenticalScripts(const LHVMFile& original, const LHVMFile& compiled, std::vector<std::string>& differing)
{
	size_t identical = 0;
	const auto& compiledScripts = compiled.GetScripts();
	for (size_t i = 0; i < original.GetScripts().size(); ++i)
	{
		const auto& name = original.GetScripts()[i].name;
		const auto it = std::ranges::find(compiledScripts, name, &VMScript::name);
		if (it != compiledScripts.end() &&
		    Normalised(original, i) == Normalised(compiled, static_cast<size_t>(std::distance(compiledScripts.begin(), it))))
		{
			++identical;
		}
		else
		{
			differing.push_back(name);
		}
	}
	return identical;
}

/// Decompiles the program, compiles it again and compares. Returns the share of instructions whose source line came
/// back the same.
double RoundTrip(const LHVMFile& original, const ConstantTable* constants, size_t expectedIdentical, bool sourceLines = false)
{
	DecompileOptions decompileOptions;
	decompileOptions.constants = constants;
	decompileOptions.sourceLines = sourceLines;
	const auto view = ProgramView::From(original);

	// The program as the files it was compiled from
	const auto decompiled = DecompileAll(view, decompileOptions);
	std::vector<SourceFile> sources;
	for (const auto& file : decompiled.files)
	{
		sources.push_back({.name = file.name, .text = decompiled.FileText(file)});
	}

	const auto compiled = Compile(sources, {.constants = constants});
	for (const auto& diagnostic : compiled.diagnostics)
	{
		ADD_FAILURE() << diagnostic.ToString();
	}
	if (!compiled.program.has_value())
	{
		ADD_FAILURE() << "The decompiled program doesn't compile";
		return 0.0;
	}
	EXPECT_EQ(compiled.program->GetVariablesNames(), original.GetVariablesNames());
	EXPECT_EQ(compiled.program->GetAutostart(), original.GetAutostart());
	EXPECT_EQ(compiled.program->GetInstructions().size(), original.GetInstructions().size());

	std::vector<std::string> differing;
	const auto identical = IdenticalScripts(original, *compiled.program, differing);
	std::printf("Round trip: %zu of %zu scripts compile back to identical instructions\n", identical,
	            original.GetScripts().size());
	for (const auto& name : differing)
	{
		std::printf("  differs: %s\n", name.c_str());
	}
	EXPECT_GE(identical, expectedIdentical);

	const auto& before = original.GetInstructions();
	const auto& after = compiled.program->GetInstructions();
	size_t sameLine = 0;
	for (size_t i = 0; i < std::min(before.size(), after.size()); ++i)
	{
		sameLine += before[i].line == after[i].line ? 1 : 0;
	}
	const auto share = before.empty() ? 0.0 : static_cast<double>(sameLine) / static_cast<double>(before.size());
	std::printf("Source lines: %zu of %zu instructions (%.1f%%) keep their line\n", sameLine, before.size(), 100.0 * share);
	return share;
}

/// Constants from a directory of script headers (.h) and info tables (.txt). A header the game ships itself (its
/// Data folder) is preferred to a copy of the same name.
ConstantTable LoadConstants(const std::filesystem::path& directory, const std::filesystem::path& gameData)
{
	ConstantTable constants;
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		files.push_back(entry.path());
	}
	std::ranges::sort(files);
	for (const auto& path : files)
	{
		const auto own = gameData / path.filename();
		if (path.extension() == ".h")
		{
			constants.LoadHeader(ReadText(std::filesystem::exists(own) ? own : path));
		}
		else if (path.extension() == ".txt")
		{
			constants.LoadInfo(ReadText(path));
		}
	}
	return constants;
}

} // namespace

TEST(ChlRoundTrip, ChallengeChlWithoutHeaders)
{
	const auto path = ChallengePath();
	if (!path)
	{
		GTEST_SKIP() << "OPENBLACK_GAME_PATH does not point at the game";
	}
	LHVMFile original;
	original.Open(*path);
	ASSERT_TRUE(original.IsLoaded());
	RoundTrip(original, nullptr, 514);
}

TEST(ChlRoundTrip, ChallengeChlWithHeaders)
{
	const auto path = ChallengePath();
	const char* headers = std::getenv("OPENBLACK_CHL_HEADERS");
	if (!path || headers == nullptr)
	{
		GTEST_SKIP() << "Needs OPENBLACK_GAME_PATH and OPENBLACK_CHL_HEADERS (a directory of script headers)";
	}
	LHVMFile original;
	original.Open(*path);
	ASSERT_TRUE(original.IsLoaded());
	const auto constants = LoadConstants(headers, path->parent_path().parent_path().parent_path() / "Data");
	RoundTrip(original, &constants, 514);
}

TEST(ChlRoundTrip, ChallengeChlOnItsSourceLines)
{
	const auto path = ChallengePath();
	const char* headers = std::getenv("OPENBLACK_CHL_HEADERS");
	if (!path || headers == nullptr)
	{
		GTEST_SKIP() << "Needs OPENBLACK_GAME_PATH and OPENBLACK_CHL_HEADERS (a directory of script headers)";
	}
	LHVMFile original;
	original.Open(*path);
	ASSERT_TRUE(original.IsLoaded());
	const auto constants = LoadConstants(headers, path->parent_path().parent_path().parent_path() / "Data");
	// Laid out on their recorded lines, most statements keep them: not all, as some sit closer together in the decompiled
	// text than in the original, and some statements spanned several lines there
	EXPECT_GE(RoundTrip(original, &constants, 514, true), 0.98);
}

TEST(ChlRoundTrip, OriginalSourcesCompileToTheGamesProgram)
{
	const auto path = ChallengePath();
	const char* sourcesPath = std::getenv("OPENBLACK_CHL_SOURCES");
	const char* headers = std::getenv("OPENBLACK_CHL_HEADERS");
	if (!path || sourcesPath == nullptr || headers == nullptr)
	{
		GTEST_SKIP() << "Needs OPENBLACK_GAME_PATH, OPENBLACK_CHL_SOURCES (the original quest sources) and "
		                "OPENBLACK_CHL_HEADERS (a directory of script headers)";
	}
	LHVMFile original;
	original.Open(*path);
	ASSERT_TRUE(original.IsLoaded());
	const auto constants = LoadConstants(headers, path->parent_path().parent_path().parent_path() / "Data");

	// Source files by lower-case name; one of the game's file names was cut short on disk, ending in '_'
	std::map<std::string, std::filesystem::path> onDisk;
	for (const auto& entry : std::filesystem::directory_iterator(sourcesPath))
	{
		auto name = entry.path().filename().string();
		std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::tolower(c); });
		onDisk[name] = entry.path();
	}
	const auto find = [&](std::string name) -> std::optional<std::filesystem::path> {
		std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::tolower(c); });
		if (const auto it = onDisk.find(name); it != onDisk.end())
		{
			return it->second;
		}
		for (const auto& [diskName, diskPath] : onDisk)
		{
			const auto stem = std::filesystem::path(diskName).stem().string();
			if (stem.ends_with('_') && name.starts_with(stem.substr(0, stem.size() - 1)))
			{
				return diskPath;
			}
		}
		return std::nullopt;
	};

	// The globals-only header file first, then the files in the order their scripts were compiled
	std::vector<SourceFile> sources;
	if (const auto headersFile = find("Headers.txt"))
	{
		sources.push_back({.name = "Headers.txt", .text = ReadText(*headersFile)});
	}
	for (const auto& layout : LayoutOf(original))
	{
		const auto file = find(layout.name);
		ASSERT_TRUE(file.has_value()) << "No source for " << layout.name;
		sources.push_back({.name = layout.name, .text = ReadText(*file)});
	}

	const auto compiled = Compile(sources, {.constants = &constants});
	for (const auto& diagnostic : compiled.diagnostics)
	{
		ADD_FAILURE() << diagnostic.ToString();
	}
	ASSERT_TRUE(compiled.program.has_value());
	std::ostringstream written(std::ios::binary);
	compiled.program->Write(written);
	const auto bytes = written.str();
	const auto expected = ReadText(*path);
	EXPECT_EQ(bytes.size(), expected.size());
	EXPECT_TRUE(bytes == expected) << "The compiled program differs from the game's";
}
