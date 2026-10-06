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
#include <filesystem>
#include <string>
#include <vector>

#include <LHVMDecompiler.h>
#include <LHVMFile.h>
#include <gtest/gtest.h>

using namespace openblack::lhvm;

namespace
{

/// Builds a small program the way the game's script compiler lays it out
class ProgramBuilder
{
public:
	explicit ProgramBuilder(std::vector<std::string> globals)
	    : _globals(std::move(globals))
	{
	}

	/// Variable number of a global
	[[nodiscard]] uint32_t Global(const std::string& name) const
	{
		return static_cast<uint32_t>(std::ranges::find(_globals, name) - _globals.begin()) + 1;
	}

	/// Variable number of the current script's parameter or local
	[[nodiscard]] uint32_t Local(const std::string& name) const
	{
		const auto& vars = _scripts.back().variables;
		return static_cast<uint32_t>(_globals.size() + (std::ranges::find(vars, name) - vars.begin()) + 1);
	}

	[[nodiscard]] static uint32_t Native(std::string_view name)
	{
		const auto natives = DefaultNativeSignatures();
		return static_cast<uint32_t>(std::ranges::find(natives, name, &NativeSignature::name) - natives.begin());
	}

	[[nodiscard]] uint32_t Here() const { return static_cast<uint32_t>(_code.size()); }

	uint32_t Emit(Opcode code, VMMode mode = VMMode::Immediate, DataType type = DataType::None, VMValue data = {})
	{
		_code.emplace_back(code, mode, type, data, 0);
		return Here() - 1;
	}

	void PushF(float value) { Emit(Opcode::Push, VMMode::Immediate, DataType::Float, VMValue(value)); }
	void PushI(int32_t value) { Emit(Opcode::Push, VMMode::Immediate, DataType::Int, VMValue(value)); }
	void PushB(bool value) { Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, VMValue(value ? 1 : 0)); }
	void Load(uint32_t var) { Emit(Opcode::Push, VMMode::Reference, DataType::Float, VMValue(var)); }
	void Store(uint32_t var) { Emit(Opcode::Pop, VMMode::Reference, DataType::Float, VMValue(var)); }
	void Discard() { Emit(Opcode::Pop, VMMode::Immediate, DataType::Int, VMValue(0)); }
	void Op(Opcode code, DataType type = DataType::Float) { Emit(code, VMMode::Immediate, type); }
	void Sys(std::string_view native) { Emit(Opcode::Sys, VMMode::Immediate, DataType::None, VMValue(Native(native))); }
	void Cast(DataType type) { Emit(Opcode::Cast, VMMode::Cast, type); }
	/// "X = ..." loads and throws away X first
	void Assign(uint32_t var)
	{
		Load(var);
		Discard();
	}

	uint32_t Jz(uint32_t target = 0)
	{
		return Emit(Opcode::Wait, target > Here() || target == 0 ? VMMode::Forward : VMMode::Backward, DataType::Int,
		            VMValue(target));
	}
	uint32_t Jmp(uint32_t target = 0)
	{
		return Emit(Opcode::Jmp, target > Here() || target == 0 ? VMMode::Forward : VMMode::Backward, DataType::Int,
		            VMValue(target));
	}
	void Patch(uint32_t ip, uint32_t target)
	{
		_code[ip].data = VMValue(target);
		_code[ip].mode = target > ip ? VMMode::Forward : VMMode::Backward;
	}

	/// Exception frame of a loop or script: EXCEPT before, the scaffolding after (returns the EXCEPT to finish)
	uint32_t BeginFrame() { return Emit(Opcode::Except, VMMode::Immediate, DataType::Int); }
	/// End of the guarded code; returns the jump over the handlers
	uint32_t EndGuarded(uint32_t except)
	{
		Emit(Opcode::EndExcept, VMMode::EndExcept, DataType::Int);
		const auto jump = Jmp();
		Patch(except, Here());
		return jump;
	}
	void EndFrame(uint32_t jump)
	{
		Emit(Opcode::FailExcept, VMMode::Immediate, DataType::Int);
		Patch(jump, Here());
	}

