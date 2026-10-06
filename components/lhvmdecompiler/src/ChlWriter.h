/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ChlAst.h"
#include "ChlConstants.h"
#include "ChlForms.h"
#include "LHVMDecompiler.h"
#include "LHVMNatives.h"

namespace openblack::lhvm::detail
{

struct OutputLine
{
	int indent {0};
	std::string text;
	std::vector<uint32_t> ips;
};

/// Writes a syntax tree as Challenge Language source, one line per statement, remembering which instructions each line
/// stands for
class ChlWriter
{
public:
	ChlWriter(std::span<const NativeSignature> natives, const chl::ConstantTable* constants,
	          std::vector<Diagnostic>& diagnostics);

	/// Write a script. `challenge` is the one in effect before it; `standalone` names the first one it needs above
	/// "begin script" rather than leaving it to the caller.
	void WriteScript(const chl::Script& script, std::optional<int32_t> challenge, bool standalone);

	/// The challenge the script needed first when none was in effect, by name
	[[nodiscard]] const std::string& LeadingChallenge() const { return _leadingChallenge; }
	/// The challenge in effect after the script
	[[nodiscard]] std::optional<int32_t> Challenge() const { return _challenge; }

	/// The expression as CHL, bracketed when its precedence is below `minPrecedence`
	[[nodiscard]] std::string Expression(const chl::ExprPtr& expr, uint8_t minPrecedence = 0);

	[[nodiscard]] const std::vector<OutputLine>& Lines() const { return _lines; }
	[[nodiscard]] uint32_t GotoCount() const { return _gotos; }
	[[nodiscard]] uint32_t FallbackCount() const { return _fallbacks; }
	[[nodiscard]] uint32_t NativeCallCount() const { return _nativeCalls; }

private:
	void Line(int indent, std::string text, std::vector<uint32_t> ips);
	void Statements(const chl::StmtList& list, int indent);
	void Statement(const chl::Stmt& stmt, int indent);
	void Handlers(const chl::StmtList& handlers, int indent);
	/// Attach instructions to the last line written, or a new comment line if there is none since `firstLine`
	void AttachToLast(size_t firstLine, const std::vector<uint32_t>& ips);
	void Fallback(const chl::Stmt& stmt, std::string_view what);

	[[nodiscard]] uint8_t Precedence(const chl::ExprPtr& expr);
	[[nodiscard]] std::string NativeCall(const chl::ExprPtr& call);
	/// The first statement form that fits the call, positive or negated
	[[nodiscard]] std::optional<std::string> FormFor(const chl::ExprPtr& call, bool negated);
	/// "X not exists" for not (X exists), when the call has such a form
	[[nodiscard]] std::optional<std::string> NegatedForm(const chl::ExprPtr& expr);
	[[nodiscard]] std::optional<std::string> Form(const chl::StatementForm& form, const chl::ExprPtr& call,
	                                              const NativeSignature* signature);
	[[nodiscard]] std::optional<std::string> Items(const std::vector<chl::PatternItem>& items, const chl::ExprPtr& call,
	                                               const NativeSignature* signature);
	/// An integer as the name of a game constant when it has one
	[[nodiscard]] std::string Constant(const chl::ExprPtr& value, const std::string& enumName);
	[[nodiscard]] std::string Argument(const chl::ExprPtr& arg, ArgType type, const std::string& enumName);
	[[nodiscard]] std::optional<std::string> SpecialForm(const chl::ExprPtr& expr);
	/// "P of O += e" and friends, if the statement is one
	[[nodiscard]] std::optional<std::string> PropertyCompound(const chl::Stmt& stmt);
	[[nodiscard]] std::string ChallengeName(int32_t id, uint32_t ip);
	/// Snapshots and highlights, which take the current challenge as a hidden argument
	[[nodiscard]] std::optional<std::string> ChallengeForm(const chl::ExprPtr& call);
	[[nodiscard]] const NativeSignature* Signature(const chl::ExprPtr& call) const;

	std::span<const NativeSignature> _natives;
	const chl::ConstantTable* _constants;
	std::vector<Diagnostic>& _diagnostics;
	std::map<std::string_view, std::vector<const NativeSignature*>, std::less<>> _byName;
	std::map<const chl::StatementForm*, std::vector<chl::PatternItem>> _patterns;
	std::vector<OutputLine> _lines;
	/// The challenge the last "challenge" line named, and the one the line being written needs
	std::optional<int32_t> _challenge;
	std::optional<int32_t> _pendingChallenge;
	std::string _leadingChallenge;
	bool _standalone {true};
	bool _inLocals {false};
	size_t _scriptStart {0};
	/// The form last written ends in an expression, which would take in an operator after it
	bool _greedy {false};
	uint32_t _gotos {0};
	uint32_t _fallbacks {0};
	uint32_t _nativeCalls {0};
};

} // namespace openblack::lhvm::detail
