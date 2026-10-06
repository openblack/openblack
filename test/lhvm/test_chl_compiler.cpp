/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <algorithm>
#include <bit>
#include <sstream>
#include <string>
#include <vector>

#include <ChlCompiler.h>
#include <ChlConstants.h>
#include <ChlLexer.h>
#include <ChlParser.h>
#include <LHVM.h>
#include <LHVMFile.h>
#include <LHVMNatives.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace openblack::lhvm;
using namespace openblack::lhvm::chl;

namespace
{

/// One instruction as text: "PUSHF 1.5", "PUSHF [3]", "POPI", "JZ fwd 7", "SYS MOVE_GAME_THING", "SYS2 SET_PROPERTY"
std::string Describe(const VMInstruction& instruction)
{
	static const std::array<std::string_view, 7> k_TypeLetters = {"", "I", "F", "C", "O", "", "B"};
	const auto type = static_cast<size_t>(instruction.type);
	const std::string_view letter = type < k_TypeLetters.size() ? k_TypeLetters[type] : "?";
	const auto data = instruction.data;
	const auto mode = static_cast<uint32_t>(instruction.mode);
	switch (instruction.code)
	{
	case Opcode::Push:
	case Opcode::Pop:
	{
		const std::string name = instruction.code == Opcode::Push ? "PUSH" : "POP";
		if (mode == 1)
		{
			return fmt::format("{}{} [{}]", name, letter, data.uintVal);
		}
		if (instruction.code == Opcode::Pop)
		{
			return fmt::format("POP{}", letter);
		}
		if (instruction.type == DataType::Float || instruction.type == DataType::Vector)
		{
			return fmt::format("{}{} {}", name, letter, data.floatVal);
		}
		return fmt::format("{}{} {}", name, letter, data.intVal);
	}
	case Opcode::Wait:
		return fmt::format("JZ {} {}", mode == 1 ? "fwd" : "back", data.uintVal);
	case Opcode::Jmp:
		return fmt::format("JMP {} {}", mode == 1 ? "fwd" : "back", data.uintVal);
	case Opcode::Except:
		return fmt::format("EXCEPT {}", data.uintVal);
	case Opcode::Sys:
		return fmt::format("{} {}", instruction.type == DataType::Float ? "SYS2" : "SYS",
		                   DefaultNativeSignatures()[data.uintVal].name);
	case Opcode::Run:
		return fmt::format("{} {}", mode == 1 ? "START" : "CALL", data.uintVal);
	case Opcode::Cast:
		return mode == 1 ? fmt::format("ZERO [{}]", data.uintVal) : fmt::format("CAST{}", letter);
	case Opcode::EndExcept:
		return mode == 1 ? "FREE" : "ENDEXCEPT";
	case Opcode::FailExcept:
		return "ITEREXCEPT";
	case Opcode::BrkExcept:
		return "BRKEXCEPT";
	case Opcode::End:
		return "END";
	case Opcode::Swap:
		return instruction.type == DataType::Float ? fmt::format("COPYTO {}", data.uintVal) : "SWAP";
	case Opcode::Not:
	case Opcode::And:
	case Opcode::Or:
	case Opcode::Sleep:
		return k_OpcodeNames[static_cast<size_t>(instruction.code)];
	default:
		return fmt::format("{}{}", k_OpcodeNames[static_cast<size_t>(instruction.code)], letter);
	}
}

std::vector<std::string> Disassemble(const LHVMFile& file, size_t first = 0, size_t count = 0)
{
	std::vector<std::string> result;
	const auto& code = file.GetInstructions();
	const auto end = count == 0 ? code.size() : std::min(code.size(), first + count);
	for (size_t i = first; i < end; ++i)
	{
		result.push_back(Describe(code[i]));
	}
	return result;
}

/// Compile one source file, expecting success
LHVMFile CompileOk(const std::string& text, const ConstantTable* constants = nullptr)
{
	const std::vector<SourceFile> sources = {{.name = "Test.txt", .text = text}};
	auto result = Compile(sources, {.constants = constants});
	for (const auto& diagnostic : result.diagnostics)
	{
		ADD_FAILURE() << diagnostic.ToString();
	}
	EXPECT_TRUE(result.program.has_value());
	return result.program.value_or(LHVMFile());
}

std::vector<CompileDiagnostic> CompileErrors(const std::string& text, const ConstantTable* constants = nullptr)
{
	const std::vector<SourceFile> sources = {{.name = "Test.txt", .text = text}};
	auto result = Compile(sources, {.constants = constants});
	EXPECT_FALSE(result.program.has_value());
	return result.diagnostics;
}

/// The body of a script: its instructions after the parameters and locals, up to its closing frame
std::vector<std::string> Body(const LHVMFile& file)
{
	auto all = Disassemble(file);
	const auto start = std::ranges::find(all, "FREE") - all.begin() + 1;
	const auto end = std::ranges::find(all, "ENDEXCEPT") - all.begin();
	return {all.begin() + start, all.begin() + end};
}

std::string TestScript(const std::string& body, const std::string& header = "")
{
	return header + "begin script Test\nstart\n" + body + "end script Test\n";
}

ConstantTable TestConstants()
{
	ConstantTable constants;
	constants.Add("SCRIPT_OBJECT_TYPE", "SCRIPT_OBJECT_TYPE_MARKER", 1);
	constants.Add("SCRIPT_OBJECT_TYPE", "SCRIPT_OBJECT_TYPE_VILLAGER", 4);
	constants.Add("SCRIPT_OBJECT_PROPERTY_TYPE", "SCRIPT_OBJECT_PROPERTY_TYPE_FLYING", 5);
	constants.Add("SCRIPT_OBJECT_PROPERTY_TYPE", "SCRIPT_OBJECT_PROPERTY_TYPE_SCALE", 8);
	constants.Add("HELP_TEXT", "HELP_TEXT_HELLO", 1234);
	constants.Add("VILLAGER_STATES", "VILLAGER_STATE_SCRIPT_PLAY_ANIM", 200);
	constants.Add("VILLAGER_STATES", "VILLAGER_STATE_WANDER", 15);
	constants.Add("ANIM", "ANM_WAVE", 42);
	constants.Add("CHALLENGE", "CHALLENGE_TEST", 7);
	constants.Add("CAMERA", "TESTT00_000", 33);
	return constants;
}

} // namespace

