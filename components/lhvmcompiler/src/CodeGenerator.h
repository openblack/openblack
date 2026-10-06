/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <map>
#include <string>
#include <vector>

#include <LHVMTypes.h>

#include "ChlParser.h"

namespace openblack::lhvm::chl
{

/// Turns parsed files into a program, emitting the instruction sequences of Black & White's own compiler
class CodeGenerator
{
public:
	struct ScriptRecord
	{
		VMScript script;
		ScriptKind kind {ScriptKind::Script};
	};

	CodeGenerator(const ParseEnvironment& environment, DiagnosticSink& sink, uint32_t firstScriptId);

	/// Start from an existing program's tables (for recompiling scripts into it)
	void Seed(std::vector<VMInstruction> instructions, std::vector<VMScript> scripts, std::vector<char> data,
	          std::vector<uint32_t> autostart, size_t globalCount);

	/// Compile the items of a file, in order
	void Add(const ParsedFile& file);

	/// Resolve script calls and autostart names. False when a name is unknown.
	bool Finish();

	[[nodiscard]] std::vector<VMInstruction>& Instructions() { return _code; }
	[[nodiscard]] std::vector<VMScript>& Scripts() { return _scripts; }
	[[nodiscard]] std::vector<char>& Data() { return _data; }
	[[nodiscard]] std::vector<uint32_t>& Autostart() { return _autostart; }

private:
	struct PendingCall
	{
		uint32_t address;
		std::string script;
		size_t argumentCount;
		std::string file;
		SourceLocation location;
	};

	struct PendingAutorun
	{
		std::string script;
		std::string file;
		SourceLocation location;
	};

	/// Where a variable lives: its number in the bytecode
	[[nodiscard]] std::optional<uint32_t> VariableId(const std::string& name) const;

	void CompileScript(const ParsedScript& parsed, const std::string& file);

	// Statements
	void EmitStatements(const StmtList& list);
	void EmitStatement(const Stmt& stmt);
	void EmitAssign(const Stmt& stmt);
	void EmitPropertyAssign(const Stmt& stmt);
	void EmitIf(const Stmt& stmt);
	void EmitLoop(const Stmt& stmt);
	/// `afterLine` is the line of what follows the last handler ("end ...")
	void EmitHandlers(const StmtList& handlers, std::vector<uint32_t>& exits, uint32_t afterLine);
	void EmitBlock(const Stmt& stmt);
	void EmitCallStatement(const Stmt& stmt);
	/// Statements the original compiler builds from several calls ("play", "state", "set camera to"...)
	void EmitCompound(const Stmt& stmt);

	// Expressions
	/// Push `expr` as a value of type `want` (Any: its own type). Returns the type it was left as.
	ArgType EmitValue(const Expr& expr, ArgType want);
	/// Push a native call's arguments and call it. Returns its result type.
	ArgType EmitCall(const Expr& call);
	void Convert(ArgType from, ArgType to, const Expr& at);

	// Instructions
	uint32_t Emit(Opcode code, VMMode mode, DataType type, uint32_t data);
	uint32_t EmitFloat(Opcode code, VMMode mode, DataType type, float value);
	uint32_t EmitSys(std::string_view native, bool second = false);
	uint32_t EmitSys(uint32_t native, bool second = false);
	void EmitJump(Opcode code, uint32_t target);
	uint32_t EmitPlaceholder(Opcode code);
	void Patch(uint32_t address, uint32_t target);
	[[nodiscard]] uint32_t Here() const { return static_cast<uint32_t>(_code.size()); }

	void Error(SourceLocation location, std::string message);
	void SetLine(SourceLocation location) { _line = location.line; }
	void SetLine(const Stmt& stmt) { SetLine(_map->Of(&stmt)); }

	const ParseEnvironment& _env;
	DiagnosticSink& _sink;
	uint32_t _nextScriptId;
	std::vector<VMInstruction> _code;
	std::vector<VMScript> _scripts;
	std::vector<char> _data;
	std::vector<uint32_t> _autostart;
	std::vector<PendingCall> _calls;
	std::vector<PendingAutorun> _autoruns;
	/// Scripts declared with "define script" and their argument counts
	std::map<std::string, size_t, std::less<>> _defined;
	size_t _globalCount {0};
	std::vector<std::string> _globalNames;

	// The script being compiled
	const SourceMap* _map {nullptr};
	std::string _file;
	std::vector<std::string> _localNames;
	uint32_t _variablesOffset {0};
	uint32_t _line {0};
	/// Nesting of blocks that allow dialogue and camera statements
	std::map<std::string, std::string> _labels;
	std::map<std::string, uint32_t> _labelAddresses;
	std::vector<std::pair<uint32_t, std::string>> _gotos;
};

} // namespace openblack::lhvm::chl
