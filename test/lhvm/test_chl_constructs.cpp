/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/// The language's constructs through the compiler and the decompiler. Each sample is compiled, decompiled and compiled
/// again, which must give the same instructions. Samples written the way the decompiler writes them must also come
/// back as the same text.

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

constexpr std::string_view k_Globals = "global X\nglobal Y\nglobal Obj\nglobal Other\n";

struct Sample
{
	std::string_view name;
	/// The body of a script, one statement per line, indented by one tab
	std::string_view body;
	/// The decompiler writes the body back as it is
	bool sameText {true};
	/// Locals before "start"
	std::string_view locals {};
};

// clang-format off
constexpr Sample k_Samples[] = {
    // Assignments and arithmetic
    {"assign", "\tX = 1\n"},
    {"assign variable", "\tX = Y\n"},
    {"add to", "\tX += 2\n"},
    {"subtract from", "\tX -= Y\n"},
    {"multiply", "\tX *= 2\n"},
    {"divide", "\tX /= 4\n"},
    {"increment", "\tX++\n"},
    {"decrement", "\tX--\n"},
    {"negate", "\tX = -Y\n"},
    {"negative literal", "\tX = -5\n"},
    {"precedence", "\tX = (X + 1) * 2\n"},
    {"left associative", "\tX = X - Y - 1\n"},
    {"right group", "\tX = X - (Y - 1)\n"},
    {"modulus", "\tX = X % 3\n"},
    {"mixed", "\tX = X * Y + 3 / 2 - -X\n"},
    {"fraction", "\tX = 0.125\n"},
    {"big number", "\tX = 2000000\n", false},
    {"small number", "\tX = 0.00001\n", false},
    {"constant as number", "\tX = variable 5\n"},
    {"number as constant", "\tsay constant X\n"},
    // Conditions
    {"if", "\tif X == 1\n\t\tY = 2\n\tend if\n"},
    {"if else", "\tif X != 1\n\t\tY = 2\n\telse\n\t\tY = 3\n\tend if\n"},
    {"elsif chain", "\tif X < 1\n\t\tY = 1\n\telsif X <= 2\n\t\tY = 2\n\telsif X > 3\n\t\tY = 3\n\telse\n\t\tY = 4\n\tend if\n"},
    {"nested if", "\tif X >= 1\n\t\tif Y == 2\n\t\t\tX = 0\n\t\tend if\n\tend if\n"},
    {"and or", "\tif X == 1 or Y == 2 and X == 3\n\t\tX = 0\n\tend if\n"},
    {"grouped or", "\tif (X == 1 or Y == 2) and X == 3\n\t\tX = 0\n\tend if\n"},
    {"not", "\tif not X == 1\n\t\tX = 0\n\tend if\n"},
    {"not group", "\tif not (X == 1 or Y == 2)\n\t\tX = 0\n\tend if\n"},
    {"object tests", "\tif Obj exists and Other not exists\n\t\tX = 0\n\tend if\n"},
    {"property test", "\tif Obj is 5 and Other is not 6\n\t\tX = 0\n\tend if\n"},
    {"empty if", "\tif X == 1\n\tend if\n"},
    // Loops and waiting
    {"while", "\twhile X < 10\n\t\tX++\n\tend while\n"},
    // Handlers are written inside the loop, after its body
    {"while handlers", "\twhile X < 10\n\t\tX++\n\t\twhen Y == 1\n\t\t\tY = 0\n\t\tuntil Y == 2\n\t\t\tX = 0\n\tend while\n"},
    {"begin loop", "\tbegin loop\n\t\twait 1 seconds\n\t\tuntil X == 3\n\tend loop\n"},
    {"handlers at loop level", "\tbegin loop\n\t\twait 1 seconds\n\tuntil X == 3\n\tend loop\n", false},
    {"script handlers", "\tX = 1\n\twhen Y == 1\n\t\tY = 0\n\tuntil X == 2\n\t\tX = 0\n"},
    {"nested loops", "\twhile X < 3\n\t\twhile Y < 3\n\t\t\tY++\n\t\tend while\n\t\tX++\n\tend while\n"},
    {"wait until", "\twait until X == 1\n"},
    {"wait seconds", "\twait 2 seconds\n"},
    {"wait expression seconds", "\twait X + 1 seconds\n"},
    {"wait condition", "\twait until Obj exists or 5 seconds\n"},
    // Blocks
    {"cinema", "\tbegin cinema\n\t\tX = 1\n\tend cinema\n"},
    {"camera", "\tbegin camera\n\t\tX = 1\n\tend camera\n"},
    {"dialogue", "\tbegin dialogue\n\t\tsay \"Hello\"\n\t\twait until read\n\tend dialogue\n"},
    {"cinema with dialogue", "\tbegin cinema\n\t\tX = 1\n\tend cinema with dialogue\n\tY = 2\n\tend dialogue\n"},
    {"dual camera", "\tbegin dual camera to Obj Other\n\t\tX = 1\n\tend dual camera\n"},
    // Properties
    {"property", "\tX = 8 of Obj\n"},
    {"set property", "\t8 of Obj = 2\n"},
    {"add to property", "\t8 of Obj += 2\n"},
    {"increment property", "\t8 of Obj++\n"},
    // Statements made of several calls
    {"play", "\tObj play 272 loop 1\n"},
    {"play forever", "\tObj play 272 loop -1\n"},
    {"state", "\tstate Obj 15\n\t\tposition [1, 2, 3]\n\t\tfloat 6\n\t\tulong 4, 20\n", false},
    {"camera enum", "\tset camera to 33\n\tmove camera to 33 time 2\n", false},
    // Positions
    {"position of", "\tset Obj position to [Other]\n"},
    {"position", "\tset Obj position to [1, 2, 3]\n"},
    {"ground position", "\tset Obj position to [1, 3]\n"},
    {"position sum", "\tset Obj position to [Other] + [1, 0, 1]\n"},
    {"position difference", "\tset Obj position to [Other] - [1, 0, 1]\n"},
    {"position scaled", "\tset Obj position to 2 * [Other]\n"},
    {"near", "\tif [Obj] near [Other] radius 5\n\t\tX = 0\n\tend if\n", false},
    {"at", "\tif [Obj] at [Other] or [Obj] not at [1, 2, 3]\n\t\tX = 0\n\tend if\n", false},
    // Objects, strings and natives
    {"create", "\tObj = create 4 3 at [1, 2, 3]\n"},
    {"marker", "\tObj = marker at [Other]\n"},
    {"delete", "\tdelete Obj with fade\n"},
    {"string", "\tsay \"Hello\"\n\tstop script \"Other\"\n"},
    {"native", "\tnative SET_GAMESPEED(2)\n", false},
    {"challenge highlight", "\tchallenge 7\n\tObj = create highlight 3 at [Other]\n", false},
    {"snapshot", "\tchallenge 7\n\tsnapshot quest success 0.5 alignment 0 12 Helper(variable 3)\n", false},
    // Locals
    {"locals", "\tX = Count\n", true, "\tCount = 3\n\tStart = marker at [1, 2, 3]\n"},
};
// clang-format on