// ------------------------------------------------------------------------------------------------------------- lexer

TEST(ChlLexer, TokensFollowTheOriginalLexer)
{
	DiagnosticSink sink;
	const auto tokens =
	    Tokenize("Foo = 3 3. 2.5 3d 1e5 \"a\\\"b\" // comment\n/* outer /* inner */ still */ x += -y", "t", sink);
	EXPECT_EQ(sink.ErrorCount(), 0);
	std::vector<std::string> spelled;
	for (const auto& token : tokens)
	{
		spelled.push_back(fmt::format("{}:{}", static_cast<int>(token.kind), token.text));
	}
	const std::vector<std::string> expected = {"0:Foo", "3:=", "1:3",  "1:3.", "1:2.5", "0:3d", "0:1e5", "2:a\"b",
	                                           "4:\n",  "0:x", "3:+=", "3:-",  "0:y",   "4:\n", "5:"};
	EXPECT_EQ(spelled, expected);
	EXPECT_TRUE(tokens[2].integer);
	EXPECT_FALSE(tokens[3].integer);
	EXPECT_EQ(tokens[9].location.line, 2u);
}

TEST(ChlLexer, CarriageReturnsEndLines)
{
	DiagnosticSink sink;
	const auto tokens = Tokenize("a\r\nb\rc", "t", sink);
	ASSERT_GE(tokens.size(), 5u);
	EXPECT_EQ(tokens[2].text, "b");
	EXPECT_EQ(tokens[2].location.line, 2u);
	EXPECT_EQ(tokens[4].text, "c");
	EXPECT_EQ(tokens[4].location.line, 3u);
}

TEST(ChlLexer, ReportsUnterminatedStringsAndComments)
{
	DiagnosticSink sink;
	(void)Tokenize("say \"oops\n/* never closed", "t.txt", sink);
	ASSERT_EQ(sink.ErrorCount(), 2u);
	EXPECT_EQ(sink.Diagnostics()[0].ToString(), "t.txt:1:5: error: unterminated string");
	EXPECT_EQ(sink.Diagnostics()[1].location.line, 2u);
}