	void BeginScript(const std::string& name, ScriptType type, std::vector<std::string> params, std::vector<std::string> locals)
	{
		auto variables = params;
		variables.insert(variables.end(), locals.begin(), locals.end());
		_scripts.emplace_back(name, name + ".txt", type, static_cast<uint32_t>(_globals.size()), variables, Here(),
		                      static_cast<uint32_t>(params.size()), static_cast<uint32_t>(_scripts.size() + 1));
		_scriptExcept = BeginFrame();
		for (const auto& param : params)
		{
			Store(Local(param));
		}
	}
	void Start() { Emit(Opcode::EndExcept, VMMode::Yield, DataType::Int); }
	void EndScript()
	{
		const auto jump = EndGuarded(_scriptExcept);
		EndFrame(jump);
		Emit(Opcode::End);
	}

	uint32_t String(const std::string& text)
	{
		const auto offset = static_cast<uint32_t>(_data.size());
		_data.insert(_data.end(), text.begin(), text.end());
		_data.push_back('\0');
		return offset;
	}

	[[nodiscard]] ProgramView View() const
	{
		return {.instructions = _code, .scripts = _scripts, .globalNames = _globals, .data = _data, .autostart = {}};
	}

private:
	std::vector<std::string> _globals;
	std::vector<VMInstruction> _code;
	std::vector<VMScript> _scripts;
	std::vector<char> _data;
	uint32_t _scriptExcept {0};
};

void ExpectAllShown(const DecompiledScript& script)
{
	EXPECT_EQ(script.unaccountedCount, 0);
	for (uint32_t ip = script.firstIp; ip < script.endIp; ++ip)
	{
		EXPECT_TRUE(script.LineForInstruction(ip).has_value()) << ip;
	}
}

} // namespace

TEST(LhvmDecompiler, ExpressionsWithPrecedence)
{
	ProgramBuilder b({"A", "B", "C"});
	b.BeginScript("Maths", ScriptType::Script, {}, {"Local"});
	b.PushF(0.0f); // Local = 0
	b.Store(b.Local("Local"));
	b.Start();
	// A = (B + 2) * 3 - -C / 4
	b.Assign(b.Global("A"));
	b.Load(b.Global("B"));
	b.PushF(2.0f);
	b.Op(Opcode::Add);
	b.PushF(3.0f);
	b.Op(Opcode::Mul);
	b.Load(b.Global("C"));
	b.Op(Opcode::Neg);
	b.PushF(4.0f);
	b.Op(Opcode::Div);
	b.Op(Opcode::Sub);
	b.Store(b.Global("A"));
	// B = A - (C - 1.5)
	b.Assign(b.Global("B"));
	b.Load(b.Global("A"));
	b.Load(b.Global("C"));
	b.PushF(1.5f);
	b.Op(Opcode::Sub);
	b.Op(Opcode::Sub);
	b.Store(b.Global("B"));
	// Local++ and C += A % 2, written without the throw-away load
	b.Load(b.Local("Local"));
	b.PushF(1.0f);
	b.Op(Opcode::Add);
	b.Store(b.Local("Local"));
	b.Load(b.Global("C"));
	b.Load(b.Global("A"));
	b.PushF(2.0f);
	b.Op(Opcode::Mod);
	b.Op(Opcode::Add);
	b.Store(b.Global("C"));
	// A = -5 (a negative constant is a positive one negated)
	b.Assign(b.Global("A"));
	b.PushF(5.0f);
	b.Op(Opcode::Neg);
	b.Store(b.Global("A"));
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	EXPECT_EQ(script.text, "begin script Maths\n"
	                       "\tLocal = 0\n"
	                       "start\n"
	                       "\tA = (B + 2) * 3 - -C / 4\n"
	                       "\tB = A - (C - 1.5)\n"
	                       "\tLocal++\n"
	                       "\tC += A % 2\n"
	                       "\tA = -5\n"
	                       "end script Maths\n");
	ExpectAllShown(script);
	EXPECT_TRUE(script.IsClean());
}

