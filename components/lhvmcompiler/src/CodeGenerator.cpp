/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CodeGenerator.h"

#include <algorithm>
#include <array>
#include <bit>

#include <fmt/format.h>

#include "FormTable.h"

namespace openblack::lhvm::chl
{

namespace
{

/// The conditional jump: openblack names opcode 1 "Wait"
constexpr Opcode k_JumpIfZero = Opcode::Wait;

/// VILLAGER_STATE_SCRIPT_PLAY_ANIM in the game's script headers, for compiling "play" without headers
constexpr int32_t k_PlayAnimationState = 200;

} // namespace

namespace
{

/// Natives the original compiler calls with the second form of the call instruction
constexpr auto k_SecondFormNatives = std::to_array<std::string_view>(
    {"SET_PROPERTY", "RANDOM_ULONG", "GET_ALIGNMENT", "GET_MOON_PERCENTAGE", "SET_INTERFACE_INTERACTION", "SET_GAMESPEED"});

[[nodiscard]] DataType ToDataType(ArgType type)
{
	switch (type)
	{
	case ArgType::Int:
	case ArgType::String:
		return DataType::Int;
	case ArgType::Coord:
		return DataType::Vector;
	case ArgType::Object:
		return DataType::Object;
	case ArgType::Bool:
		return DataType::Boolean;
	default:
		return DataType::Float;
	}
}

/// The type an expression has before any conversion
[[nodiscard]] ArgType NaturalType(ValueType type)
{
	switch (type)
	{
	case ValueType::Int:
		return ArgType::Int;
	case ValueType::Bool:
		return ArgType::Bool;
	case ValueType::Object:
		return ArgType::Object;
	case ValueType::Vector:
		return ArgType::Coord;
	case ValueType::String:
		return ArgType::String;
	default:
		return ArgType::Float;
	}
}

[[nodiscard]] Opcode ArithmeticOpcode(Op op)
{
	switch (op)
	{
	case Op::Add:
		return Opcode::Add;
	case Op::Sub:
		return Opcode::Sub;
	case Op::Mul:
		return Opcode::Mul;
	case Op::Div:
		return Opcode::Div;
	case Op::Mod:
		return Opcode::Mod;
	case Op::Eq:
		return Opcode::Eq;
	case Op::Ne:
		return Opcode::Ne;
	case Op::Lt:
		return Opcode::Lt;
	case Op::Le:
		return Opcode::Le;
	case Op::Gt:
		return Opcode::Gt;
	case Op::Ge:
		return Opcode::Ge;
	case Op::And:
		return Opcode::And;
	case Op::Or:
		return Opcode::Or;
	default:
		return Opcode::End;
	}
}

[[nodiscard]] std::optional<Op> CompoundOperator(std::string_view op)
{
	if (op == "+=" || op == "++")
	{
		return Op::Add;
	}
	if (op == "-=" || op == "--")
	{
		return Op::Sub;
	}
	if (op == "*=")
	{
		return Op::Mul;
	}
	if (op == "/=")
	{
		return Op::Div;
	}
	if (op == "%=")
	{
		return Op::Mod;
	}
	return std::nullopt;
}

} // namespace

CodeGenerator::CodeGenerator(const ParseEnvironment& environment, DiagnosticSink& sink, uint32_t firstScriptId)
    : _env(environment)
    , _sink(sink)
    , _nextScriptId(firstScriptId)
{
}

void CodeGenerator::Seed(std::vector<VMInstruction> instructions, std::vector<VMScript> scripts, std::vector<char> data,
                         std::vector<uint32_t> autostart, size_t globalCount)
{
	_code = std::move(instructions);
	_scripts = std::move(scripts);
	_data = std::move(data);
	_autostart = std::move(autostart);
	_globalCount = globalCount;
	for (const auto& script : _scripts)
	{
		_nextScriptId = std::max(_nextScriptId, script.scriptId + 1);
	}
}

void CodeGenerator::Error(SourceLocation location, std::string message)
{
	_sink.Error(_file, location, std::move(message));
}

// ------------------------------------------------------------------------------------------------------------ program

void CodeGenerator::Add(const ParsedFile& file)
{
	_file = file.fileName;
	for (const auto& item : file.items)
	{
		if (std::holds_alternative<GlobalDeclaration>(item))
		{
			++_globalCount;
		}
		else if (const auto* autorun = std::get_if<AutorunDeclaration>(&item))
		{
			_autoruns.push_back({.script = autorun->script, .file = _file, .location = autorun->location});
		}
		else if (const auto* definition = std::get_if<ScriptDefinition>(&item))
		{
			_defined[definition->name] = definition->params.size();
		}
		else if (const auto* script = std::get_if<ParsedScript>(&item))
		{
			CompileScript(*script, file.fileName);
		}
	}
}

bool CodeGenerator::Finish()
{
	bool ok = true;
	const auto find = [this](const std::string& name) -> const VMScript* {
		// The last script of that name wins, so recompiled scripts replace older ones
		const VMScript* found = nullptr;
		for (const auto& script : _scripts)
		{
			if (script.name == name)
			{
				found = &script;
			}
		}
		return found;
	};
	for (const auto& call : _calls)
	{
		const auto* script = find(call.script);
		if (script == nullptr)
		{
			_sink.Error(call.file, call.location, fmt::format("cannot run script \"{}\": it doesn't exist", call.script));
			ok = false;
			continue;
		}
		if (script->parameterCount != call.argumentCount)
		{
			_sink.Error(
			    call.file, call.location,
			    fmt::format("script {} takes {} arguments, not {}", call.script, script->parameterCount, call.argumentCount));
			ok = false;
		}
		_code[call.address].data = VMValue(script->scriptId);
	}
	for (const auto& autorun : _autoruns)
	{
		const auto* script = find(autorun.script);
		if (script == nullptr)
		{
			_sink.Error(autorun.file, autorun.location,
			            fmt::format("cannot autostart script \"{}\": it doesn't exist", autorun.script));
			ok = false;
			continue;
		}
		if (script->parameterCount != 0)
		{
			_sink.Error(autorun.file, autorun.location,
			            fmt::format("script {} takes arguments, so it can't be started automatically", autorun.script));
			ok = false;
		}
		_autostart.push_back(script->scriptId);
	}
	_calls.clear();
	_autoruns.clear();
	return ok;
}

std::optional<uint32_t> CodeGenerator::VariableId(const std::string& name) const
{
	if (const auto it = std::ranges::find(_localNames, name); it != _localNames.end())
	{
		return _variablesOffset + 1 + static_cast<uint32_t>(it - _localNames.begin());
	}
	const auto& globals = _env.Globals();
	if (const auto it = std::ranges::find(globals, name); it != globals.end())
	{
		return 1 + static_cast<uint32_t>(it - globals.begin());
	}
	return std::nullopt;
}

void CodeGenerator::CompileScript(const ParsedScript& parsed, const std::string& file)
{
	const auto& script = *parsed.script;
	_map = &parsed.sourceMap;
	_file = file;
	_localNames = script.params;
	for (const auto& local : script.locals)
	{
		_localNames.push_back(local->name);
	}
	_variablesOffset = static_cast<uint32_t>(_globalCount);
	_labelAddresses.clear();
	_gotos.clear();

	VMScript record;
	record.name = script.name;
	record.filename = file;
	record.type = static_cast<ScriptType>(1u << static_cast<uint32_t>(script.kind));
	record.variablesOffset = _variablesOffset;
	record.variables = _localNames;
	record.instructionAddress = Here();
	record.parameterCount = static_cast<uint32_t>(script.params.size());
	record.scriptId = _nextScriptId++;

	// Exception frame of the whole script, then the parameters and the locals
	SetLine(_map->begin);
	const auto except = EmitPlaceholder(Opcode::Except);
	for (const auto& param : script.params)
	{
		Emit(Opcode::Pop, VMMode::Reference, DataType::Float, *VariableId(param));
	}
	for (const auto& local : script.locals)
	{
		SetLine(*local);
		EmitValue(*local->expr, ArgType::Float);
		_line = _map->EndLine(local.get());
		Emit(Opcode::Pop, VMMode::Reference, DataType::Float, *VariableId(local->name));
	}
	_line = 0;
	Emit(Opcode::EndExcept, VMMode::Yield, DataType::Int, 0);

	EmitStatements(script.body);

	// The frame's exit: emitted when the parser sees what follows the body (a handler, or "end script")
	_line = script.handlers.empty() ? _map->end.line : _map->Of(script.handlers.front().get()).line;
	Emit(Opcode::EndExcept, VMMode::EndExcept, DataType::Int, 0);
	std::vector<uint32_t> exits {EmitPlaceholder(Opcode::Jmp)};
	Patch(except, Here());
	EmitHandlers(script.handlers, exits, _map->end.line);
	SetLine(_map->end);
	Emit(Opcode::FailExcept, VMMode::Immediate, DataType::Int, 0);
	_line = 0;
	for (const auto exit : exits)
	{
		Patch(exit, Here());
	}
	_line = 0;
	Emit(Opcode::End, VMMode::Immediate, DataType::None, 0);

	for (const auto& [address, label] : _gotos)
	{
		const auto it = _labelAddresses.find(label);
		if (it == _labelAddresses.end())
		{
			Error(_map->begin, fmt::format("goto to unknown label {}", label));
			continue;
		}
		_code[address].data = VMValue(it->second);
		_code[address].mode = it->second > address ? VMMode::Forward : VMMode::Backward;
	}

	_scripts.push_back(std::move(record));
	_map = nullptr;
}

// --------------------------------------------------------------------------------------------------------- statements

void CodeGenerator::EmitStatements(const StmtList& list)
{
	for (const auto& stmt : list)
	{
		EmitStatement(*stmt);
	}
}

void CodeGenerator::EmitStatement(const Stmt& stmt)
{
	SetLine(stmt);
	switch (stmt.kind)
	{
	case StmtKind::Expression:
		if (!stmt.op.empty())
		{
			EmitCompound(stmt);
		}
		else
		{
			EmitCallStatement(stmt);
		}
		break;
	case StmtKind::Assign:
		if (stmt.expr != nullptr && stmt.name.empty() && stmt.expr->kind == ExprKind::NativeCall &&
		    stmt.expr->text == "SET_PROPERTY")
		{
			EmitPropertyAssign(stmt);
		}
		else
		{
			EmitAssign(stmt);
		}
		break;
	case StmtKind::RunScript:
	{
		for (const auto& arg : stmt.args)
		{
			EmitValue(*arg, ArgType::Float);
		}
		_line = _map->EndLine(&stmt);
		const auto address = Emit(Opcode::Run, stmt.async ? VMMode::Async : VMMode::Sync, DataType::Int, 0);
		_calls.push_back({.address = address,
		                  .script = stmt.name,
		                  .argumentCount = stmt.args.size(),
		                  .file = _file,
		                  .location = _map->Of(&stmt)});
		break;
	}
	case StmtKind::WaitUntil:
	{
		const auto top = Here();
		EmitValue(*stmt.expr, ArgType::Bool);
		_line = _map->EndLine(&stmt);
		EmitJump(k_JumpIfZero, top);
		break;
	}
	case StmtKind::If:
		EmitIf(stmt);
		break;
	case StmtKind::While:
		EmitLoop(stmt);
		break;
	case StmtKind::Block:
		EmitBlock(stmt);
		break;
	case StmtKind::Goto:
		_gotos.emplace_back(Emit(Opcode::Jmp, VMMode::Forward, DataType::Int, 0), stmt.name);
		break;
	case StmtKind::Label:
		_labelAddresses[stmt.name] = Here();
		break;
	default:
		Error(_map->Of(&stmt), "this statement can't be compiled");
		break;
	}
}

void CodeGenerator::EmitAssign(const Stmt& stmt)
{
	const auto id = VariableId(stmt.name);
	if (!id)
	{
		Error(_map->Of(&stmt), fmt::format("unknown variable {}", stmt.name));
		return;
	}
	Emit(Opcode::Push, VMMode::Reference, DataType::Float, *id);
	if (stmt.op == "=")
	{
		Emit(Opcode::Pop, VMMode::Immediate, DataType::Int, 0);
		EmitValue(*stmt.expr, ArgType::Float);
	}
	else
	{
		const auto op = CompoundOperator(stmt.op);
		if (stmt.expr != nullptr)
		{
			EmitValue(*stmt.expr, ArgType::Float);
		}
		else
		{
			_line = _map->EndLine(&stmt);
			EmitFloat(Opcode::Push, VMMode::Immediate, DataType::Float, 1.0f);
		}
		_line = _map->EndLine(&stmt);
		Emit(ArithmeticOpcode(op.value_or(Op::Add)), VMMode::Immediate, DataType::Float, 0);
	}
	_line = _map->EndLine(&stmt);
	Emit(Opcode::Pop, VMMode::Reference, DataType::Float, *id);
}

void CodeGenerator::EmitPropertyAssign(const Stmt& stmt)
{
	// The property and object are pushed twice: once for reading the old value, once for writing the new one
	const auto& call = *stmt.expr;
	for (int i = 0; i < 2; ++i)
	{
		EmitValue(*call.args[0], ArgType::Int);
		EmitValue(*call.args[1], ArgType::Object);
	}
	EmitSys("GET_PROPERTY", true);
	if (stmt.op == "=")
	{
		Emit(Opcode::Pop, VMMode::Immediate, DataType::Int, 0);
		EmitValue(*call.args[2], ArgType::Float);
	}
	else
	{
		if (call.args[2] != nullptr)
		{
			EmitValue(*call.args[2], ArgType::Float);
		}
		else
		{
			_line = _map->EndLine(&stmt);
			EmitFloat(Opcode::Push, VMMode::Immediate, DataType::Float, 1.0f);
		}
		_line = _map->EndLine(&stmt);
		Emit(ArithmeticOpcode(CompoundOperator(stmt.op).value_or(Op::Add)), VMMode::Immediate, DataType::Float, 0);
	}
	_line = _map->EndLine(&stmt);
	EmitSys("SET_PROPERTY", true);
}

void CodeGenerator::EmitCallStatement(const Stmt& stmt)
{
	const auto& call = *stmt.expr;
	const auto result = EmitValue(call, ArgType::Any);
	_line = _map->EndLine(&stmt);
	if (call.kind == ExprKind::NativeCall && call.text == "OBJECT_DELETE" && !call.args.empty() &&
	    call.args[0]->kind == ExprKind::Variable)
	{
		// Deleting an object held in a variable also clears the variable
		if (const auto id = VariableId(call.args[0]->text))
		{
			Emit(Opcode::Cast, VMMode::Zero, DataType::Float, *id);
		}
	}
	switch (result)
	{
	case ArgType::None:
		break;
	case ArgType::Object:
		Emit(Opcode::Pop, VMMode::Immediate, DataType::Object, 0);
		break;
	default:
		Emit(Opcode::Pop, VMMode::Immediate, DataType::Float, 0);
		break;
	}
}

void CodeGenerator::EmitCompound(const Stmt& stmt)
{
	const auto& op = stmt.op;
	const auto variable = [this, &stmt]() -> std::optional<uint32_t> {
		const auto& object = *stmt.args[0];
		const auto id = object.kind == ExprKind::Variable ? VariableId(object.text) : std::nullopt;
		if (!id)
		{
			Error(_map->Of(&stmt), fmt::format("\"{}\" needs a variable", stmt.op));
		}
		return id;
	};
	if (op == "play")
	{
		// The object is pushed as a number for the state and as an object for the animation
		const auto id = variable();
		if (!id)
		{
			return;
		}
		_line = _map->EndLine(stmt.args[0].get());
		Emit(Opcode::Push, VMMode::Reference, DataType::Float, *id);
		Emit(Opcode::Push, VMMode::Reference, DataType::Object, *id);
		EmitValue(*stmt.args[1], ArgType::Int);
		EmitValue(*stmt.args[2], ArgType::Float);
		_line = _map->EndLine(&stmt);
		Emit(Opcode::Cast, VMMode::Cast, DataType::Int, 0);
		EmitSys("SET_SCRIPT_ULONG");
		// The villager state that plays the animation, as the headers name it, or its number in the game's headers
		const auto* constants = _env.Constants();
		const auto playAnimation = constants != nullptr ? constants->ValueOf("VILLAGER_STATE_SCRIPT_PLAY_ANIM") : std::nullopt;
		Emit(Opcode::Push, VMMode::Immediate, DataType::Int,
		     static_cast<uint32_t>(playAnimation.value_or(k_PlayAnimationState)));
		EmitSys("SET_SCRIPT_STATE");
		return;
	}
	if (op == "state")
	{
		// The state's settings are made before the state itself, each on the object read as an integer
		const auto id = variable();
		if (!id)
		{
			return;
		}
		_line = _map->EndLine(stmt.args[0].get());
		Emit(Opcode::Push, VMMode::Reference, DataType::Float, *id);
		EmitValue(*stmt.args[1], ArgType::Int);
		for (const auto& function : stmt.body)
		{
			SetLine(*function);
			Emit(Opcode::Push, VMMode::Reference, DataType::Int, *id);
			if (function->op == "position")
			{
				EmitValue(*function->args[0], ArgType::Coord);
				_line = _map->EndLine(function.get());
				EmitSys("SET_SCRIPT_STATE_POS");
			}
			else if (function->op == "float")
			{
				EmitValue(*function->args[0], ArgType::Float);
				_line = _map->EndLine(function.get());
				EmitSys("SET_SCRIPT_FLOAT");
			}
			else
			{
				EmitValue(*function->args[0], ArgType::Float);
				Emit(Opcode::Cast, VMMode::Cast, DataType::Int, 0);
				EmitValue(*function->args[1], ArgType::Float);
				_line = _map->EndLine(function.get());
				Emit(Opcode::Cast, VMMode::Cast, DataType::Int, 0);
				EmitSys("SET_SCRIPT_ULONG");
			}
		}
		// Emitted once the parser has seen the token after the settings
		if (const auto it = _map->closings.find(&stmt); it != _map->closings.end())
		{
			_line = it->second.line;
		}
		EmitSys("SET_SCRIPT_STATE");
		return;
	}
	if (op == "set camera to" || op == "move camera to")
	{
		// The camera's focus and position, both converted from the camera enum
		EmitValue(*stmt.args[0], ArgType::Int);
		EmitSys("CONVERT_CAMERA_FOCUS");
		EmitValue(*stmt.args[0], ArgType::Int);
		EmitSys("CONVERT_CAMERA_POSITION");
		if (op == "set camera to")
		{
			_line = _map->EndLine(&stmt);
			EmitSys("SET_CAMERA_POSITION");
			EmitSys("SET_CAMERA_FOCUS");
			return;
		}
		// The time is copied for the second call
		EmitValue(*stmt.args[1], ArgType::Float);
		_line = _map->EndLine(&stmt);
		Emit(Opcode::Swap, VMMode::CopyTo, DataType::Float, 4);
		EmitSys("MOVE_CAMERA_POSITION");
		EmitSys("MOVE_CAMERA_FOCUS");
		return;
	}
	Error(_map->Of(&stmt), fmt::format("unknown statement \"{}\"", op));
}

void CodeGenerator::EmitIf(const Stmt& stmt)
{
	// Each branch ends with a jump. When the next branch's body ends, the jump is pointed at the next branch's own
	// jump, so they chain; "end if" points the last one, and the last test, at the end.
	const auto closing = _map->closings.contains(&stmt) ? _map->closings.at(&stmt).line : _map->EndLine(&stmt);
	const auto lineOf = [&](size_t index) {
		if (index >= stmt.branches.size())
		{
			return closing;
		}
		const auto it = _map->branches.find(&stmt.branches[index]);
		return it != _map->branches.end() ? it->second.line : _map->Of(&stmt).line;
	};
	std::optional<uint32_t> pendingTest;
	std::optional<uint32_t> pendingJump;
	for (size_t i = 0; i < stmt.branches.size(); ++i)
	{
		const auto& branch = stmt.branches[i];
		if (i > 0)
		{
			_line = lineOf(i);
			Patch(*pendingTest, Here());
		}
		if (branch.cond != nullptr)
		{
			EmitValue(*branch.cond, ArgType::Bool);
			_line = _map->EndLine(branch.cond.get());
		}
		else
		{
			_line = lineOf(i);
			Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, 1);
		}
		pendingTest = EmitPlaceholder(k_JumpIfZero);
		EmitStatements(branch.body);
		_line = lineOf(i + 1);
		const auto jump = Here();
		if (pendingJump)
		{
			Patch(*pendingJump, jump);
		}
		EmitPlaceholder(Opcode::Jmp);
		pendingJump = jump;
	}
	_line = closing;
	if (pendingJump)
	{
		Patch(*pendingJump, Here());
	}
	if (pendingTest)
	{
		Patch(*pendingTest, Here());
	}
}

