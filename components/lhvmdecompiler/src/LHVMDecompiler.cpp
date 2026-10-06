/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LHVMDecompiler.h"

#include <algorithm>

#include <LHVMFile.h>
#include <fmt/format.h>

#include "ChlWriter.h"
#include "Simplify.h"
#include "Structurer.h"

namespace openblack::lhvm
{

namespace
{

using namespace openblack::lhvm::chl;
using detail::Structurer;

constexpr int k_LabelPasses = 4;

[[nodiscard]] ScriptKind KindOf(ScriptType type)
{
	const auto bits = static_cast<uint32_t>(type);
	for (uint32_t i = 0; i < 7; ++i)
	{
		if ((bits & (1u << i)) != 0)
		{
			return static_cast<ScriptKind>(i);
		}
	}
	return ScriptKind::Script;
}

[[nodiscard]] bool IsControl(Opcode code)
{
	switch (code)
	{
	case Opcode::Jmp:
	case Opcode::Wait:
	case Opcode::Except:
	case Opcode::End:
	case Opcode::RetExcept:
	case Opcode::FailExcept:
	case Opcode::BrkExcept:
		return true;
	default:
		return false;
	}
}

/// The script's syntax tree, from the instructions [first, end)
Script BuildScript(Structurer& structurer, const ProgramView& program, const VMScript& vmScript, uint32_t first, uint32_t end)
{
	const auto& code = program.instructions;
	Script script;
	script.kind = KindOf(vmScript.type);
	script.name = vmScript.name;
	script.filename = vmScript.filename;
	for (uint32_t i = 0; i < vmScript.parameterCount && i < vmScript.variables.size(); ++i)
	{
		script.params.push_back(vmScript.variables[i]);
	}

	// A script is compiled as: install its exception handlers; take its parameters off the stack; set its locals; yield;
	// its body; end of exceptions; jump over the handlers; the handlers; end of handlers; end
	uint32_t bodyLo = first;
	uint32_t bodyHi = end;
	std::optional<Structurer::ExceptionLayout> layout;
	if (first < end && code[first].code == Opcode::Except)
	{
		layout = structurer.MatchException(first, end);
		if (!layout || !layout->explicitEnd || layout->after >= end || code[layout->after].code != Opcode::End)
		{
			layout.reset();
		}
	}
	if (layout)
	{
		script.beginIps.push_back(first);
		bodyLo = first + 1;
		bodyHi = layout->handler - 2;
	}
	else if (end > first && code[end - 1].code == Opcode::End)
	{
		bodyHi = end - 1;
	}

	uint32_t ip = bodyLo;
	for (uint32_t i = 0; i < vmScript.parameterCount; ++i)
	{
		if (ip < bodyHi && code[ip].code == Opcode::Pop && code[ip].mode == VMMode::Reference &&
		    code[ip].data.uintVal == vmScript.variablesOffset + 1 + i)
		{
			script.beginIps.push_back(ip++);
		}
		else
		{
			structurer.Diagnose(DiagnosticSeverity::Warning, ip, "Parameters aren't taken in the usual way");
			break;
		}
	}

	// Locals are set before the yield that ends the script's preamble
	std::optional<uint32_t> yield;
	for (uint32_t i = ip; i < bodyHi; ++i)
	{
		if (code[i].code == Opcode::EndExcept && code[i].mode == VMMode::Yield)
		{
			yield = i;
			break;
		}
		if (IsControl(code[i].code))
		{
			break;
		}
	}
	if (yield)
	{
		std::vector<uint32_t> trailing;
		script.locals = structurer.ParseSequence(ip, *yield, trailing);
		for (auto& stmt : script.locals)
		{
			if (stmt->kind == StmtKind::Assign && stmt->op == "=" &&
			    std::ranges::find(vmScript.variables, stmt->name) != vmScript.variables.end())
			{
				stmt->kind = StmtKind::Declaration;
			}
		}
		script.startIps = std::move(trailing);
		script.startIps.push_back(*yield);
		ip = *yield + 1;
	}

	std::vector<uint32_t> trailing;
	script.body = structurer.ParseSequence(ip, bodyHi, trailing);
	if (layout)
	{
		script.midIps = std::move(trailing);
		script.midIps.push_back(layout->handler - 2);
		script.midIps.push_back(layout->handler - 1);
		std::vector<uint32_t> handlerTrailing;
		script.handlers = structurer.ParseHandlers(layout->handler, layout->after - 1, layout->after, handlerTrailing);
		script.endIps = std::move(handlerTrailing);
		script.endIps.push_back(layout->after - 1);
		for (uint32_t i = layout->after; i < end; ++i)
		{
			script.endIps.push_back(i);
		}
	}
	else
	{
		script.endIps = std::move(trailing);
		for (uint32_t i = bodyHi; i < end; ++i)
		{
			script.endIps.push_back(i);
		}
	}
	return script;
}

} // namespace

ProgramView ProgramView::From(const LHVMFile& file)
{
	return {
	    .instructions = file.GetInstructions(),
	    .scripts = file.GetScripts(),
	    .globalNames = file.GetVariablesNames(),
	    .data = file.GetData(),
	    .autostart = file.GetAutostart(),
	};
}

std::vector<std::pair<uint32_t, uint32_t>> ScriptRanges(const ProgramView& program)
{
	const auto size = static_cast<uint32_t>(program.instructions.size());
	std::vector<uint32_t> starts;
	starts.reserve(program.scripts.size());
	for (const auto& script : program.scripts)
	{
		starts.push_back(std::min(script.instructionAddress, size));
	}
	std::vector<uint32_t> sorted = starts;
	std::ranges::sort(sorted);
	std::vector<std::pair<uint32_t, uint32_t>> ranges;
	ranges.reserve(starts.size());
	for (const auto start : starts)
	{
		const auto next = std::ranges::upper_bound(sorted, start);
		ranges.emplace_back(start, next == sorted.end() ? size : *next);
	}
	return ranges;
}

std::optional<size_t> DecompiledScript::LineForInstruction(uint32_t ip) const
{
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (std::ranges::find(lines[i], ip) != lines[i].end())
		{
			return i;
		}
	}
	return std::nullopt;
}

