/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "lhvmtool_compile.h"

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <ChlCompiler.h>
#include <ChlConstants.h>
#include <cxxopts.hpp>

namespace
{

std::optional<std::string> ReadText(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	if (!stream)
	{
		return std::nullopt;
	}
	std::ostringstream text;
	text << stream.rdbuf();
	return text.str();
}

/// File names listed one per line; blank lines and // comments are skipped
std::vector<std::string> ReadList(const std::string& text)
{
	std::vector<std::string> names;
	std::istringstream lines(text);
	std::string line;
	while (std::getline(lines, line))
	{
		const auto comment = line.find("//");
		if (comment != std::string::npos)
		{
			line.resize(comment);
		}
		std::erase_if(line, [](char c) { return c == '\r'; });
		const auto first = line.find_first_not_of(" \t");
		if (first == std::string::npos)
		{
			continue;
		}
		const auto last = line.find_last_not_of(" \t");
		names.push_back(line.substr(first, last - first + 1));
	}
	return names;
}

} // namespace

int RunCompile(int argc, char** argv)
{
	cxxopts::Options options("lhvmtool compile", "Compile Challenge Language sources into an LHVM program (.chl).");
	options.add_options()                                                                                //
	    ("h,help", "Display this help message.")                                                         //
	    ("o,output", "Program to write.", cxxopts::value<std::string>()->default_value("challenge.chl")) //
	    ("headers", "Directory of script headers (.h) and info tables (.txt) to read constants from.",
	     cxxopts::value<std::vector<std::string>>())("header", "Script header file to read constants from.",
	                                                 cxxopts::value<std::vector<std::string>>())               //
	    ("info", "Data table (info file) to read constants from.", cxxopts::value<std::vector<std::string>>()) //
	    ("l,list", "File listing the sources to compile, in order, one per line.",                             //
	     cxxopts::value<std::string>())                                                                        //
	    ("sources", "Source files, or directories of .txt sources compiled in name order.",                    //
	     cxxopts::value<std::vector<std::string>>())                                                           //
	    ;
	options.positional_help("[OPTION...] sources...");
	options.parse_positional({"sources"});
	auto result = options.parse(argc, argv);
	if (result["help"].as<bool>())
	{
		std::cout << options.help() << '\n';
		return EXIT_SUCCESS;
	}

	openblack::lhvm::chl::ConstantTable constants;
	std::vector<std::filesystem::path> headers;
	std::vector<std::filesystem::path> infos;
	if (result["headers"].count() > 0)
	{
		for (const auto& directory : result["headers"].as<std::vector<std::string>>())
		{
			std::error_code ec;
			for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
			{
				if (entry.path().extension() == ".h")
				{
					headers.push_back(entry.path());
				}
				else if (entry.path().extension() == ".txt")
				{
					infos.push_back(entry.path());
				}
			}
		}
		std::ranges::sort(headers);
		std::ranges::sort(infos);
	}
	if (result["header"].count() > 0)
	{
		for (const auto& header : result["header"].as<std::vector<std::string>>())
		{
			headers.emplace_back(header);
		}
	}
	if (result["info"].count() > 0)
	{
		for (const auto& info : result["info"].as<std::vector<std::string>>())
		{
			infos.emplace_back(info);
		}
	}
	for (const auto& header : headers)
	{
		const auto text = ReadText(header);
		if (!text)
		{
			std::cerr << "Cannot read " << header.string() << '\n';
			return EXIT_FAILURE;
		}
		constants.LoadHeader(*text);
	}
	for (const auto& info : infos)
	{
		const auto text = ReadText(info);
		if (!text)
		{
			std::cerr << "Cannot read " << info.string() << '\n';
			return EXIT_FAILURE;
		}
		constants.LoadInfo(*text);
	}

	std::vector<std::filesystem::path> paths;
	if (result["list"].count() > 0)
	{
		const std::filesystem::path listPath = result["list"].as<std::string>();
		const auto text = ReadText(listPath);
		if (!text)
		{
			std::cerr << "Cannot read " << listPath.string() << '\n';
			return EXIT_FAILURE;
		}
		for (const auto& name : ReadList(*text))
		{
			paths.push_back(listPath.parent_path() / name);
		}
	}
	if (result["sources"].count() > 0)
	{
		for (const auto& source : result["sources"].as<std::vector<std::string>>())
		{
			if (std::filesystem::is_directory(source))
			{
				std::vector<std::filesystem::path> files;
				for (const auto& entry : std::filesystem::directory_iterator(source))
				{
					if (entry.path().extension() == ".txt")
					{
						files.push_back(entry.path());
					}
				}
				std::ranges::sort(files);
				paths.insert(paths.end(), files.begin(), files.end());
			}
			else
			{
				paths.emplace_back(source);
			}
		}
	}
	if (paths.empty())
	{
		std::cerr << options.help() << '\n';
		return EXIT_FAILURE;
	}

	std::vector<openblack::lhvm::chl::SourceFile> sources;
	for (const auto& path : paths)
	{
		const auto text = ReadText(path);
		if (!text)
		{
			std::cerr << "Cannot read " << path.string() << '\n';
			return EXIT_FAILURE;
		}
		sources.push_back({.name = path.filename().string(), .text = *text});
	}

	const auto compiled = openblack::lhvm::chl::Compile(sources, {.natives = {}, .constants = &constants});
	for (const auto& diagnostic : compiled.diagnostics)
	{
		std::cerr << diagnostic.ToString() << '\n';
	}
	if (!compiled.program)
	{
		return EXIT_FAILURE;
	}
	const std::filesystem::path output = result["output"].as<std::string>();
	if (!compiled.program->Write(output))
	{
		std::cerr << "Cannot write " << output.string() << '\n';
		return EXIT_FAILURE;
	}
	std::printf("Compiled %zu files: %zu scripts, %zu instructions, %zu globals into %s\n", sources.size(),
	            compiled.program->GetScripts().size(), compiled.program->GetInstructions().size(),
	            compiled.program->GetVariablesNames().size(), output.string().c_str());
	return EXIT_SUCCESS;
}