void CodeGenerator::EmitLoop(const Stmt& stmt)
{
	const bool isWhile = stmt.expr != nullptr;
	const auto closing = _map->closings.contains(&stmt) ? _map->closings.at(&stmt).line : _map->EndLine(&stmt);
	// What follows the body: the first handler, or "end"
	const auto afterBody = stmt.handlers.empty() ? closing : _map->Of(stmt.handlers.front().get()).line;
	const auto except = EmitPlaceholder(Opcode::Except);
	const auto top = Here();
	std::optional<uint32_t> test;
	if (isWhile)
	{
		EmitValue(*stmt.expr, ArgType::Bool);
		_line = _map->EndLine(stmt.expr.get());
		test = EmitPlaceholder(k_JumpIfZero);
	}
	EmitStatements(stmt.body);
	_line = afterBody;
	EmitJump(Opcode::Jmp, top);
	std::vector<uint32_t> exits;
	if (isWhile)
	{
		Patch(*test, Here());
		Emit(Opcode::EndExcept, VMMode::EndExcept, DataType::Int, 0);
		exits.push_back(EmitPlaceholder(Opcode::Jmp));
	}
	Patch(except, Here());
	EmitHandlers(stmt.handlers, exits, closing);
	_line = closing;
	Emit(Opcode::FailExcept, VMMode::Immediate, DataType::Int, 0);
	// The exits are pointed past the frame with no line
	const auto saved = _line;
	_line = 0;
	for (const auto exit : exits)
	{
		Patch(exit, Here());
	}
	_line = saved;
}

