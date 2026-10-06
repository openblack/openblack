/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/// Randomly made scripts through the compiler and the decompiler: nested arithmetic, conditions, brackets and control
/// flow, from a fixed seed. Each must compile, decompile, and compile again to the same instructions.

#include <cstdio>

#include <random>
#include <string>
#include <vector>

#include <ChlCompiler.h>
#include <LHVMDecompiler.h>
#include <LHVMFile.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace openblack::lhvm;
using namespace openblack::lhvm::chl;

namespace
{

constexpr std::string_view k_Globals = "global X\nglobal Y\nglobal Z\nglobal Obj\n";

class ScriptMaker
{
public:
	explicit ScriptMaker(uint32_t seed)
	    : _random(seed)
	{
	}

	std::string Script(std::string_view name)
	{
		std::string body;
		const auto count = Pick(1, 4);
		for (int i = 0; i < count; ++i)
		{
			body += Statement(1, 3);
		}
		return fmt::format("begin script {}\nstart\n{}end script {}\n", name, body, name);
	}

private:
	int Pick(int low, int high) { return std::uniform_int_distribution<int>(low, high)(_random); }
	bool Chance(int percent) { return Pick(1, 100) <= percent; }

	std::string Number()
	{
		switch (Pick(0, 3))
		{
		case 0:
			return std::to_string(Pick(0, 99));
		case 1:
			return fmt::format("{}.{}", Pick(0, 9), Pick(1, 9));
		case 2:
			return fmt::format("0.{}{}", Pick(0, 9), Pick(1, 9));
		default:
			return fmt::format("variable {}", Pick(0, 20));
		}
	}

	std::string Value(int depth)
	{
		if (depth <= 0 || Chance(30))
		{
			switch (Pick(0, 7))
			{
			case 0:
				return "X";
			case 1:
				return "Y";
			case 2:
				// Forms ending with an object, a position or a number
				return "(size of Obj)";
			case 3:
				return "(8 of Obj)";
			case 4:
				return "(get distance from [Obj] to [1, 2, 3])";
			case 5:
				return "(number from 1 to X)";
			default:
				return Number();
			}
		}
		if (Chance(15))
		{
			const auto operand = Value(depth - 1);
			return operand.starts_with('-') ? "-(" + operand + ")" : "-" + operand;
		}
		static constexpr std::array<std::string_view, 5> k_Operators = {"+", "-", "*", "/", "%"};
		auto text = fmt::format("{} {} {}", Value(depth - 1), k_Operators[static_cast<size_t>(Pick(0, 4))], Value(depth - 1));
		return Chance(40) ? "(" + text + ")" : text;
	}

	std::string Condition(int depth)
	{
		if (depth <= 0 || Chance(30))
		{
			switch (Pick(0, 5))
			{
			case 0:
				return "Obj exists";
			case 1:
				return "Obj not exists";
			case 2:
				return "[Obj] near [X, Y] radius " + Number();
			default:
			{
				static constexpr std::array<std::string_view, 6> k_Comparisons = {"==", "!=", "<", "<=", ">", ">="};
				return fmt::format("{} {} {}", Value(2), k_Comparisons[static_cast<size_t>(Pick(0, 5))], Value(2));
			}
			}
		}
		switch (Pick(0, 3))
		{
		case 0:
			return "not " + Bracketed(Condition(depth - 1));
		case 1:
		{
			auto text = fmt::format("{} and {}", Condition(depth - 1), Condition(depth - 1));
			return Chance(40) ? "(" + text + ")" : text;
		}
		default:
		{
			auto text = fmt::format("{} or {}", Condition(depth - 1), Condition(depth - 1));
			return Chance(40) ? "(" + text + ")" : text;
		}
		}
	}

	static std::string Bracketed(const std::string& text) { return "(" + text + ")"; }