DecompiledScript DecompileScript(const ProgramView& program, size_t scriptIndex, const DecompileOptions& options)
{
	DecompiledScript result;
	result.scriptIndex = scriptIndex;
	if (scriptIndex >= program.scripts.size())
	{
		result.diagnostics.push_back({.severity = DiagnosticSeverity::Error, .ip = 0, .message = "No such script"});
		return result;
	}
	const auto& vmScript = program.scripts[scriptIndex];
	const auto natives = options.natives.empty() ? DefaultNativeSignatures() : options.natives;
	const auto [first, end] = ScriptRanges(program)[scriptIndex];
	result.name = vmScript.name;
	result.firstIp = first;
	result.endIp = end;

	// Gotos can target addresses already passed; parse again with those labels known until none are missing
	std::set<uint32_t> labels;
	std::optional<Structurer> structurer;
	Script script;
	for (int pass = 0; pass < k_LabelPasses; ++pass)
	{
		structurer.emplace(program, vmScript, natives, options, first, end, labels);
		script = BuildScript(*structurer, program, vmScript, first, end);
		const auto& wanted = structurer->WantedLabels();
		if (std::ranges::includes(structurer->EmittedLabels(), wanted))
		{
			break;
		}
		labels.insert(wanted.begin(), wanted.end());
	}
	for (const auto label : structurer->WantedLabels())
	{
		if (!structurer->EmittedLabels().contains(label))
		{
			structurer->Diagnose(DiagnosticSeverity::Error, label, "A goto target has no label");
		}
	}
	detail::Simplify(script.locals);
	detail::Simplify(script.body);
	detail::Simplify(script.handlers);

	result.diagnostics = std::move(structurer->Diagnostics());
	detail::ChlWriter writer(natives, options.constants, result.diagnostics);
	writer.WriteScript(script, options.challenge, options.standalone);
	result.leadingChallenge = writer.LeadingChallenge();
	result.challenge = writer.Challenge();

	for (const auto& line : writer.Lines())
	{
		for (int i = 0; i < line.indent; ++i)
		{
			result.text += options.indent;
		}
		result.text += line.text;
		result.text += '\n';
		auto ips = line.ips;
		std::ranges::sort(ips);
		const auto [dupFirst, dupLast] = std::ranges::unique(ips);
		ips.erase(dupFirst, dupLast);
		// The source line of the statement: where its own instructions were compiled. Jumps and exception
		// bookkeeping take the line current when they were patched, so they don't count.
		uint32_t sourceLine = 0;
		for (const auto ip : ips)
		{
			const auto& instruction = program.instructions[ip];
			switch (instruction.code)
			{
			case Opcode::Jmp:
			case Opcode::Wait:
			case Opcode::Except:
			case Opcode::EndExcept:
			case Opcode::FailExcept:
			case Opcode::BrkExcept:
			case Opcode::End:
				break;
			default:
				if (instruction.line != 0 && (sourceLine == 0 || instruction.line < sourceLine))
				{
					sourceLine = instruction.line;
				}
			}
		}
		result.sourceLines.push_back(sourceLine);
		result.lines.push_back(std::move(ips));
	}

	// Every instruction must be shown on some line
	std::vector<bool> covered(end - first, false);
	for (const auto& ips : result.lines)
	{
		for (const auto ip : ips)
		{
			if (ip >= first && ip < end)
			{
				covered[ip - first] = true;
			}
		}
	}
	for (uint32_t ip = first; ip < end; ++ip)
	{
		if (!covered[ip - first])
		{
			++result.unaccountedCount;
			result.diagnostics.push_back(
			    {.severity = DiagnosticSeverity::Error, .ip = ip, .message = "Instruction not shown on any line"});
		}
	}

	const auto graph = BuildControlFlowGraph(program.instructions, first, end);
	const auto unreachable = std::ranges::count_if(graph.blocks, [](const auto& block) { return !block.reachable; });
	if (unreachable > 0)
	{
		result.diagnostics.push_back({.severity = DiagnosticSeverity::Info,
		                              .ip = first,
		                              .message = fmt::format("{} blocks of code can never run", unreachable)});
	}

	result.gotoCount = writer.GotoCount();
	result.fallbackCount = writer.FallbackCount() + structurer->FallbackCount();
	result.nativeCallCount = writer.NativeCallCount();
	result.unsupportedCount = structurer->UnsupportedCount();
	result.ast = std::make_shared<const Script>(std::move(script));
	return result;
}

