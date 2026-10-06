/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Parser of Challenge Language source into the shared syntax tree (ChlAst.h).
///
/// The parser is recursive descent. Control statements (if, while, begin ... end, wait, run script, assignments) are
/// parsed by hand; everything else is a native function call written in one of its statement forms, which are matched
/// against the patterns of ChlForms.h rather than coded one by one. Arguments are parsed as expressions of their
/// parameter's type, and forms whose arguments don't fit are rejected, which tells apart forms that read the same
/// ("[Pos] viewed" and "Thing viewed").
///
/// Names are resolved while parsing, in source order, as the original compiler did: a global is visible to the scripts
/// that follow its declaration, locals and parameters to their script, and other identifiers are looked up in the
/// constant table.

#include <cstdint>

#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ChlAst.h"
#include "ChlConstants.h"
#include "ChlDiagnostics.h"
#include "ChlLexer.h"
#include "LHVMNatives.h"

namespace openblack::lhvm::chl
{

class FormTable;

/// Source positions of the nodes of a parsed script, used for error messages and the line numbers of the code
struct SourceMap
{
	std::unordered_map<const Stmt*, SourceLocation> statements;
	/// Line closing a block statement ("end if", "end while"...)
	std::unordered_map<const Stmt*, SourceLocation> closings;
	std::unordered_map<const Expr*, SourceLocation> expressions;
	/// Line of each branch's "if"/"elsif"/"else"
	std::unordered_map<const Branch*, SourceLocation> branches;
	/// Line of the last token of each expression and simple statement
	std::unordered_map<const void*, uint32_t> endLines;
	SourceLocation begin;
	SourceLocation start;
	SourceLocation end;

	[[nodiscard]] SourceLocation Of(const Stmt* stmt) const;
	[[nodiscard]] SourceLocation Of(const Expr* expr) const;
	/// Line of the node's last token (its first line when unknown)
	[[nodiscard]] uint32_t EndLine(const Stmt* stmt) const;
	[[nodiscard]] uint32_t EndLine(const Expr* expr) const;
};

struct ParsedScript
{
	std::shared_ptr<Script> script;
	SourceMap sourceMap;
};

struct GlobalDeclaration
{
	std::string name;
	SourceLocation location;
};

struct AutorunDeclaration
{
	std::string script;
	SourceLocation location;
};

struct ScriptDefinition
{
	ScriptKind kind {ScriptKind::Script};
	std::string name;
	std::vector<std::string> params;
	SourceLocation location;
};

/// One top-level item of a source file, in source order
using TopLevelItem = std::variant<GlobalDeclaration, AutorunDeclaration, ScriptDefinition, ParsedScript>;

struct ParsedFile
{
	std::string fileName;
	/// "challenge NAME" at the top of the file, if any
	std::string challenge;
	std::vector<TopLevelItem> items;
};

/// Names and tables shared by every file of a compilation. Files must be parsed in order: a global declared in one file
/// is visible in the files that follow.
class ParseEnvironment
{
public:
	ParseEnvironment(std::span<const NativeSignature> natives, const ConstantTable* constants);
	~ParseEnvironment();
	ParseEnvironment(const ParseEnvironment&) = delete;
	ParseEnvironment& operator=(const ParseEnvironment&) = delete;

	[[nodiscard]] std::span<const NativeSignature> Natives() const { return _natives; }
	[[nodiscard]] const ConstantTable* Constants() const { return _constants; }
	[[nodiscard]] const FormTable& Forms() const { return *_forms; }

	/// Index of the native function called `name` taking `argumentCount` arguments (or any number when negative)
	[[nodiscard]] std::optional<uint32_t> FindNative(std::string_view name, int argumentCount = -1) const;

	/// Globals in declaration order
	[[nodiscard]] const std::vector<std::string>& Globals() const { return _globals; }
	[[nodiscard]] bool IsGlobal(std::string_view name) const;
	void AddGlobal(std::string name) { _globals.push_back(std::move(name)); }

	/// Constant aliases declared with "global constant NAME = CONSTANT"
	void AddAlias(std::string name, int32_t value);
	[[nodiscard]] std::optional<int32_t> FindAlias(std::string_view name) const;

private:
	std::span<const NativeSignature> _natives;
	const ConstantTable* _constants;
	std::unique_ptr<FormTable> _forms;
	std::multimap<std::string, uint32_t, std::less<>> _nativeIndex;
	std::vector<std::string> _globals;
	std::map<std::string, int32_t, std::less<>> _aliases;
};

/// Parse one file. Errors are reported to `sink`; the parser recovers at the next line or block so that one run reports
/// as many problems as it can.
[[nodiscard]] ParsedFile ParseFile(std::string_view text, std::string_view fileName, ParseEnvironment& environment,
                                   DiagnosticSink& sink);

/// The statement forms that could spell the call, for diagnostics and tests: each entry is the form's pattern
[[nodiscard]] std::vector<std::string> DescribeFormsOf(const ParseEnvironment& environment, std::string_view native);

} // namespace openblack::lhvm::chl
