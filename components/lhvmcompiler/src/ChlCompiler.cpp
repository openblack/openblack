/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlCompiler.h"

#include <algorithm>

#include "ChlParser.h"
#include "CodeGenerator.h"

namespace openblack::lhvm::chl
{

namespace
{

std::span<const NativeSignature> NativesOf(const CompileOptions& options)
{
	return options.natives.empty() ? DefaultNativeSignatures() : options.natives;
}

} // namespace

CompileResult Compile(std::span<const SourceFile> sources, const CompileOptions& options)
{
	DiagnosticSink sink;
	ParseEnvironment environment(NativesOf(options), options.constants);
	std::vector<ParsedFile> files;
	files.reserve(sources.size());
	for (const auto& source : sources)
	{
		files.push_back(ParseFile(source.text, source.name, environment, sink));
	}

	CompileResult result;
	if (sink.ErrorCount() > 0)
	{
		result.diagnostics = sink.Take();
		return result;
	}
	CodeGenerator generator(environment, sink, options.firstScriptId);
	for (const auto& file : files)
	{
		generator.Add(file);
	}
	generator.Finish();

	if (sink.ErrorCount() == 0)
	{
		result.program.emplace(LHVMVersion::BlackAndWhite, environment.Globals(), generator.Instructions(),
		                       generator.Autostart(), generator.Scripts(), generator.Data());
	}
	result.diagnostics = sink.Take();
	return result;
}

CompileResult CompileScript(const LHVMFile& program, const SourceFile& source, const CompileOptions& options)
{
	DiagnosticSink sink;
	ParseEnvironment environment(NativesOf(options), options.constants);
	for (const auto& name : program.GetVariablesNames())
	{
		environment.AddGlobal(name);
	}
	const auto globalCount = environment.Globals().size();
	auto file = ParseFile(source.text, source.name, environment, sink);
	if (sink.ErrorCount() > 0)
	{
		return {.program = std::nullopt, .diagnostics = sink.Take()};
	}

	CodeGenerator generator(environment, sink, options.firstScriptId);
	generator.Seed(program.GetInstructions(), program.GetScripts(), program.GetData(), program.GetAutostart(), globalCount);
	const auto oldCount = program.GetScripts().size();
	generator.Add(file);
	generator.Finish();

	// A recompiled script takes the place (and id) of the old one of the same name
	auto& scripts = generator.Scripts();
	std::vector<VMScript> merged(scripts.begin(), scripts.begin() + static_cast<std::ptrdiff_t>(oldCount));
	for (size_t i = oldCount; i < scripts.size(); ++i)
	{
		auto script = scripts[i];
		const auto it = std::ranges::find(merged, script.name, &VMScript::name);
		if (it != merged.end())
		{
			const auto oldId = it->scriptId;
			const auto newId = script.scriptId;
			script.scriptId = oldId;
			*it = script;
			// Calls compiled against the new id point at the old one
			for (auto& instruction : generator.Instructions())
			{
				if (instruction.code == Opcode::Run && instruction.data.uintVal == newId)
				{
					instruction.data = VMValue(oldId);
				}
			}
		}
		else
		{
			merged.push_back(std::move(script));
		}
	}

	CompileResult result;
	if (sink.ErrorCount() == 0)
	{
		result.program.emplace(program.GetVersion(), environment.Globals(), generator.Instructions(), generator.Autostart(),
		                       merged, generator.Data());
	}
	result.diagnostics = sink.Take();
	return result;
}

} // namespace openblack::lhvm::chl