void CodeGenerator::EmitHandlers(const StmtList& handlers, std::vector<uint32_t>& exits, uint32_t afterLine)
{
	for (size_t i = 0; i < handlers.size(); ++i)
	{
		const auto& handler = handlers[i];
		SetLine(*handler);
		EmitValue(*handler->expr, ArgType::Bool);
		_line = _map->EndLine(handler->expr.get());
		const auto skip = EmitPlaceholder(k_JumpIfZero);
		if (handler->kind == StmtKind::Until)
		{
			Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, 0);
			EmitSys("SET_WIDESCREEN");
			EmitSys("END_GAME_SPEED");
			EmitSys("END_DIALOGUE");
			EmitSys("END_CAMERA_CONTROL");
		}
		EmitStatements(handler->body);
		_line = i + 1 < handlers.size() ? _map->Of(handlers[i + 1].get()).line : afterLine;
		if (handler->kind == StmtKind::Until)
		{
			Emit(Opcode::BrkExcept, VMMode::Immediate, DataType::Int, 0);
			exits.push_back(EmitPlaceholder(Opcode::Jmp));
		}
		Patch(skip, Here());
	}
}

void CodeGenerator::EmitBlock(const Stmt& stmt)
{
	const auto& name = stmt.name;
	const auto retry = [this](std::string_view native) {
		const auto top = Here();
		EmitSys(native);
		EmitJump(k_JumpIfZero, top);
	};
	const auto closing = [&]() {
		if (const auto it = _map->closings.find(&stmt); it != _map->closings.end())
		{
			SetLine(it->second);
		}
	};
	if (name == "known cinema" || name == "known dialogue")
	{
		EmitStatements(stmt.body);
		return;
	}
	if (name == "cinema" || name == "camera")
	{
		const bool cinema = name == "cinema";
		retry("START_CAMERA_CONTROL");
		retry("START_DIALOGUE");
		EmitSys("START_GAME_SPEED");
		if (cinema)
		{
			Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, 1);
			EmitSys("SET_WIDESCREEN");
		}
		EmitStatements(stmt.body);
		closing();
		if (cinema)
		{
			Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, 0);
			EmitSys("SET_WIDESCREEN");
		}
		EmitSys("END_GAME_SPEED");
		EmitSys("END_CAMERA_CONTROL");
		if (stmt.op == "with dialogue")
		{
			EmitStatements(stmt.handlers);
			_line = _map->EndLine(&stmt);
		}
		EmitSys("END_DIALOGUE");
		return;
	}
	if (name == "dialogue")
	{
		retry("START_DIALOGUE");
		EmitStatements(stmt.body);
		closing();
		EmitSys("END_DIALOGUE");
		return;
	}
	if (name == "dual camera")
	{
		for (const auto& arg : stmt.args)
		{
			EmitValue(*arg, ArgType::Object);
		}
		_line = _map->EndLine(stmt.args.back().get());
		EmitSys("START_DUAL_CAMERA");
		EmitStatements(stmt.body);
		closing();
		EmitSys("RELEASE_DUAL_CAMERA");
		return;
	}
	Error(_map->Of(&stmt), fmt::format("unknown block \"begin {}\"", name));
}