TEST(LhvmDecompiler, IfElsifElse)
{
	ProgramBuilder b({"X", "Y"});
	b.BeginScript("Choose", ScriptType::Help, {"Value"}, {});
	b.Start();
	// if Value == 1 ... elsif Value > 2 and not Y == 0 ... else ... end if
	b.Load(b.Local("Value"));
	b.PushF(1.0f);
	b.Op(Opcode::Eq);
	const auto skip1 = b.Jz();
	b.Assign(b.Global("X"));
	b.PushF(10.0f);
	b.Store(b.Global("X"));
	const auto end1 = b.Jmp();
	b.Patch(skip1, b.Here());
	b.Load(b.Local("Value"));
	b.PushF(2.0f);
	b.Op(Opcode::Gt);
	b.Load(b.Global("Y"));
	b.PushF(0.0f);
	b.Op(Opcode::Eq);
	b.Op(Opcode::Not, DataType::Int);
	b.Op(Opcode::And, DataType::Int);
	const auto skip2 = b.Jz();
	b.Assign(b.Global("X"));
	b.PushF(20.0f);
	b.Store(b.Global("X"));
	b.Patch(end1, b.Here()); // the first branch's jump chains to the second's
	const auto end2 = b.Jmp();
	b.Patch(skip2, b.Here());
	b.PushB(true); // else
	const auto skip3 = b.Jz();
	b.Assign(b.Global("X"));
	b.PushF(30.0f);
	b.Store(b.Global("X"));
	b.Patch(end2, b.Here());
	const auto end3 = b.Jmp();
	b.Patch(skip3, b.Here());
	b.Patch(end3, b.Here());
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	EXPECT_EQ(script.text, "begin help script Choose(Value)\n"
	                       "start\n"
	                       "\tif Value == 1\n"
	                       "\t\tX = 10\n"
	                       "\telsif Value > 2 and not Y == 0\n"
	                       "\t\tX = 20\n"
	                       "\telse\n"
	                       "\t\tX = 30\n"
	                       "\tend if\n"
	                       "end script Choose\n");
	ExpectAllShown(script);
	EXPECT_TRUE(script.IsClean());
}

TEST(LhvmDecompiler, NestedLoopsAndWaits)
{
	ProgramBuilder b({"Count", "Done"});
	b.BeginScript("Loops", ScriptType::Script, {}, {});
	b.Start();
	// while Count < 10 / while Done == 0 / wait until read / Done = 1 / end while / Count++ / end while
	const auto outer = b.BeginFrame();
	const auto outerTop = b.Here();
	b.Load(b.Global("Count"));
	b.PushF(10.0f);
	b.Op(Opcode::Lt);
	const auto outerExit = b.Jz();
	const auto inner = b.BeginFrame();
	const auto innerTop = b.Here();
	b.Load(b.Global("Done"));
	b.PushF(0.0f);
	b.Op(Opcode::Eq);
	const auto innerExit = b.Jz();
	const auto wait = b.Here();
	b.Sys("TEXT_READ");
	b.Jz(wait);
	b.Assign(b.Global("Done"));
	b.PushF(1.0f);
	b.Store(b.Global("Done"));
	b.Jmp(innerTop);
	b.Patch(innerExit, b.Here());
	b.EndFrame(b.EndGuarded(inner));
	b.Load(b.Global("Count"));
	b.PushF(1.0f);
	b.Op(Opcode::Add);
	b.Store(b.Global("Count"));
	b.Jmp(outerTop);
	b.Patch(outerExit, b.Here());
	b.EndFrame(b.EndGuarded(outer));
	// wait 2.5 seconds
	const auto sleep = b.Here();
	b.PushF(2.5f);
	b.Op(Opcode::Sleep);
	b.Jz(sleep);
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	EXPECT_EQ(script.text, "begin script Loops\n"
	                       "start\n"
	                       "\twhile Count < 10\n"
	                       "\t\twhile Done == 0\n"
	                       "\t\t\twait until read\n"
	                       "\t\t\tDone = 1\n"
	                       "\t\tend while\n"
	                       "\t\tCount++\n"
	                       "\tend while\n"
	                       "\twait 2.5 seconds\n"
	                       "end script Loops\n");
	ExpectAllShown(script);
	EXPECT_TRUE(script.IsClean());
}