// ------------------------------------------------------------------------------------------------------------ parser

TEST(ChlParser, BuildsTheSyntaxTree)
{
	const auto constants = TestConstants();
	ParseEnvironment environment(DefaultNativeSignatures(), &constants);
	DiagnosticSink sink;
	const auto file = ParseFile("global G\n"
	                            "global constant FLYING = SCRIPT_OBJECT_PROPERTY_TYPE_FLYING\n"
	                            "begin help script Talk(Who)\n"
	                            "\tCount = 0\n"
	                            "start\n"
	                            "\tif Who exists and Count < 3\n"
	                            "\t\tmove Who position to [G] radius 5\n"
	                            "\telsif Who is not FLYING\n"
	                            "\t\tCount++\n"
	                            "\telse\n"
	                            "\t\twait 2 seconds\n"
	                            "\tend if\n"
	                            "\twhile Count > 0\n"
	                            "\t\tCount -= 1\n"
	                            "\twhen Who not exists\n"
	                            "\t\tCount = 0\n"
	                            "\tend while\n"
	                            "end script Talk\n",
	                            "Talk.txt", environment, sink);
	for (const auto& diagnostic : sink.Diagnostics())
	{
		ADD_FAILURE() << diagnostic.ToString();
	}
	ASSERT_EQ(file.items.size(), 2u);
	const auto& script = *std::get<ParsedScript>(file.items[1]).script;
	EXPECT_EQ(std::get<GlobalDeclaration>(file.items[0]).name, "G");
	EXPECT_EQ(script.kind, ScriptKind::HelpScript);
	EXPECT_EQ(script.params, std::vector<std::string> {"Who"});
	ASSERT_EQ(script.locals.size(), 1u);
	ASSERT_EQ(script.body.size(), 2u);

	const auto& ifStmt = *script.body[0];
	ASSERT_EQ(ifStmt.kind, StmtKind::If);
	ASSERT_EQ(ifStmt.branches.size(), 3u);
	const auto& condition = *ifStmt.branches[0].cond;
	EXPECT_EQ(condition.kind, ExprKind::Binary);
	EXPECT_EQ(condition.op, Op::And);
	EXPECT_EQ(condition.args[0]->text, "THING_VALID");
	const auto& move = *ifStmt.branches[0].body[0]->expr;
	EXPECT_EQ(move.text, "MOVE_GAME_THING");
	ASSERT_EQ(move.args.size(), 3u);
	EXPECT_EQ(move.args[1]->text, "GET_POSITION");
	EXPECT_EQ(move.args[2]->number, 5.0);
	const auto& isNot = *ifStmt.branches[1].cond;
	EXPECT_EQ(isNot.op, Op::Not);
	EXPECT_TRUE(isNot.args[0]->swapped);
	EXPECT_EQ(ifStmt.branches[2].cond, nullptr);
	EXPECT_EQ(ifStmt.branches[2].body[0]->expr->kind, ExprKind::Elapsed);

	const auto& loop = *script.body[1];
	EXPECT_EQ(loop.kind, StmtKind::While);
	ASSERT_EQ(loop.handlers.size(), 1u);
	EXPECT_EQ(loop.handlers[0]->kind, StmtKind::When);
}

TEST(ChlParser, MatchesStatementFormsFromTheTable)
{
	const auto constants = TestConstants();
	ParseEnvironment environment(DefaultNativeSignatures(), &constants);
	DiagnosticSink sink;
	const auto file = ParseFile(TestScript("\tX = create VILLAGER 3 at [1, 2, 3]\n"
	                                       "\tX = create MARKER at [1, 2]\n"
	                                       "\tenable X active\n"
	                                       "\tsay single line HELP_TEXT_HELLO with interaction\n"
	                                       "\tdelete X with fade\n",
	                                       "global X\nglobal constant VILLAGER = SCRIPT_OBJECT_TYPE_VILLAGER\n"
	                                       "global constant MARKER = SCRIPT_OBJECT_TYPE_MARKER\n"),
	                            "t.txt", environment, sink);
	ASSERT_EQ(sink.ErrorCount(), 0u) << sink.Diagnostics().front().ToString();
	const auto& body = std::get<ParsedScript>(file.items[1]).script->body;
	ASSERT_EQ(body.size(), 5u);
	EXPECT_EQ(body[0]->expr->text, "CREATE");
	EXPECT_EQ(body[1]->expr->args[1]->number, 5000.0); // a type without a subtype
	EXPECT_EQ(body[2]->expr->text, "SET_ACTIVE");
	EXPECT_EQ(body[2]->expr->args[0]->number, 1.0);
	EXPECT_EQ(body[3]->expr->text, "RUN_TEXT");
	EXPECT_EQ(body[3]->expr->args[2]->number, 1.0);
	EXPECT_EQ(body[4]->expr->text, "OBJECT_DELETE");
}