// -------------------------------------------------------------------------------------------------------- expressions

void CodeGenerator::Convert(ArgType from, ArgType to, const Expr& at)
{
	if (to == ArgType::Any || to == ArgType::None || from == to)
	{
		return;
	}
	if (to == ArgType::Float && from == ArgType::Int)
	{
		Emit(Opcode::Cast, VMMode::Cast, DataType::Float, 0);
	}
	else if (to == ArgType::Int && from == ArgType::Float)
	{
		Emit(Opcode::Cast, VMMode::Cast, DataType::Int, 0);
	}
	else if ((to == ArgType::Float && from == ArgType::Object) || (to == ArgType::Object && from == ArgType::Float))
	{
		// Objects travel as numbers
	}
	else if (to == ArgType::Bool || to == ArgType::Coord || from == ArgType::Coord || from == ArgType::Bool ||
	         from == ArgType::None)
	{
		Error(_map->Of(&at), "this value has the wrong type here");
	}
}

ArgType CodeGenerator::EmitValue(const Expr& expr, ArgType want)
{
	// An instruction carries the line the original parser was on when it emitted it: the last line of the
	// expression whose rule emits it
	const auto line = _map->EndLine(&expr);
	switch (expr.kind)
	{
	case ExprKind::Literal:
	{
		_line = line;
		if (expr.type == ValueType::String)
		{
			const auto offset = static_cast<uint32_t>(_data.size());
			_data.insert(_data.end(), expr.text.begin(), expr.text.end());
			_data.push_back('\0');
			Emit(Opcode::Push, VMMode::Immediate, DataType::Int, offset);
			return ArgType::String;
		}
		const auto value = expr.number;
		auto type = want;
		if (type == ArgType::Any)
		{
			type = expr.type == ValueType::Bool ? ArgType::Bool : ArgType::Float;
		}
		switch (type)
		{
		case ArgType::Int:
			Emit(Opcode::Push, VMMode::Immediate, DataType::Int, static_cast<uint32_t>(static_cast<int32_t>(value)));
			return ArgType::Int;
		case ArgType::Bool:
			Emit(Opcode::Push, VMMode::Immediate, DataType::Boolean, value != 0.0 ? 1 : 0);
			return ArgType::Bool;
		case ArgType::Object:
			Emit(Opcode::Push, VMMode::Immediate, DataType::Object, static_cast<uint32_t>(value));
			return ArgType::Object;
		case ArgType::Coord:
			for (int i = 0; i < 3; ++i)
			{
				EmitFloat(Opcode::Push, VMMode::Immediate, DataType::Vector, static_cast<float>(value));
			}
			return ArgType::Coord;
		default:
			EmitFloat(Opcode::Push, VMMode::Immediate, DataType::Float, static_cast<float>(value));
			return ArgType::Float;
		}
	}
	case ExprKind::Variable:
	{
		const auto id = VariableId(expr.text);
		if (!id)
		{
			Error(_map->Of(&expr), fmt::format("unknown variable {}", expr.text));
			return want;
		}
		_line = line;
		Emit(Opcode::Push, VMMode::Reference, DataType::Float, *id);
		const auto natural = want == ArgType::Object ? ArgType::Object : ArgType::Float;
		Convert(natural, want, expr);
		return want == ArgType::Any ? ArgType::Float : want;
	}
	case ExprKind::Constant:
		_line = line;
		Emit(Opcode::Push, VMMode::Immediate, DataType::Int, static_cast<uint32_t>(static_cast<int32_t>(expr.number)));
		Convert(ArgType::Int, want, expr);
		return want == ArgType::Any ? ArgType::Int : want;
	case ExprKind::Cast:
	{
		const auto target = NaturalType(expr.type);
		const auto source = target == ArgType::Float ? ArgType::Int : ArgType::Float;
		EmitValue(*expr.args[0], source);
		_line = line;
		Emit(Opcode::Cast, VMMode::Cast, ToDataType(target), 0);
		Convert(target, want, expr);
		return want == ArgType::Any ? target : want;
	}
	case ExprKind::Unary:
		if (expr.op == Op::Not)
		{
			EmitValue(*expr.args[0], ArgType::Bool);
			_line = line;
			Emit(Opcode::Not, VMMode::Immediate, DataType::Int, 0);
			Convert(ArgType::Bool, want, expr);
			return ArgType::Bool;
		}
		else
		{
			const auto type = expr.type == ValueType::Vector ? ArgType::Coord : ArgType::Float;
			EmitValue(*expr.args[0], type);
			_line = line;
			Emit(Opcode::Neg, VMMode::Immediate, ToDataType(type), 0);
			Convert(type, want, expr);
			return want == ArgType::Any ? type : want;
		}
	case ExprKind::Binary:
	{
		const auto op = expr.op;
		if (op == Op::And || op == Op::Or)
		{
			EmitValue(*expr.args[0], ArgType::Bool);
			EmitValue(*expr.args[1], ArgType::Bool);
			_line = line;
			Emit(ArithmeticOpcode(op), VMMode::Immediate, DataType::Int, 0);
			Convert(ArgType::Bool, want, expr);
			return ArgType::Bool;
		}
		if (op == Op::Eq || op == Op::Ne || op == Op::Lt || op == Op::Le || op == Op::Gt || op == Op::Ge)
		{
			EmitValue(*expr.args[0], ArgType::Float);
			EmitValue(*expr.args[1], ArgType::Float);
			_line = line;
			Emit(ArithmeticOpcode(op), VMMode::Immediate, DataType::Float, 0);
			Convert(ArgType::Bool, want, expr);
			return ArgType::Bool;
		}
		if (expr.type == ValueType::Vector)
		{
			const auto leftType = expr.args[0]->type == ValueType::Vector ? ArgType::Coord : ArgType::Float;
			const auto rightType = expr.args[1]->type == ValueType::Vector ? ArgType::Coord : ArgType::Float;
			EmitValue(*expr.args[0], leftType);
			EmitValue(*expr.args[1], rightType);
			_line = line;
			Emit(ArithmeticOpcode(op), VMMode::Immediate, DataType::Vector, 0);
			Convert(ArgType::Coord, want, expr);
			return ArgType::Coord;
		}
		EmitValue(*expr.args[0], ArgType::Float);
		EmitValue(*expr.args[1], ArgType::Float);
		_line = line;
		Emit(ArithmeticOpcode(op), VMMode::Immediate, DataType::Float, 0);
		Convert(ArgType::Float, want, expr);
		return want == ArgType::Any ? ArgType::Float : want;
	}
	case ExprKind::VectorLiteral:
		for (const auto& component : expr.args)
		{
			EmitValue(*component, ArgType::Float);
			_line = _map->EndLine(component.get());
			Emit(Opcode::Cast, VMMode::Cast, DataType::Vector, 0);
		}
		_line = line;
		if (expr.args.size() == 2)
		{
			EmitFloat(Opcode::Push, VMMode::Immediate, DataType::Vector, 0.0f);
			Emit(Opcode::Swap, VMMode::Immediate, DataType::Int, 0);
		}
		Convert(ArgType::Coord, want, expr);
		return ArgType::Coord;
	case ExprKind::Elapsed:
		EmitValue(*expr.args[0], ArgType::Float);
		_line = line;
		Emit(Opcode::Sleep, VMMode::Immediate, DataType::Float, 0);
		Convert(ArgType::Bool, want, expr);
		return ArgType::Bool;
	case ExprKind::NativeCall:
	{
		const auto result = EmitCall(expr);
		_line = line;
		if (want != ArgType::Any && result != ArgType::None)
		{
			Convert(result, want, expr);
			return want;
		}
		return result;
	}
	default:
		Error(_map->Of(&expr), "this expression can't be compiled");
		return want;
	}
}

