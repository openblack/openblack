/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScriptModel.h"

#include <cctype>
#include <charconv>

#include <algorithm>
#include <map>

#include <ChlSyntax.h>
#include <fmt/format.h>

#include "Editor/EditorOutline.h"

namespace openblack::editor::scripts
{

using lhvm::DataType;
using lhvm::Opcode;
using lhvm::VMInstruction;
using lhvm::VMMode;
using lhvm::VMScript;

Category CategoryOf(lhvm::ScriptType type)
{
	switch (type)
	{
	case lhvm::ScriptType::Script:
		return Category::Script;
	case lhvm::ScriptType::Help:
		return Category::Help;
	case lhvm::ScriptType::ChallengeHelp:
		return Category::ChallengeHelp;
	case lhvm::ScriptType::TempleHelp:
		return Category::TempleHelp;
	case lhvm::ScriptType::TempleSpecial:
		return Category::TempleSpecial;
	case lhvm::ScriptType::MultiplayerHelp:
		return Category::MultiplayerHelp;
	default:
		return Category::Other;
	}
}

std::string_view Name(Category category)
{
	constexpr std::array<std::string_view, k_CategoryCount> k_Names {
	    "Script", "Help", "Challenge help", "Temple help", "Temple special", "Multiplayer help", "Other",
	};
	return k_Names.at(static_cast<size_t>(category));
}

bool Matches(const VMScript& script, const ScriptFilter& filter)
{
	if (!filter.categories.at(static_cast<size_t>(CategoryOf(script.type))))
	{
		return false;
	}
	return MatchesSearch(script.name + " " + script.filename, filter.text);
}

CodeRange RangeOf(std::span<const VMInstruction> code, const VMScript& script)
{
	const auto begin = std::min<uint32_t>(script.instructionAddress, static_cast<uint32_t>(code.size()));
	auto end = begin;
	while (end < code.size())
	{
		const auto isEnd = code[end].code == Opcode::End;
		++end;
		if (isEnd)
		{
			break;
		}
	}
	return {.begin = begin, .end = end};
}

const VMScript* ScriptAt(std::span<const VMInstruction> code, std::span<const VMScript> scripts, uint32_t address)
{
	const VMScript* best = nullptr;
	for (const auto& script : scripts)
	{
		if (script.instructionAddress <= address && (best == nullptr || script.instructionAddress > best->instructionAddress))
		{
			best = &script;
		}
	}
	if (best != nullptr && !RangeOf(code, *best).Contains(address))
	{
		return nullptr;
	}
	return best;
}

std::optional<uint32_t> JumpTarget(const VMInstruction& instruction)
{
	switch (instruction.code)
	{
	// The conditional jump is the opcode named Wait
	case Opcode::Wait:
	case Opcode::Jmp:
	case Opcode::Except:
		return instruction.data.uintVal;
	default:
		return std::nullopt;
	}
}

std::string VariableName(const Program& program, const VMScript* script, uint32_t id, bool& local)
{
	local = script != nullptr && id > script->variablesOffset;
	if (local)
	{
		const auto index = id - script->variablesOffset - 1;
		return index < script->variables.size() ? script->variables.at(index) : fmt::format("local_{}", index);
	}
	return id < program.globals.size() ? program.globals[id].name : fmt::format("global_{}", id);
}

std::optional<std::string_view> StringAt(std::span<const char> data, int32_t offset)
{
	if (offset < 0 || static_cast<size_t>(offset) >= data.size())
	{
		return std::nullopt;
	}
	const auto start = static_cast<size_t>(offset);
	if (start > 0 && data[start - 1] != '\0')
	{
		return std::nullopt;
	}
	size_t end = start;
	while (end < data.size() && data[end] != '\0')
	{
		if (std::isprint(static_cast<unsigned char>(data[end])) == 0)
		{
			return std::nullopt;
		}
		++end;
	}
	if (end >= data.size() || end - start < 2)
	{
		return std::nullopt;
	}
	return std::string_view(data.data() + start, end - start);
}

std::string_view TypeName(DataType type)
{
	switch (type)
	{
	case DataType::None:
		return "none";
	case DataType::Int:
		return "int";
	case DataType::Float:
		return "float";
	case DataType::Vector:
		return "vector";
	case DataType::Object:
		return "object";
	case DataType::Boolean:
		return "bool";
	default:
		return "unknown";
	}
}

std::string FormatValue(lhvm::VMValue value, DataType type)
{
	switch (type)
	{
	case DataType::Int:
		return std::to_string(value.intVal);
	case DataType::Float:
	case DataType::Vector:
		return fmt::format("{:g}", value.floatVal);
	case DataType::Boolean:
		return value.intVal != 0 ? "true" : "false";
	case DataType::Object:
		return fmt::format("object {}", value.uintVal);
	default:
		return fmt::format("{} ({})", value.intVal, TypeName(type));
	}
}

std::optional<lhvm::VMValue> ParseValue(std::string_view text, DataType type)
{
	while (!text.empty() && text.front() == ' ')
	{
		text.remove_prefix(1);
	}
	while (!text.empty() && text.back() == ' ')
	{
		text.remove_suffix(1);
	}
	if (text.empty())
	{
		return std::nullopt;
	}
	const auto* const first = text.data();
	const auto* const last = text.data() + text.size();
	switch (type)
	{
	case DataType::Float:
	case DataType::Vector:
	{
		const auto value = lhvm::chl::ParseFloat(text);
		return value.has_value() ? std::optional(lhvm::VMValue(*value)) : std::nullopt;
	}
	case DataType::Boolean:
		if (text == "true" || text == "1")
		{
			return lhvm::VMValue(int32_t {1});
		}
		if (text == "false" || text == "0")
		{
			return lhvm::VMValue(int32_t {0});
		}
		return std::nullopt;
	case DataType::Object:
	{
		uint32_t value = 0;
		const auto [end, error] = std::from_chars(first, last, value);
		return error == std::errc {} && end == last ? std::optional(lhvm::VMValue(value)) : std::nullopt;
	}
	default:
	{
		int32_t value = 0;
		const auto [end, error] = std::from_chars(first, last, value);
		return error == std::errc {} && end == last ? std::optional(lhvm::VMValue(value)) : std::nullopt;
	}
	}
}

namespace
{
std::string_view TypeSuffix(DataType type)
{
	const auto index = static_cast<size_t>(type);
	return index < lhvm::k_DataTypeChars.size() ? std::string_view(lhvm::k_DataTypeChars.at(index)) : std::string_view();
}

std::string OpcodeName(Opcode code)
{
	const auto index = static_cast<size_t>(code);
	return index < lhvm::k_OpcodeNames.size() ? lhvm::k_OpcodeNames.at(index) : fmt::format("OP{}", index);
}
} // namespace

std::vector<Token> Disassemble(const Program& program, const VMScript* script, uint32_t address)
{
	using Kind = Token::Kind;
	std::vector<Token> tokens;
	if (address >= program.code.size())
	{
		return tokens;
	}
	const auto& instruction = program.code[address];
	const auto opcode = OpcodeName(instruction.code);
	const auto push = [&tokens](Kind kind, std::string text, uint32_t target = 0) {
		tokens.push_back({.kind = kind, .text = std::move(text), .target = target});
	};
	const auto pushVariable = [&](uint32_t id) {
		bool local = false;
		auto name = VariableName(program, script, id, local);
		push(local ? Kind::Local : Kind::Global, std::move(name), id);
	};

	switch (instruction.code)
	{
	case Opcode::Push:
		push(Kind::Opcode, fmt::format("PUSH{}", TypeSuffix(instruction.type)));
		if (instruction.mode == VMMode::Reference)
		{
			pushVariable(instruction.data.uintVal);
		}
		else
		{
			push(Kind::Number, FormatValue(instruction.data, instruction.type));
			if (instruction.type == DataType::Int)
			{
				if (const auto text = StringAt(program.data, instruction.data.intVal))
				{
					push(Kind::String, fmt::format("\"{}\"", *text));
				}
			}
		}
		break;
	case Opcode::Pop:
		push(Kind::Opcode, fmt::format("POP{}", TypeSuffix(instruction.type)));
		if (instruction.mode == VMMode::Reference)
		{
			pushVariable(instruction.data.uintVal);
		}
		break;
	case Opcode::Sys:
	{
		push(Kind::Opcode, "SYS");
		const auto native = instruction.data.uintVal;
		if (native < program.natives.size())
		{
			const auto& function = program.natives[native];
			push(Kind::Native, function.name, native);
			push(Kind::Comment, fmt::format("// takes {}, gives {}", function.stackIn, function.stackOut));
		}
		else
		{
			push(Kind::Native, fmt::format("NATIVE_{}", native), native);
		}
		break;
	}
	case Opcode::Run:
	{
		push(Kind::Opcode, instruction.mode == VMMode::Async ? "RUN async" : "RUN");
		const auto id = instruction.data.uintVal;
		if (id >= 1 && id <= program.scripts.size())
		{
			const auto& called = program.scripts[id - 1];
			push(Kind::Script, called.name, id);
			push(Kind::Comment, fmt::format("// {} parameter{}", called.parameterCount, called.parameterCount == 1 ? "" : "s"));
		}
		else
		{
			push(Kind::Script, fmt::format("script_{}", id), id);
		}
		break;
	}
	case Opcode::Wait:
	case Opcode::Jmp:
	case Opcode::Except:
	{
		push(Kind::Opcode, opcode);
		const auto target = instruction.data.uintVal;
		push(Kind::Jump, fmt::format("0x{:04x}", target), target);
		if (instruction.code != Opcode::Except && target <= address)
		{
			push(Kind::Comment, "// back");
		}
		break;
	}
	case Opcode::EndExcept:
		push(Kind::Opcode, instruction.mode == VMMode::Yield ? "YIELD" : "ENDEXCEPT");
		break;
	case Opcode::Cast:
		push(Kind::Opcode,
		     fmt::format("{}{}", instruction.mode == VMMode::Zero ? "ZERO" : "CAST", TypeSuffix(instruction.type)));
		break;
	case Opcode::Swap:
		if (instruction.type == DataType::Int)
		{
			push(Kind::Opcode, "SWAP");
		}
		else
		{
			push(Kind::Opcode, instruction.mode == VMMode::CopyFrom ? "COPY from" : "COPY to");
			push(Kind::Number, std::to_string(instruction.data.intVal));
		}
		break;
	case Opcode::Line:
		push(Kind::Opcode, "LINE");
		push(Kind::Number, std::to_string(instruction.data.intVal));
		break;
	case Opcode::Add:
	case Opcode::Sub:
	case Opcode::Mul:
	case Opcode::Div:
	case Opcode::Mod:
	case Opcode::Neg:
		push(Kind::Opcode, fmt::format("{}{}", opcode, TypeSuffix(instruction.type)));
		break;
	default:
		push(Kind::Opcode, opcode);
		break;
	}
	return tokens;
}

std::string LineText(std::span<const Token> tokens)
{
	std::string text;
	for (const auto& token : tokens)
	{
		if (!text.empty())
		{
			text.push_back(' ');
		}
		text += token.text;
	}
	return text;
}

std::optional<uint32_t> FindNext(const Program& program, const VMScript* script, CodeRange range, std::string_view search,
                                 std::optional<uint32_t> after)
{
	if (search.empty() || range.Size() == 0)
	{
		return std::nullopt;
	}
	const auto start = after.has_value() && range.Contains(*after) ? *after - range.begin + 1 : 0;
	for (uint32_t i = 0; i < range.Size(); ++i)
	{
		const auto address = range.begin + ((start + i) % range.Size());
		const auto tokens = Disassemble(program, script, address);
		if (MatchesSearch(fmt::format("0x{:04x} {}", address, LineText(tokens)), search))
		{
			return address;
		}
	}
	return std::nullopt;
}

std::vector<NativeUse> NativesCalled(std::span<const VMInstruction> code, CodeRange range)
{
	std::map<uint32_t, uint32_t> calls;
	for (auto address = range.begin; address < range.end && address < code.size(); ++address)
	{
		if (code[address].code == Opcode::Sys)
		{
			++calls[code[address].data.uintVal];
		}
	}
	std::vector<NativeUse> uses;
	uses.reserve(calls.size());
	for (const auto& [native, count] : calls)
	{
		uses.push_back({.native = native, .calls = count});
	}
	return uses;
}

bool IsImplemented(uint32_t native, std::span<const uint32_t> unimplemented)
{
	return !std::ranges::binary_search(unimplemented, native);
}

Coverage CoverageOf(std::span<const NativeUse> uses, std::span<const uint32_t> unimplemented)
{
	Coverage coverage {.used = uses.size(), .implemented = 0};
	coverage.implemented = static_cast<size_t>(std::ranges::count_if(
	    uses, [unimplemented](const NativeUse& use) { return IsImplemented(use.native, unimplemented); }));
	return coverage;
}

TaskState StateOf(const lhvm::VMTask& task, bool held)
{
	if (task.stop)
	{
		return TaskState::Stopping;
	}
	if (held)
	{
		return TaskState::Held;
	}
	if (task.waitingTaskId != 0)
	{
		return TaskState::Waiting;
	}
	if (task.inExceptionHandler)
	{
		return TaskState::InExceptionHandler;
	}
	if (task.sleeping)
	{
		return TaskState::Sleeping;
	}
	return TaskState::Running;
}

std::string_view Name(TaskState state)
{
	constexpr std::array<std::string_view, 6> k_Names {"Running", "Held", "Waiting", "Sleeping", "In handler", "Stopping"};
	return k_Names.at(static_cast<size_t>(state));
}

std::string Describe(const lhvm::VMTask& task, bool held, std::string_view waitedFor)
{
	switch (StateOf(task, held))
	{
	case TaskState::Stopping:
		return "Has ended; it goes at the end of the turn";
	case TaskState::Held:
		return fmt::format("Held by the debugger before 0x{:04x}", task.instructionAddress);
	case TaskState::Waiting:
		return fmt::format("Waits for task {}{}{} to finish", task.waitingTaskId, waitedFor.empty() ? "" : ", ", waitedFor);
	case TaskState::InExceptionHandler:
		return fmt::format("Runs exception handler {} of {}", task.currentExceptionHandlerIndex + 1,
		                   task.exceptionHandlerIps.size());
	case TaskState::Sleeping:
		return fmt::format("Sleeps; {:.1f} s have passed", static_cast<double>(task.ticks) / 10.0);
	case TaskState::Running:
	default:
		return "Runs at each turn";
	}
}

std::optional<size_t> LineOf(const DecompiledSource& source, uint32_t address)
{
	std::optional<size_t> best;
	std::optional<uint32_t> bestAddress;
	for (size_t line = 0; line < source.lineAddresses.size(); ++line)
	{
		const auto& lineAddress = source.lineAddresses[line];
		if (lineAddress.has_value() && *lineAddress <= address && (!bestAddress.has_value() || *lineAddress >= *bestAddress))
		{
			if (!bestAddress.has_value() || *lineAddress > *bestAddress)
			{
				best = line;
				bestAddress = lineAddress;
			}
		}
	}
	return best;
}

bool IsKeyword(std::string_view word)
{
	constexpr std::array<std::string_view, 42> k_Keywords {
	    "begin",  "end",      "script",   "while",    "if",    "elsif",      "else",   "then",        "wait",
	    "until",  "cinema",   "dialogue", "loop",     "run",   "background", "start",  "challenge",   "help",
	    "global", "constant", "define",   "function", "local", "and",        "or",     "not",         "when",
	    "return", "exit",     "true",     "false",    "known", "exception",  "camera", "multiplayer", "with",
	    "for",    "remove",   "force",    "single",   "line",  "is",
	};
	return std::ranges::any_of(k_Keywords, [word](std::string_view keyword) {
		return keyword.size() == word.size() && std::ranges::equal(keyword, word, [](char a, char b) {
			       return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		       });
	});
}

std::vector<SourceToken> HighlightSource(std::string_view line)
{
	using Kind = SourceToken::Kind;
	std::vector<SourceToken> tokens;
	const auto push = [&tokens](Kind kind, std::string_view text) {
		if (text.empty())
		{
			return;
		}
		// Plain text runs together
		if (kind == Kind::Text && !tokens.empty() && tokens.back().kind == Kind::Text)
		{
			tokens.back().text = std::string_view(tokens.back().text.data(), tokens.back().text.size() + text.size());
			return;
		}
		tokens.push_back({.kind = kind, .text = text});
	};
	const auto isWord = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; };
	size_t i = 0;
	while (i < line.size())
	{
		const auto c = line[i];
		if (c == '/' && i + 1 < line.size() && line[i + 1] == '/')
		{
			push(Kind::Comment, line.substr(i));
			break;
		}
		if (c == '"')
		{
			auto end = line.find('"', i + 1);
			end = end == std::string_view::npos ? line.size() : end + 1;
			push(Kind::String, line.substr(i, end - i));
			i = end;
			continue;
		}
		if (isWord(c))
		{
			auto end = i;
			while (end < line.size() &&
			       (isWord(line[end]) || (line[end] == '.' && std::isdigit(static_cast<unsigned char>(c)) != 0)))
			{
				++end;
			}
			const auto word = line.substr(i, end - i);
			const bool number = std::isdigit(static_cast<unsigned char>(c)) != 0;
			push(number ? Kind::Number : (IsKeyword(word) ? Kind::Keyword : Kind::Text), word);
			i = end;
			continue;
		}
		push(Kind::Text, line.substr(i, 1));
		++i;
	}
	return tokens;
}

} // namespace openblack::editor::scripts