TEST(ChlParser, ReadsKeywordSynonyms)
{
	ParseEnvironment environment(DefaultNativeSignatures(), nullptr);
	DiagnosticSink sink;
	(void)ParseFile(TestScript("\twait 1 second\n\twait 2 secs\n\tset X time to 3 seconds\n", "global X\n"), "t.txt",
	                environment, sink);
	EXPECT_EQ(sink.ErrorCount(), 0u);
}

// ------------------------------------------------------------------------------------------------------- code layout

TEST(ChlCompiler, ScriptFrameParametersAndLocals)
{
	const auto file = CompileOk("global G\n"
	                            "begin script Test(A, B)\n"
	                            "\tL = A * 2\n"
	                            "start\n"
	                            "\tG = L\n"
	                            "end script Test\n");
	const std::vector<std::string> expected = {"EXCEPT 14", "POPF [2]",   "POPF [3]",   "PUSHF [2]", "PUSHF 2",   "MULF",
	                                           "POPF [4]",  "FREE",       "PUSHF [1]",  "POPI",      "PUSHF [4]", "POPF [1]",
	                                           "ENDEXCEPT", "JMP fwd 15", "ITEREXCEPT", "END"};
	EXPECT_EQ(Disassemble(file), expected);
	const auto& script = file.GetScripts().at(0);
	EXPECT_EQ(script.variablesOffset, 1u);
	EXPECT_EQ(script.parameterCount, 2u);
	EXPECT_EQ(script.variables, (std::vector<std::string> {"A", "B", "L"}));
	EXPECT_EQ(script.filename, "Test.txt");
	EXPECT_EQ(script.scriptId, 1u);
}

TEST(ChlCompiler, IfChainsItsJumps)
{
	const auto file =
	    CompileOk(TestScript("\tif X == 1\n\t\tX = 2\n\telsif X == 2\n\t\tX = 3\n\telse\n\t\tX = 4\n\tend if\n", "global X\n"));
	// Each branch ends with a jump to the next branch's jump, the last one to just past itself. "else" is a test of
	// true.
	const std::vector<std::string> expected = {
	    "EXCEPT 29", "FREE",     "PUSHF [1]",  "PUSHF 1",    "EQF",        "JZ fwd 11",  "PUSHF [1]", "POPI",
	    "PUSHF 2",   "POPF [1]", "JMP fwd 19", "PUSHF [1]",  "PUSHF 2",    "EQF",        "JZ fwd 20", "PUSHF [1]",
	    "POPI",      "PUSHF 3",  "POPF [1]",   "JMP fwd 26", "PUSHB 1",    "JZ fwd 27",  "PUSHF [1]", "POPI",
	    "PUSHF 4",   "POPF [1]", "JMP fwd 27", "ENDEXCEPT",  "JMP fwd 30", "ITEREXCEPT", "END"};
	EXPECT_EQ(Disassemble(file), expected);
}