std::string Program(const Sample& sample)
{
	return fmt::format("{}begin script Helper(A)\nstart\nend script Helper\n\nbegin script Test\n{}start\n{}end script "
	                   "Test\n",
	                   k_Globals, sample.locals, sample.body);
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

std::optional<LHVMFile> CompileText(const std::string& text, std::string& errors)
{
	auto result = Compile(std::vector<SourceFile> {{.name = "Test.txt", .text = text}});
	for (const auto& diagnostic : result.diagnostics)
	{
		errors += diagnostic.ToString() + "\n";
	}
	return std::move(result.program);
}

class ChlConstructs: public testing::TestWithParam<Sample>
{
};

} // namespace

// Shown when a sample fails
void PrintTo(const Sample& sample, std::ostream* stream)
{
	*stream << sample.name;
}

TEST_P(ChlConstructs, CompileDecompileCompile)
{
	const auto& sample = GetParam();
	std::string errors;
	const auto program = CompileText(Program(sample), errors);
	ASSERT_TRUE(program.has_value()) << errors;

	const auto view = ProgramView::From(*program);
	// The program as the file it came from: its challenge, its globals and its scripts
	const auto decompiled = DecompileAll(view).Text();
	const auto again = CompileText(decompiled, errors);
	ASSERT_TRUE(again.has_value()) << "Decompiled as:\n" << decompiled << errors;
	EXPECT_EQ(Code(*again), Code(*program)) << "Decompiled as:\n" << decompiled;

	if (sample.sameText)
	{
		const auto test = DecompileScript(view, 1).text;
		const auto expected = fmt::format("begin script Test\n{}start\n{}end script Test\n", sample.locals, sample.body);
		EXPECT_EQ(test, expected);
	}
}

INSTANTIATE_TEST_SUITE_P(Samples, ChlConstructs, testing::ValuesIn(k_Samples), [](const testing::TestParamInfo<Sample>& info) {
	std::string name;
	for (const char c : info.param.name)
	{
		name += std::isalnum(static_cast<unsigned char>(c)) != 0 ? c : '_';
	}
	return name;
});
