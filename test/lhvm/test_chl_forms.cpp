/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/// Every statement form of the language, through the compiler and the decompiler: for each form of ChlForms.h a
/// sample statement is written (with its optional parts and without), compiled, checked to call the form's native,
/// decompiled, and compiled again, which must give the same instructions.

#include <cstdio>

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <vector>

#include <ChlCompiler.h>
#include <ChlForms.h>
#include <LHVMDecompiler.h>
#include <LHVMFile.h>
#include <LHVMNatives.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace openblack::lhvm;
using namespace openblack::lhvm::chl;

namespace
{

constexpr std::string_view k_Header = "challenge 7\nglobal Obj\nglobal Other\nglobal Num\nglobal Pos\n";

/// The native the form calls: the form says which of those sharing its name
const NativeSignature* SignatureOf(const StatementForm& form)
{
	size_t nth = 0;
	for (const auto& signature : DefaultNativeSignatures())
	{
		if (signature.name == form.native && nth++ == form.overload)
		{
			return &signature;
		}
	}
	return nullptr;
}

size_t CountArguments(const std::vector<PatternItem>& items)
{
	size_t count = 0;
	for (const auto& item : items)
	{
		if (item.argument >= 0)
		{
			count = std::max(count, static_cast<size_t>(item.argument) + 1);
		}
		count = std::max(count, CountArguments(item.items));
	}
	return count;
}

/// A sample of an argument of the given type
std::string Sample(ArgType type)
{
	switch (type)
	{
	case ArgType::Object:
		return "Obj";
	case ArgType::Coord:
		return "[1, 2, 3]";
	case ArgType::Int:
		return "3";
	case ArgType::Bool:
		return "Obj exists";
	case ArgType::String:
		return "\"Sample\"";
	default:
		return "2.5";
	}
}

/// The words of a pattern, with samples for its arguments. `full` writes the optional groups and flags, otherwise they
/// are left out.
std::string Spell(const std::vector<PatternItem>& items, const NativeSignature& signature, bool full)
{
	std::string text;
	const auto add = [&text](std::string_view word) {
		if (!word.empty())
		{
			text += (text.empty() ? "" : " ") + std::string(word);
		}
	};
	for (const auto& item : items)
	{
		switch (item.kind)
		{
		case PatternItemKind::Word:
			add(item.text);
			break;
		case PatternItemKind::Argument:
			if (item.enumName == "CURRENT_CHALLENGE")
			{
				break;
			}
			add(Sample(static_cast<size_t>(item.argument) < signature.params.size()
			               ? signature.params[static_cast<size_t>(item.argument)].type
			               : ArgType::Float));
			break;
		case PatternItemKind::Flag:
			if (full)
			{
				add(item.text);
			}
			break;
		case PatternItemKind::Choice:
		{
			// The first alternative that has words, or the last one when full is off
			const auto& choice = full ? item.choices.front() : item.choices.back();
			add(choice.first);
			break;
		}
		case PatternItemKind::Optional:
			if (full)
			{
				add(Spell(item.items, signature, true));
			}
			break;
		default:
			break;
		}
	}
	return text;
}

/// A script using the form where its result fits
std::string Wrap(const std::string& form, ArgType result, bool getProperty)
{
	std::string body;
	if (getProperty)
	{
		result = form.find(" is ") != std::string::npos ? ArgType::Bool : ArgType::Float;
	}
	switch (result)
	{
	case ArgType::None:
		body = "\t" + form + "\n";
		break;
	case ArgType::Bool:
		body = "\tif " + form + "\n\tend if\n";
		break;
	case ArgType::Coord:
		body = "\tPos = marker at " + form + "\n";
		break;
	case ArgType::Object:
		body = "\tOther = " + form + "\n";
		break;
	case ArgType::Int:
		body = "\tNum = variable " + form + "\n";
		break;
	default:
		body = "\tNum = " + form + "\n";
		break;
	}
	return std::string(k_Header) + "begin script Sample\nstart\n" + body + "end script Sample\n";
}

std::vector<std::array<uint32_t, 4>> Code(const LHVMFile& program)
{
	std::vector<std::array<uint32_t, 4>> result;
	for (const auto& instruction : program.GetInstructions())
	{
		result.push_back({static_cast<uint32_t>(instruction.code), static_cast<uint32_t>(instruction.mode),
		                  static_cast<uint32_t>(instruction.type), instruction.data.uintVal});
	}
	return result;
}

bool Calls(const LHVMFile& program, std::string_view native)
{
	const auto natives = DefaultNativeSignatures();
	return std::ranges::any_of(program.GetInstructions(), [&](const VMInstruction& instruction) {
		return instruction.code == Opcode::Sys && natives[instruction.data.uintVal].name == native;
	});
}

struct Outcome
{
	size_t samples {0};
	size_t roundTrips {0};
	std::vector<std::string> failures;
};

void Check(const StatementForm& form, bool full, Outcome& outcome, std::set<std::string>& seen)
{
	// Positions ([Thing]) and the distance tests ("A not at B") are read by the expression grammar itself and
	// tested on their own
	if (form.native == "GET_POSITION" || form.native == "GET_DISTANCE")
	{
		return;
	}
	const auto items = ParsePattern(form.pattern);
	if (items.empty())
	{
		return;
	}
	const auto* signature = SignatureOf(form);
	if (signature == nullptr || signature->stackIn < 0 || signature->params.size() < CountArguments(items))
	{
		return;
	}
	const auto text = Spell(items, *signature, full);
	const auto source = Wrap(text, signature->returnType, form.native == "GET_PROPERTY");
	if (!seen.insert(source).second)
	{
		return;
	}
	++outcome.samples;
	const auto label = fmt::format("{} \"{}\" as \"{}\"", form.native, form.pattern, text);

	const std::vector<SourceFile> sources = {{.name = "Sample.txt", .text = source}};
	const auto compiled = Compile(sources);
	if (!compiled.program)
	{
		outcome.failures.push_back(
		    fmt::format("{}: doesn't compile: {}", label, compiled.diagnostics.empty() ? "" : compiled.diagnostics[0].message));
		return;
	}
	if (!Calls(*compiled.program, form.native))
	{
		outcome.failures.push_back(fmt::format("{}: compiles without calling {}", label, form.native));
		return;
	}

	const auto view = ProgramView::From(*compiled.program);
	// The challenge the header names is in effect
	const auto decompiled = DecompileScript(view, 0, {.challenge = 7, .standalone = false});
	const auto again =
	    Compile(std::vector<SourceFile> {{.name = "Sample.txt", .text = std::string(k_Header) + decompiled.text}});
	if (!again.program)
	{
		outcome.failures.push_back(fmt::format("{}: decompiled as {} which doesn't compile: {}", label, decompiled.text,
		                                       again.diagnostics.empty() ? "" : again.diagnostics[0].message));
		return;
	}
	if (Code(*again.program) != Code(*compiled.program))
	{
		outcome.failures.push_back(fmt::format("{}: decompiled as {} which compiles differently", label, decompiled.text));
		return;
	}
	++outcome.roundTrips;
}

} // namespace

TEST(ChlForms, EveryFormCompilesAndRoundTrips)
{
	Outcome outcome;
	std::set<std::string> seen;
	for (const auto& form : StatementForms())
	{
		Check(form, true, outcome, seen);
		Check(form, false, outcome, seen);
	}
	std::printf("%zu samples of %zu forms, %zu round trips\n", outcome.samples, StatementForms().size(), outcome.roundTrips);
	for (const auto& failure : outcome.failures)
	{
		ADD_FAILURE() << failure;
	}
	EXPECT_EQ(outcome.roundTrips, outcome.samples);
	EXPECT_GT(outcome.samples, StatementForms().size());
}