TEST(ChlCompiler, LoopsHaveExceptionFrames)
{
	const auto file = CompileOk(TestScript("\twhile X < 3\n\t\tX++\n\tend while\n"
	                                       "\tbegin loop\n\t\twait until X > 5\n\tuntil X == 9\n\t\tX = 0\n\tend loop\n",
	                                       "global X\n"));
	const std::vector<std::string> expected = {"EXCEPT 39", "FREE",
	                                           // while: a frame even without handlers
	                                           "EXCEPT 14", "PUSHF [1]", "PUSHF 3", "LTF", "JZ fwd 12", "PUSHF [1]", "PUSHF 1",
	                                           "ADDF", "POPF [1]", "JMP back 3", "ENDEXCEPT", "JMP fwd 15", "ITEREXCEPT",
	                                           // begin loop: its only way out is the until handler
	                                           "EXCEPT 21", "PUSHF [1]", "PUSHF 5", "GTF", "JZ back 16", "JMP back 16",
	                                           "PUSHF [1]", "PUSHF 9", "EQF", "JZ fwd 36", "PUSHB 0", "SYS SET_WIDESCREEN",
	                                           "SYS END_GAME_SPEED", "SYS END_DIALOGUE", "SYS END_CAMERA_CONTROL", "PUSHF [1]",
	                                           "POPI", "PUSHF 0", "POPF [1]", "BRKEXCEPT", "JMP fwd 37", "ITEREXCEPT",
	                                           // the script's frame
	                                           "ENDEXCEPT", "JMP fwd 40", "ITEREXCEPT", "END"};
	EXPECT_EQ(Disassemble(file), expected);
}

TEST(ChlCompiler, AssignmentsAndProperties)
{
	const auto constants = TestConstants();
	const auto file = CompileOk(TestScript("\tX += 2\n\tX--\n\tSCALE of X = 3\n\tSCALE of X *= 2\n\tif X is FLYING\n\tend if\n",
	                                       "global X\nglobal constant SCALE = SCRIPT_OBJECT_PROPERTY_TYPE_SCALE\n"
	                                       "global constant FLYING = SCRIPT_OBJECT_PROPERTY_TYPE_FLYING\n"),
	                            &constants);
	const std::vector<std::string> expected = {"PUSHF [1]",
	                                           "PUSHF 2",
	                                           "ADDF",
	                                           "POPF [1]",
	                                           "PUSHF [1]",
	                                           "PUSHF 1",
	                                           "SUBF",
	                                           "POPF [1]",
	                                           "PUSHI 8",
	                                           "PUSHF [1]",
	                                           "PUSHI 8",
	                                           "PUSHF [1]",
	                                           "SYS2 GET_PROPERTY",
	                                           "POPI",
	                                           "PUSHF 3",
	                                           "SYS2 SET_PROPERTY",
	                                           "PUSHI 8",
	                                           "PUSHF [1]",
	                                           "PUSHI 8",
	                                           "PUSHF [1]",
	                                           "SYS2 GET_PROPERTY",
	                                           "PUSHF 2",
	                                           "MULF",
	                                           "SYS2 SET_PROPERTY",
	                                           "PUSHF [1]",
	                                           "PUSHI 5",
	                                           "SWAP",
	                                           "SYS2 GET_PROPERTY",
	                                           "CASTB"};
	auto body = Body(file);
	body.resize(expected.size());
	EXPECT_EQ(body, expected);
}

TEST(ChlCompiler, ValuesAndConversions)
{
	const auto constants = TestConstants();
	const auto file = CompileOk(TestScript("\tX = -5\n"
	                                       "\tX = variable HELP_TEXT_HELLO\n"
	                                       "\tsay constant X\n"
	                                       "\tX = marker at [1, 2]\n"
	                                       "\tif X exists\n\tend if\n"
	                                       "\tdelete X\n"
	                                       "\tstop all scripts excluding \"Foo\"\n",
	                                       "global X\n"),
	                            &constants);
	const std::vector<std::string> expected = {
	    "PUSHF [1]", "POPI",      "PUSHF 5",    "NEGF",    "POPF [1]",              // no constant folding
	    "PUSHF [1]", "POPI",      "PUSHI 1234", "CASTF",   "POPF [1]",              // variable CONST
	    "PUSHB 0",   "PUSHF [1]", "CASTI",      "PUSHI 0", "SYS RUN_TEXT",          // constant VAR
	    "PUSHF [1]", "POPI",      "PUSHI 1",    "PUSHI 0", "PUSHF 1",      "CASTC", // marker at [x, z]
	    "PUSHF 2",   "CASTC",     "PUSHC 0",    "SWAP",    "SYS CREATE",   "POPF [1]", "PUSHF [1]", "CASTO", "SYS THING_VALID"};
	auto body = Body(file);
	ASSERT_GE(body.size(), expected.size() + 6);
	EXPECT_EQ(std::vector<std::string>(body.begin(), body.begin() + static_cast<std::ptrdiff_t>(expected.size())), expected);
	// delete also clears the variable; strings go to the data section
	const std::vector<std::string> tail(body.end() - 6, body.end());
	EXPECT_EQ(tail, (std::vector<std::string> {"PUSHF [1]", "PUSHI 0", "SYS OBJECT_DELETE", "ZERO [1]", "PUSHI 0",
	                                           "SYS STOP_ALL_SCRIPTS_EXCLUDING"}));
	EXPECT_EQ(std::string(file.GetData().data()), "Foo");
}