TEST(LhvmDecompiler, LoopWithExceptionsAndDialogue)
{
	ProgramBuilder b({"Highlight"});
	b.BeginScript("Notify", ScriptType::ChallengeHelp, {}, {});
	b.Start();
	// begin loop / begin dialogue / say single line 5 / end dialogue / until Highlight clicked / end loop
	const auto frame = b.BeginFrame();
	const auto top = b.Here();
	const auto dialogue = b.Here();
	b.Sys("START_DIALOGUE");
	b.Jz(dialogue);
	b.PushB(true);
	b.PushI(5);
	b.PushI(0);
	b.Sys("RUN_TEXT");
	b.Sys("END_DIALOGUE");
	b.Jmp(top);
	b.Patch(frame, b.Here());
	b.Load(b.Global("Highlight"));
	b.Sys("GAME_THING_CLICKED");
	const auto next = b.Jz();
	b.PushB(false);
	b.Sys("SET_WIDESCREEN");
	b.Sys("END_GAME_SPEED");
	b.Sys("END_DIALOGUE");
	b.Sys("END_CAMERA_CONTROL");
	b.Emit(Opcode::BrkExcept, VMMode::Immediate, DataType::Int);
	const auto leave = b.Jmp();
	b.Patch(next, b.Here());
	b.Emit(Opcode::FailExcept, VMMode::Immediate, DataType::Int);
	b.Patch(leave, b.Here());
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	EXPECT_EQ(script.text, "begin challenge help script Notify\n"
	                       "start\n"
	                       "\tbegin loop\n"
	                       "\t\tbegin dialogue\n"
	                       "\t\t\tsay single line 5\n"
	                       "\t\tend dialogue\n"
	                       "\t\tuntil Highlight clicked\n"
	                       "\tend loop\n"
	                       "end script Notify\n");
	ExpectAllShown(script);
	EXPECT_TRUE(script.IsClean());
}

TEST(LhvmDecompiler, NativeCallsAndRunScript)
{
	ProgramBuilder b({"Boy", "Girl", "Speak"});
	b.BeginScript("Helper", ScriptType::Script, {"A", "B"}, {});
	b.Start();
	b.EndScript();
	b.BeginScript("Calls", ScriptType::Script, {}, {});
	b.Start();
	// move Boy position to [Girl] radius 5
	b.Load(b.Global("Boy"));
	b.Load(b.Global("Girl"));
	b.Sys("GET_POSITION");
	b.PushF(5.0f);
	b.Sys("MOVE_GAME_THING");
	// set Boy position to [10, 20]  (a ground position: height zero swapped into place)
	b.Load(b.Global("Boy"));
	b.PushF(10.0f);
	b.Cast(DataType::Vector);
	b.PushF(20.0f);
	b.Cast(DataType::Vector);
	b.Emit(Opcode::Push, VMMode::Immediate, DataType::Vector, VMValue(0.0f));
	b.Emit(Opcode::Swap, VMMode::Immediate, DataType::Int);
	b.Sys("SET_POSITION");
	// say constant Speak
	b.PushB(false);
	b.Load(b.Global("Speak"));
	b.Cast(DataType::Int);
	b.PushI(0);
	b.Sys("RUN_TEXT");
	// stop script "Other"
	b.PushI(static_cast<int32_t>(b.String("Other")));
	b.Sys("STOP_SCRIPT");
	// Girl = number from 1 to variable 3
	b.Assign(b.Global("Girl"));
	b.PushF(1.0f);
	b.PushI(3);
	b.Cast(DataType::Float);
	b.Sys("RANDOM");
	b.Store(b.Global("Girl"));
	// run script Helper(1, Boy) / run background script Helper(2, Girl)
	b.PushF(1.0f);
	b.Load(b.Global("Boy"));
	b.Emit(Opcode::Run, VMMode::Sync, DataType::Int, VMValue(1));
	b.PushF(2.0f);
	b.Load(b.Global("Girl"));
	b.Emit(Opcode::Run, VMMode::Async, DataType::Int, VMValue(1));
	b.EndScript();

	const auto script = DecompileScript(b.View(), 1);
	EXPECT_EQ(script.text, "begin script Calls\n"
	                       "start\n"
	                       "\tmove Boy position to [Girl] radius 5\n"
	                       "\tset Boy position to [10, 20]\n"
	                       "\tsay constant Speak\n"
	                       "\tstop script \"Other\"\n"
	                       "\tGirl = number from 1 to variable 3\n"
	                       "\trun script Helper(1, Boy)\n"
	                       "\trun background script Helper(2, Girl)\n"
	                       "end script Calls\n");
	ExpectAllShown(script);
	EXPECT_TRUE(script.IsClean());
	EXPECT_EQ(script.nativeCallCount, 0);

	const auto helper = DecompileScript(b.View(), 0);
	EXPECT_EQ(helper.text, "begin script Helper(A, B)\nstart\nend script Helper\n");
}