	std::string Statement(int indent, int depth)
	{
		const std::string tab(static_cast<size_t>(indent), '\t');
		const auto choice = depth <= 0 ? Pick(0, 2) : Pick(0, 6);
		switch (choice)
		{
		case 0:
			return fmt::format("{}X = {}\n", tab, Value(3));
		case 1:
			return fmt::format("{}Y += {}\n", tab, Value(2));
		case 2:
			return fmt::format("{}wait until {}\n", tab, Condition(2));
		case 3:
		{
			std::string text = fmt::format("{}if {}\n{}", tab, Condition(3), Statement(indent + 1, depth - 1));
			const auto branches = Pick(0, 2);
			for (int i = 0; i < branches; ++i)
			{
				text += fmt::format("{}elsif {}\n{}", tab, Condition(2), Statement(indent + 1, depth - 1));
			}
			if (Chance(50))
			{
				text += fmt::format("{}else\n{}", tab, Statement(indent + 1, depth - 1));
			}
			return text + tab + "end if\n";
		}
		case 4:
			return fmt::format("{}while {}\n{}{}end while\n", tab, Condition(2), Statement(indent + 1, depth - 1), tab);
		case 5:
			return fmt::format("{}begin loop\n{}{}\tuntil {}\n{}\t\tZ = 1\n{}end loop\n", tab, Statement(indent + 1, depth - 1),
			                   tab, Condition(2), tab, tab);
		default:
			return fmt::format("{}begin cinema\n{}{}end cinema\n", tab, Statement(indent + 1, depth - 1), tab);
		}
	}

	std::mt19937 _random;
};

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

std::string Errors(const CompileResult& result)
{
	std::string text;
	for (const auto& diagnostic : result.diagnostics)
	{
		text += diagnostic.ToString() + "\n";
	}
	return text;
}

} // namespace

TEST(ChlRandom, ScriptsSurviveDecompilingAndCompiling)
{
	constexpr int k_Scripts = 1000;
	ScriptMaker maker(20261006);
	int roundTrips = 0;
	for (int i = 0; i < k_Scripts; ++i)
	{
		const auto source = std::string(k_Globals) + maker.Script(fmt::format("Random{}", i));
		const auto compiled = Compile(std::vector<SourceFile> {{.name = "Random.txt", .text = source}});
		ASSERT_TRUE(compiled.program.has_value()) << source << Errors(compiled);

		const auto view = ProgramView::From(*compiled.program);
		const auto decompiled = DecompileScript(view, 0);
		const auto again =
		    Compile(std::vector<SourceFile> {{.name = "Random.txt", .text = std::string(k_Globals) + decompiled.text}});
		ASSERT_TRUE(again.program.has_value()) << source << "Decompiled as:\n" << decompiled.text << Errors(again);
		EXPECT_EQ(Code(*again.program), Code(*compiled.program)) << source << "Decompiled as:\n" << decompiled.text;
		if (Code(*again.program) == Code(*compiled.program))
		{
			++roundTrips;
		}
	}
	std::printf("%d of %d random scripts round trip\n", roundTrips, k_Scripts);
}

namespace
{

/// Compile `source`, decompile it, compile the result: whether the instructions come back the same
bool RoundTrips(const std::string& source, std::string& decompiledText)
{
	const auto compiled = Compile(std::vector<SourceFile> {{.name = "Sample.txt", .text = source}});
	if (!compiled.program)
	{
		return false;
	}
	const auto view = ProgramView::From(*compiled.program);
	decompiledText = DecompileScript(view, 0).text;
	const auto again =
	    Compile(std::vector<SourceFile> {{.name = "Sample.txt", .text = std::string(k_Globals) + decompiledText}});
	return again.program && Code(*again.program) == Code(*compiled.program);
}

} // namespace

// A form's last argument takes in all that can follow it, so "get distance from [A] to [B] / 2" would divide the
// position [B]; and two minus signs together read as "--".
TEST(ChlRandom, FormsEndingWithAPositionAreBracketed)
{
	std::string decompiled;
	EXPECT_TRUE(RoundTrips(std::string(k_Globals) +
	                           "begin script Sample\nstart\n\tX = (get distance from [Obj] to [1, 2, 3]) / 2\n"
	                           "end script Sample\n",
	                       decompiled))
	    << decompiled;
}

TEST(ChlRandom, DoubleNegationIsBracketed)
{
	for (const std::string value : {"-(-Y)", "-(-5)", "-(-(-0.5))", "-(get distance from [Obj] to [1, 2, 3]) / 2"})
	{
		std::string decompiled;
		EXPECT_TRUE(RoundTrips(std::string(k_Globals) + "begin script Sample\nstart\n\tX = " + value + "\nend script Sample\n",
		                       decompiled))
		    << decompiled;
	}
}

// The language has no exponents, however large or small the number
TEST(ChlRandom, NumbersHaveNoExponent)
{
	std::string decompiled;
	EXPECT_TRUE(
	    RoundTrips(std::string(k_Globals) + "begin script Sample\nstart\n\tX = 2000000 + 900000 * 0.00001\nend script Sample\n",
	               decompiled))
	    << decompiled;
	EXPECT_EQ(decompiled.find("e+"), std::string::npos) << decompiled;
	EXPECT_EQ(decompiled.find("e-"), std::string::npos) << decompiled;
}