TEST(ChlCompiler, BlocksAndCompoundStatements)
{
	const auto constants = TestConstants();
	const auto file = CompileOk("challenge TEST\nglobal X\n"
	                            "begin script Test\nstart\n"
	                            "\tbegin cinema\n"
	                            "\t\tX play ANM_WAVE loop 2\n"
	                            "\t\tset camera to T00_000\n"
	                            "\tend cinema\n"
	                            "\tstate X VILLAGER_STATE_WANDER\n"
	                            "\t\tfloat 6\n"
	                            "\tX = create highlight 3 at [X]\n"
	                            "end script Test\n",
	                            &constants);
	const std::vector<std::string> expected = {
	    "SYS START_CAMERA_CONTROL", "JZ back 2", "SYS START_DIALOGUE", "JZ back 4", "SYS START_GAME_SPEED", "PUSHB 1",
	    "SYS SET_WIDESCREEN",
	    // play
	    "PUSHF [1]", "PUSHO [1]", "PUSHI 42", "PUSHF 2", "CASTI", "SYS SET_SCRIPT_ULONG", "PUSHI 200", "SYS SET_SCRIPT_STATE",
	    // set camera to
	    "PUSHI 33", "SYS CONVERT_CAMERA_FOCUS", "PUSHI 33", "SYS CONVERT_CAMERA_POSITION", "SYS SET_CAMERA_POSITION",
	    "SYS SET_CAMERA_FOCUS", "PUSHB 0", "SYS SET_WIDESCREEN", "SYS END_GAME_SPEED", "SYS END_CAMERA_CONTROL",
	    "SYS END_DIALOGUE",
	    // state
	    "PUSHF [1]", "PUSHI 15", "PUSHI [1]", "PUSHF 6", "SYS SET_SCRIPT_FLOAT", "SYS SET_SCRIPT_STATE",
	    // the current challenge's id is pushed last
	    "PUSHF [1]", "POPI", "PUSHI 3", "PUSHF [1]", "SYS GET_POSITION", "PUSHI 7", "SYS CREATE_HIGHLIGHT", "POPF [1]"};
	EXPECT_EQ(Body(file), expected);
}

TEST(ChlCompiler, DistanceTestsAndRunScript)
{
	const auto file = CompileOk("global X\n"
	                            "begin script Other(A)\nstart\nend script Other\n"
	                            "begin script Test\nstart\n"
	                            "\twait until [X] near [1, 2, 3] radius 4 or [X] not at [X]\n"
	                            "\trun background script Other(X + 1)\n"
	                            "end script Test\n");
	const auto& scripts = file.GetScripts();
	ASSERT_EQ(scripts.size(), 2u);
	const auto all = Disassemble(file, scripts[1].instructionAddress + 2, 25);
	const std::vector<std::string> expected = {"PUSHF [1]",
	                                           "SYS GET_POSITION",
	                                           "PUSHF 1",
	                                           "CASTC",
	                                           "PUSHF 2",
	                                           "CASTC",
	                                           "PUSHF 3",
	                                           "CASTC",
	                                           "SYS GET_DISTANCE",
	                                           "PUSHF 4",
	                                           "LTF",
	                                           "PUSHF [1]",
	                                           "SYS GET_POSITION",
	                                           "PUSHF [1]",
	                                           "SYS GET_POSITION",
	                                           "SYS GET_DISTANCE",
	                                           "PUSHF 0",
	                                           "EQF",
	                                           "NOT",
	                                           "OR",
	                                           "JZ back 9",
	                                           "PUSHF [1]",
	                                           "PUSHF 1",
	                                           "ADDF",
	                                           "START 1"};
	EXPECT_EQ(all, expected);
}