TEST(LhvmDecompiler, ConstantsByName)
{
	chl::ConstantTable constants;
	constants.LoadHeader("enum SCRIPT_OBJECT_TYPE { SCRIPT_OBJECT_TYPE_NONE, SCRIPT_OBJECT_TYPE_MARKER, "
	                     "SCRIPT_OBJECT_TYPE_ABODE };");
	ProgramBuilder b({"Hut"});
	b.BeginScript("Find", ScriptType::Script, {}, {});
	b.Start();
	// Hut = get SCRIPT_OBJECT_TYPE_ABODE at [1, 2, 3]
	b.Assign(b.Global("Hut"));
	b.PushI(2);
	b.PushI(5000);
	for (const float v : {1.0f, 2.0f, 3.0f})
	{
		b.PushF(v);
		b.Cast(DataType::Vector);
	}
	b.PushB(false);
	b.Sys("CALL");
	b.Store(b.Global("Hut"));
	b.EndScript();

	const auto plain = DecompileScript(b.View(), 0);
	EXPECT_NE(plain.text.find("\tHut = get 2 at [1, 2, 3]\n"), std::string::npos) << plain.text;
	const auto named = DecompileScript(b.View(), 0, {.constants = &constants});
	EXPECT_NE(named.text.find("\tHut = get SCRIPT_OBJECT_TYPE_ABODE at [1, 2, 3]\n"), std::string::npos) << named.text;
}

TEST(LhvmDecompiler, GotoFallback)
{
	ProgramBuilder b({"X"});
	b.BeginScript("Tangled", ScriptType::Script, {}, {});
	b.Start();
	// A jump into the middle of a loop has no structured form
	const auto into = b.Jmp();
	const auto frame = b.BeginFrame();
	const auto top = b.Here();
	b.Load(b.Global("X"));
	b.PushF(3.0f);
	b.Op(Opcode::Lt);
	const auto exit = b.Jz();
	b.Assign(b.Global("X"));
	b.PushF(1.0f);
	b.Store(b.Global("X"));
	b.Patch(into, b.Here());
	b.Load(b.Global("X"));
	b.PushF(2.0f);
	b.Op(Opcode::Add);
	b.Store(b.Global("X"));
	b.Jmp(top);
	b.Patch(exit, b.Here());
	b.EndFrame(b.EndGuarded(frame));
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	EXPECT_GT(script.gotoCount, 0);
	EXPECT_FALSE(script.IsClean());
	EXPECT_NE(script.text.find("\tgoto label_"), std::string::npos) << script.text;
	ExpectAllShown(script);
	// The label the goto names is written where the jump lands
	const auto gotoAt = script.text.find("goto label_");
	const auto name = script.text.substr(gotoAt + 5, script.text.find('\n', gotoAt) - gotoAt - 5);
	EXPECT_NE(script.text.find(name + ":\n"), std::string::npos) << script.text;
}