DecompiledProgram DecompileAll(const ProgramView& program, const DecompileOptions& options)
{
	DecompiledProgram result;
	result.sourceLines = options.sourceLines;
	// Files in program order: a new one starts where the scripts' recorded file name changes. Each declares the globals
	// added since the previous file's scripts were compiled (a script records how many there were).
	struct FileState
	{
		std::vector<std::string> globals;
		std::vector<std::string> autorun;
		std::optional<int32_t> challenge;
		std::string leadingChallenge;
	};
	std::vector<FileState> states;
	size_t declared = 0;
	for (size_t i = 0; i < program.scripts.size(); ++i)
	{
		const auto& vmScript = program.scripts[i];
		if (result.files.empty() || result.files.back().name != vmScript.filename)
		{
			result.files.push_back({.name = vmScript.filename, .header = {}, .scripts = {}});
			auto& state = states.emplace_back();
			for (; declared < std::min<size_t>(vmScript.variablesOffset, program.globalNames.size()); ++declared)
			{
				state.globals.push_back(program.globalNames[declared]);
			}
		}
		auto scriptOptions = options;
		scriptOptions.challenge = states.back().challenge;
		scriptOptions.standalone = false;
		auto script = DecompileScript(program, i, scriptOptions);
		auto& state = states.back();
		state.challenge = script.challenge;
		if (state.leadingChallenge.empty())
		{
			state.leadingChallenge = script.leadingChallenge;
		}
		result.files.back().scripts.push_back(i);
		auto& stats = result.stats;
		++stats.scripts;
		stats.instructions += script.endIp - script.firstIp;
		stats.unaccountedInstructions += script.unaccountedCount;
		stats.unsupportedOpcodes += script.unsupportedCount;
		stats.nativeCalls += script.nativeCallCount;
		stats.gotos += script.gotoCount;
		stats.clean += script.IsClean() ? 1 : 0;
		stats.withGoto += script.gotoCount > 0 ? 1 : 0;
		stats.withFallback += script.fallbackCount > 0 ? 1 : 0;
		result.scripts.push_back(std::move(script));
	}
	if (declared < program.globalNames.size())
	{
		// Globals no script was compiled after
		result.files.push_back({.name = "Globals.txt", .header = {}, .scripts = {}});
		auto& state = states.emplace_back();
		state.globals.assign(program.globalNames.begin() + static_cast<std::ptrdiff_t>(declared), program.globalNames.end());
	}
	// Scripts run at load are named in the file that defines them
	for (const auto id : program.autostart)
	{
		for (size_t f = 0; f < result.files.size(); ++f)
		{
			if (std::ranges::find(result.files[f].scripts, static_cast<size_t>(id) - 1) != result.files[f].scripts.end())
			{
				states[f].autorun.push_back(program.scripts[id - 1].name);
			}
		}
	}
	for (size_t f = 0; f < result.files.size(); ++f)
	{
		auto& header = result.files[f].header;
		const auto& state = states[f];
		if (!state.leadingChallenge.empty())
		{
			header += "challenge " + state.leadingChallenge + "\n";
		}
		for (const auto& global : state.globals)
		{
			header += "global " + global + "\n";
		}
		for (const auto& name : state.autorun)
		{
			header += "run script " + name + "\n";
		}
	}
	return result;
}

std::string DecompiledProgram::FileText(const DecompiledFile& file) const
{
	std::string text = file.header;
	auto line = static_cast<uint32_t>(std::ranges::count(text, '\n'));
	for (const auto index : file.scripts)
	{
		const auto& script = scripts[index];
		if (!text.empty())
		{
			text += '\n';
			++line;
		}
		if (!sourceLines)
		{
			text += script.text;
			continue;
		}
		// Blank lines bring each statement down to its recorded line; one recorded earlier stays where it falls
		size_t start = 0;
		for (size_t i = 0; start < script.text.size(); ++i)
		{
			const auto target = i < script.sourceLines.size() ? script.sourceLines[i] : 0;
			while (target > line + 1)
			{
				text += '\n';
				++line;
			}
			const auto end = script.text.find('\n', start);
			text += script.text.substr(start, end - start + 1);
			++line;
			start = end + 1;
		}
	}
	return text;
}

std::string DecompiledProgram::Text() const
{
	std::string text;
	for (const auto& file : files)
	{
		if (!text.empty())
		{
			text += '\n';
		}
		text += FileText(file);
	}
	return text;
}

} // namespace openblack::lhvm