TEST(ChlCompiler, LineNumbersFollowTheOriginalParser)
{
	const auto file = CompileOk("global X\n"          // 1
	                            "begin script Test\n" // 2
	                            "start\n"             // 3
	                            "\tif X == 1 and\n"   // 4
	                            "\t\tX == 2\n"        // 5
	                            "\t\tX = 3\n"         // 6
	                            "\t// a comment\n"    // 7
	                            "\tend if\n"          // 8
	                            "end script Test\n"); // 9
	std::vector<uint32_t> lines;
	for (const auto& instruction : file.GetInstructions())
	{
		lines.push_back(instruction.line);
	}
	// EXCEPT (patched at "end script"), FREE, the test spread over two lines, the jump patched at "end if", the
	// assignment, the branch's jump (emitted at "end if"), the frame
	const std::vector<uint32_t> expected = {9, 0, 4, 4, 4, 5, 5, 5, 5, 8, 6, 6, 6, 6, 8, 9, 0, 9, 0};
	EXPECT_EQ(lines, expected);
}

TEST(ChlCompiler, NativeCallsGotoAndLabels)
{
	const auto file = CompileOk(TestScript("\tnative SET_GAMESPEED(2)\n\tgoto done\n\tX = 1\ndone:\n", "global X\n"));
	const std::vector<std::string> expected = {"PUSHF 2", "SYS2 SET_GAMESPEED", "JMP fwd 9", "PUSHF [1]", "POPI", "PUSHF 1",
	                                           "POPF [1]"};
	EXPECT_EQ(Body(file), expected);
}

TEST(ChlCompiler, ScriptsAcrossFilesShareGlobalsAndIds)
{
	const std::vector<SourceFile> sources = {
	    {.name = "A.txt", .text = "global One\nrun script Starter\nbegin script First\nstart\nend script First\n"},
	    {.name = "B.txt",
	     .text = "global Two\nbegin script Starter\nstart\n\trun script First\n\tTwo = One\nend script Starter\n"}};
	const auto result = Compile(sources);
	ASSERT_TRUE(result.program.has_value());
	const auto& program = *result.program;
	EXPECT_EQ(program.GetVariablesNames(), (std::vector<std::string> {"One", "Two"}));
	EXPECT_EQ(program.GetAutostart(), std::vector<uint32_t> {2});
	ASSERT_EQ(program.GetScripts().size(), 2u);
	EXPECT_EQ(program.GetScripts()[0].variablesOffset, 1u);
	EXPECT_EQ(program.GetScripts()[1].variablesOffset, 2u);
	EXPECT_EQ(program.GetScripts()[1].filename, "B.txt");
	const auto body =
	    Body(LHVMFile(program.GetVersion(), {},
	                  std::vector<VMInstruction>(program.GetInstructions().begin() + program.GetScripts()[1].instructionAddress,
	                                             program.GetInstructions().end()),
	                  {}, {}, {}));
	EXPECT_EQ(body, (std::vector<std::string> {"CALL 1", "PUSHF [2]", "POPI", "PUSHF [1]", "POPF [2]"}));
}

// ------------------------------------------------------------------------------------------------------------ errors

TEST(ChlCompiler, ReportsErrorsWithPositions)
{
	const auto errors = CompileErrors("begin script Test\n"
	                                  "start\n"
	                                  "\tUnknown = 3\n"
	                                  "\tmove Foo position to [1, 2, 3]\n"
	                                  "\twait until\n"
	                                  "end script Test\n");
	ASSERT_GE(errors.size(), 3u);
	EXPECT_EQ(errors[0].location.line, 3u);
	EXPECT_EQ(errors[1].location.line, 4u);
	EXPECT_NE(errors[1].message.find("\"Foo\" is not a variable or constant"), std::string::npos) << errors[1].message;
	EXPECT_EQ(errors[2].location.line, 5u);
	EXPECT_EQ(errors[0].file, "Test.txt");
}