TEST(LhvmDecompiler, ChallengeNamedWhereTheGrammarAllows)
{
	ProgramBuilder b({"Obj"});
	b.BeginScript("Lit", ScriptType::Script, {}, {"Highlight"});
	// Highlight = create highlight 3 at [Obj], for challenge 7, among the locals
	b.PushI(3);
	b.Load(b.Global("Obj"));
	b.Sys("GET_POSITION");
	b.PushI(7);
	b.Sys("CREATE_HIGHLIGHT");
	b.Store(b.Local("Highlight"));
	b.Start();
	// A second highlight for challenge 8, in the body
	b.Assign(b.Local("Highlight"));
	b.PushI(4);
	b.Load(b.Global("Obj"));
	b.Sys("GET_POSITION");
	b.PushI(8);
	b.Sys("CREATE_HIGHLIGHT");
	b.Store(b.Local("Highlight"));
	b.EndScript();

	// On its own, the script is a file: the first challenge goes above "begin script"
	const auto alone = DecompileScript(b.View(), 0);
	EXPECT_EQ(alone.text, "challenge 7\n"
	                      "begin script Lit\n"
	                      "\tHighlight = create highlight 3 at [Obj]\n"
	                      "start\n"
	                      "\tchallenge 8\n"
	                      "\tHighlight = create highlight 4 at [Obj]\n"
	                      "end script Lit\n");
	EXPECT_EQ(alone.leadingChallenge, "7");
	EXPECT_EQ(alone.challenge, 8);
	ExpectAllShown(alone);

	// With the challenge already named by the file, nothing is repeated
	const auto inFile = DecompileScript(b.View(), 0, {.challenge = 7, .standalone = false});
	EXPECT_EQ(inFile.text.find("challenge 7"), std::string::npos) << inFile.text;
	EXPECT_TRUE(inFile.leadingChallenge.empty());

	// As a file: the challenge, then the globals, then the scripts
	const auto program = DecompileAll(b.View());
	ASSERT_EQ(program.files.size(), 1);
	EXPECT_EQ(program.files[0].header, "challenge 7\nglobal Obj\n");
	EXPECT_EQ(program.Text().find("challenge 7"), 0);
	EXPECT_EQ(program.Text().find("challenge 7", 1), std::string::npos) << program.Text();
}

TEST(LhvmDecompiler, LineMapAndControlFlowGraph)
{
	ProgramBuilder b({"X"});
	b.BeginScript("Map", ScriptType::Script, {}, {});
	b.Start();
	const auto assign = b.Here();
	b.Assign(b.Global("X"));
	b.PushF(7.0f);
	b.Store(b.Global("X"));
	b.EndScript();

	const auto script = DecompileScript(b.View(), 0);
	ASSERT_EQ(script.text, "begin script Map\nstart\n\tX = 7\nend script Map\n");
	ASSERT_EQ(script.lines.size(), 4);
	EXPECT_EQ(script.LineForInstruction(assign), 2);
	EXPECT_EQ(script.LineForInstruction(assign + 3), 2);
	EXPECT_EQ(script.LineForInstruction(script.firstIp), 0);
	EXPECT_EQ(script.LineForInstruction(script.endIp - 1), 3);
	EXPECT_FALSE(script.LineForInstruction(script.endIp).has_value());

	const auto graph = BuildControlFlowGraph(b.View().instructions, script.firstIp, script.endIp);
	ASSERT_FALSE(graph.blocks.empty());
	EXPECT_TRUE(graph.blocks.front().handler.has_value());
	EXPECT_EQ(graph.BlockAt(assign), graph.BlockAt(assign + 1));
	EXPECT_TRUE(std::ranges::all_of(graph.blocks, [](const auto& block) { return block.reachable; }));
}

TEST(LhvmDecompiler, ChallengeChlSmoke)
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		GTEST_SKIP() << "OPENBLACK_GAME_PATH is not set";
	}
	const auto path = std::filesystem::path(gamePath) / "Scripts" / "Quests" / "challenge.chl";
	if (!std::filesystem::exists(path))
	{
		GTEST_SKIP() << "No " << path;
	}
	LHVMFile file;
	file.Open(path);
	ASSERT_TRUE(file.IsLoaded());
	const auto program = DecompileAll(ProgramView::From(file));
	EXPECT_EQ(program.stats.scripts, file.GetScripts().size());
	EXPECT_EQ(program.stats.instructions, file.GetInstructions().size());
	EXPECT_EQ(program.stats.unaccountedInstructions, 0);
	EXPECT_EQ(program.stats.unsupportedOpcodes, 0);
	EXPECT_EQ(program.stats.clean, program.stats.scripts);
	EXPECT_EQ(program.stats.nativeCalls, 0);
	EXPECT_NE(program.Text().find("begin help script StandardReminder(AdvisorSpeak)"), std::string::npos);
}