ArgType CodeGenerator::EmitCall(const Expr& call)
{
	const auto index = static_cast<uint32_t>(call.number);
	const auto& signature = _env.Natives()[index];
	const auto& name = signature.name;
	const auto paramType = [&](size_t i) { return ParameterType(signature, i); };
	const auto line = _map->EndLine(&call);

	if (name == "GET_PROPERTY" && call.swapped)
	{
		// "Thing is PROPERTY": the object goes first and the two are swapped
		EmitValue(*call.args[1], ArgType::Object);
		EmitValue(*call.args[0], ArgType::Int);
		_line = line;
		Emit(Opcode::Swap, VMMode::Immediate, DataType::Int, 0);
		EmitSys(index, true);
		Emit(Opcode::Cast, VMMode::Cast, DataType::Boolean, 0);
		return ArgType::Bool;
	}

	if (signature.stackIn < 0)
	{
		// A variable number of arguments: the fixed ones, the extra values, their count and what follows
		const auto variadic = static_cast<size_t>(std::ranges::find(signature.params, ArgType::VarArgs, &NativeParam::type) -
		                                          signature.params.begin());
		const auto trailing = signature.params.size() - variadic - 1;
		const auto extra = call.args.size() - variadic - trailing;
		for (size_t i = 0; i < call.args.size(); ++i)
		{
			ArgType type = ArgType::Float;
			if (i < variadic)
			{
				type = paramType(i);
			}
			else if (i >= variadic + extra)
			{
				type = paramType(i - extra + 1);
			}
			EmitValue(*call.args[i], type);
		}
		_line = line;
		EmitSys(index);
		return ResultType(signature);
	}

	size_t first = 0;
	if (call.swapped && call.args.size() >= 2)
	{
		EmitValue(*call.args[1], paramType(1));
		EmitValue(*call.args[0], paramType(0));
		_line = _map->EndLine(call.args[0].get());
		Emit(Opcode::Swap, VMMode::Immediate, DataType::Int, 0);
		first = 2;
	}
	for (size_t i = first; i < call.args.size(); ++i)
	{
		EmitValue(*call.args[i], paramType(i));
	}
	_line = line;
	if (name == "THING_VALID")
	{
		Emit(Opcode::Cast, VMMode::Cast, DataType::Object, 0);
	}
	const bool second = std::ranges::find(k_SecondFormNatives, name) != k_SecondFormNatives.end();
	EmitSys(index, second);
	return ResultType(signature);
}