TEST(ChlCompiler, ReportsBlockAndScriptMistakes)
{
	auto errors = CompileErrors("begin script Test\nstart\n\tif 1 == 1\nend script Other\n");
	ASSERT_FALSE(errors.empty());
	EXPECT_NE(errors[0].message.find("expected 'end if' to close the block opened at line 3"), std::string::npos)
	    << errors[0].message;

	errors = CompileErrors("begin script Test\nstart\n\trun script Missing(1)\nend script Test\n");
	ASSERT_EQ(errors.size(), 1u);
	EXPECT_EQ(errors[0].ToString(), "Test.txt:3:2: error: cannot run script \"Missing\": it doesn't exist");

	errors =
	    CompileErrors("begin script A(X)\nstart\nend script A\nbegin script Test\nstart\n\trun script A\nend script Test\n");
	ASSERT_EQ(errors.size(), 1u);
	EXPECT_NE(errors[0].message.find("takes 1 arguments, not 0"), std::string::npos);
}

TEST(ChlCompiler, KeepsGoingAfterAnError)
{
	const auto errors = CompileErrors(TestScript("\tX = = 3\n\tX = 2\n\tY = 1\n\tZ = 1\n", "global X\n"));
	// The two unknown names are reported as well as the syntax error
	EXPECT_EQ(errors.size(), 3u);
}

// --------------------------------------------------------------------------------------------- recompiling one script

TEST(ChlCompiler, RecompilesAScriptIntoAProgram)
{
	const std::vector<SourceFile> sources = {
	    {.name = "A.txt",
	     .text = "global G\nbegin script Main\nstart\n\trun script Helper(1)\nend script Main\n"
	             "begin script Helper(N)\nstart\n\tG = N\nend script Helper\n"}};
	const auto original = Compile(sources);
	ASSERT_TRUE(original.program.has_value());
	const auto oldSize = original.program->GetInstructions().size();

	const auto edited = CompileScript(*original.program, {.name = "A.txt",
	                                                      .text = "begin script Helper(N)\nstart\n\tG = N * 2\n"
	                                                              "end script Helper\n"});
	for (const auto& diagnostic : edited.diagnostics)
	{
		ADD_FAILURE() << diagnostic.ToString();
	}
	ASSERT_TRUE(edited.program.has_value());
	const auto& scripts = edited.program->GetScripts();
	ASSERT_EQ(scripts.size(), 2u);
	EXPECT_EQ(scripts[1].name, "Helper");
	EXPECT_EQ(scripts[1].scriptId, 2u);
	EXPECT_EQ(scripts[1].instructionAddress, oldSize);
	EXPECT_EQ(scripts[0].instructionAddress, original.program->GetScripts()[0].instructionAddress);
	const auto body = Disassemble(*edited.program, oldSize + 2, 6);
	EXPECT_EQ(body, (std::vector<std::string> {"FREE", "PUSHF [1]", "POPI", "PUSHF [2]", "PUSHF 2", "MULF"}));

	const auto broken = CompileScript(*original.program,
	                                  {.name = "A.txt", .text = "begin script Helper(N)\nstart\n\tQ = 1\nend script Helper\n"});
	EXPECT_FALSE(broken.program.has_value());
	EXPECT_FALSE(broken.diagnostics.empty());
}

// ------------------------------------------------------------------------------------------------------------ writer

TEST(ChlCompiler, WrittenProgramsReadBack)
{
	const auto file = CompileOk(TestScript("\tstop all scripts excluding \"Foo\"\n\tX = 1\n", "global X\n"));
	std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
	file.Write(stream);
	const auto text = stream.str();
	const std::vector<uint8_t> bytes(text.begin(), text.end());
	LHVMFile read;
	read.Open(bytes);
	ASSERT_TRUE(read.IsLoaded());
	EXPECT_EQ(read.GetVariablesNames(), file.GetVariablesNames());
	EXPECT_EQ(Disassemble(read), Disassemble(file));
	EXPECT_EQ(read.GetData(), file.GetData());
	ASSERT_EQ(read.GetScripts().size(), 1u);
	EXPECT_EQ(read.GetScripts()[0].name, "Test");
	EXPECT_EQ(read.GetScripts()[0].filename, "Test.txt");
}