// ------------------------------------------------------------------------------------------------------- instructions

uint32_t CodeGenerator::Emit(Opcode code, VMMode mode, DataType type, uint32_t data)
{
	_code.emplace_back(code, mode, type, VMValue(data), _line);
	return Here() - 1;
}

uint32_t CodeGenerator::EmitFloat(Opcode code, VMMode mode, DataType type, float value)
{
	_code.emplace_back(code, mode, type, VMValue(value), _line);
	return Here() - 1;
}

uint32_t CodeGenerator::EmitSys(std::string_view native, bool second)
{
	const auto index = _env.FindNative(native);
	return EmitSys(index.value_or(0), second);
}

uint32_t CodeGenerator::EmitSys(uint32_t native, bool second)
{
	return Emit(Opcode::Sys, VMMode::Immediate, second ? DataType::Float : DataType::None, native);
}

void CodeGenerator::EmitJump(Opcode code, uint32_t target)
{
	Emit(code, target > Here() ? VMMode::Forward : VMMode::Backward, DataType::Int, target);
}

uint32_t CodeGenerator::EmitPlaceholder(Opcode code)
{
	const auto mode = code == Opcode::Except ? VMMode::Immediate : VMMode::Forward;
	return Emit(code, mode, DataType::Int, 0);
}

void CodeGenerator::Patch(uint32_t address, uint32_t target)
{
	auto& instruction = _code[address];
	instruction.data = VMValue(target);
	instruction.line = _line;
	if (instruction.code != Opcode::Except)
	{
		instruction.mode = target > address ? VMMode::Forward : VMMode::Backward;
	}
}

} // namespace openblack::lhvm::chl
