/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// The parser of one source file, shared by ChlParser.cpp (statements and the file layout) and ParserExpressions.cpp
/// (expressions and statement forms). See ChlParser.h for the language it reads.

#include <cstdint>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "ChlParser.h"
#include "ChlSyntax.h"
#include "FormTable.h"

namespace openblack::lhvm::chl::parser
{

inline constexpr uint8_t k_PrecedenceOr = 1;
inline constexpr uint8_t k_PrecedenceAnd = 2;
inline constexpr uint8_t k_PrecedenceNot = 3;
inline constexpr uint8_t k_PrecedenceComparison = 4;
inline constexpr uint8_t k_PrecedenceAdditive = 5;
inline constexpr uint8_t k_PrecedenceUnary = 7;
inline constexpr uint8_t k_PrecedenceAtom = 8;

[[nodiscard]] inline ValueType ToValueType(ArgType type)
{
	switch (type)
	{
	case ArgType::Int:
		return ValueType::Int;
	case ArgType::Float:
		return ValueType::Float;
	case ArgType::Coord:
		return ValueType::Vector;
	case ArgType::Object:
		return ValueType::Object;
	case ArgType::Bool:
		return ValueType::Bool;
	case ArgType::String:
		return ValueType::String;
	default:
		return ValueType::Unknown;
	}
}

/// Whether an expression of natural type `actual` can be passed where `expected` is wanted. Variables (Unknown) hold
/// numbers and objects alike.
[[nodiscard]] inline bool Fits(ValueType actual, ArgType expected)
{
	switch (expected)
	{
	case ArgType::Any:
		return actual != ValueType::String;
	case ArgType::Float:
		return actual == ValueType::Float || actual == ValueType::Int || actual == ValueType::Unknown ||
		       actual == ValueType::Object;
	case ArgType::Int:
		return actual == ValueType::Int || actual == ValueType::Float || actual == ValueType::Unknown;
	case ArgType::Object:
		return actual == ValueType::Object || actual == ValueType::Unknown || actual == ValueType::Float;
	case ArgType::Coord:
		return actual == ValueType::Vector;
	case ArgType::Bool:
		return actual == ValueType::Bool;
	case ArgType::String:
		return actual == ValueType::String;
	default:
		return false;
	}
}

[[nodiscard]] inline bool ResultFits(ArgType result, ArgType expected)
{
	if (result == ArgType::None)
	{
		return expected == ArgType::None;
	}
	if (expected == ArgType::None)
	{
		return true;
	}
	if (expected == ArgType::Object)
	{
		return result == ArgType::Object;
	}
	return Fits(ToValueType(result), expected);
}

/// Keywords the original lexer reads as the same word
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 14> k_Synonyms = {{
    {"animation", "anim"},
    {"desires", "desire"},
    {"everything", "all"},
    {"second", "seconds"},
    {"secs", "seconds"},
    {"event", "events"},
    {"property", "properties"},
    {"demonstration", "demo"},
    {"gfx", "graphics"},
    {"introduction", "intro"},
    {"explosion", "explode"},
    {"minimum", "min"},
    {"automatic", "auto"},
    {"color", "colour"},
}};

[[nodiscard]] inline std::string_view Canonical(std::string_view word)
{
	for (const auto& [synonym, canonical] : k_Synonyms)
	{
		if (synonym == word)
		{
			return canonical;
		}
	}
	return word;
}

/// The token is the keyword `word` or one of its synonyms
[[nodiscard]] inline bool IsWord(const Token& token, std::string_view word)
{
	if (token.kind == TokenKind::Symbol)
	{
		return token.text == word;
	}
	return token.kind == TokenKind::Identifier && Canonical(token.text) == Canonical(word);
}

/// Words of a pattern item's text, split at spaces
[[nodiscard]] inline std::vector<std::string_view> SplitWords(std::string_view text)
{
	std::vector<std::string_view> words;
	size_t start = 0;
	while (start < text.size())
	{
		auto end = text.find(' ', start);
		if (end == std::string_view::npos)
		{
			end = text.size();
		}
		if (end > start)
		{
			words.push_back(text.substr(start, end - start));
		}
		start = end + 1;
	}
	return words;
}

class Parser
{
public:
	Parser(std::vector<Token> tokens, std::string_view fileName, ParseEnvironment& environment, DiagnosticSink& sink)
	    : _tokens(std::move(tokens))
	    , _fileName(fileName)
	    , _env(environment)
	    , _forms(environment.Forms())
	    , _sink(sink)
	{
	}

	ParsedFile Run();

private:
	// ---------------------------------------------------------------- tokens

	[[nodiscard]] const Token& Current() const { return _tokens[_pos]; }
	[[nodiscard]] const Token& At(size_t index) const { return _tokens[std::min(index, _tokens.size() - 1)]; }
	[[nodiscard]] bool AtEnd() const { return Current().kind == TokenKind::EndOfFile; }
	[[nodiscard]] bool AtLineEnd() const;
	void Advance();
	bool Accept(std::string_view spelling);
	void SkipNewlines();
	void SkipLine();

	[[nodiscard]] static std::string Describe(const Token& token);

	void Error(SourceLocation location, std::string message)
	{
		_lastErrorLine = location.line;
		_sink.Error(_fileName, location, std::move(message));
	}

	bool Expect(std::string_view spelling);

	void ExpectLineEnd();

	std::string ExpectIdentifier(std::string_view what);

	/// "challenge help script" also starts with "challenge"
	[[nodiscard]] bool IsScriptKindStart() const { return At(_pos + 1).Is("help"); }

	/// Note a place where parsing failed, to report the furthest one when nothing matches
	void NoteExpected(std::string what);

	void ResetFurthest();

	void ReportFurthest(std::string_view context);

	// ---------------------------------------------------------------- names

	[[nodiscard]] bool IsVariable(std::string_view name) const { return _locals.contains(name) || _env.IsGlobal(name); }

	[[nodiscard]] bool IsKeyword(std::string_view name) const { return _forms.IsKeyword(name); }

	/// A named constant: a local alias, a global alias or a header constant
	[[nodiscard]] std::optional<int32_t> LookupConstant(std::string_view name) const;

	/// The id of the current challenge: "challenge NAME" names it, "challenge 53" gives it as a number
	[[nodiscard]] std::optional<int32_t> ChallengeId() const;

	/// "constant NAME = CONSTANT" (after "global" or among a script's locals)
	std::optional<std::pair<std::string, int32_t>> ParseAlias();

	// ---------------------------------------------------------------- nodes

	/// Record where a new expression is in the source
	ExprPtr Finish(std::shared_ptr<Expr> expr, SourceLocation location);

	/// Line of the last token read, not counting line ends
	[[nodiscard]] uint32_t LastConsumedLine() const;

	/// Line of the next token that isn't a line end
	[[nodiscard]] uint32_t NextTokenLine() const;

	ExprPtr MakeNumber(double value, ValueType type, std::string text, SourceLocation location);

	/// The value a pattern gives an argument, typed as the parameter
	ExprPtr MakeValue(double value, ArgType type, SourceLocation location);

	// ---------------------------------------------------------------- forms

	struct FormMatch
	{
		const CompiledForm* form {nullptr};
		std::vector<ExprPtr> args;
		size_t end {0};
	};

	struct MatchState
	{
		const CompiledForm* form;
		std::vector<ExprPtr> args;
		ExprPtr operand;
		SourceLocation location;
		bool statement;
	};

	bool MatchWords(std::string_view text);

	/// Give default values to the arguments of an absent optional group
	void DefaultArguments(const std::vector<PatternItem>& items, MatchState& state);

	bool MatchItems(const std::vector<PatternItem>& items, size_t index, MatchState& state, const std::function<bool()>& rest);

	/// Try one form at the current position. For a postfix form, `operand` is its first argument, already parsed.
	std::optional<FormMatch> TryForm(const CompiledForm& form, const ExprPtr& operand, bool statement, SourceLocation location);

	ExprPtr BuildCall(const FormMatch& match, SourceLocation location);

	/// The best of several candidate matches: the longest, the first of equals. Ties between different functions are
	/// reported when `reportAmbiguity` is set.
	std::optional<FormMatch> Choose(std::vector<FormMatch>& matches, bool reportAmbiguity, SourceLocation location);

	/// A prefix form starting at the current word, with a result fitting `expected`
	ExprPtr ParsePrefixForm(ArgType expected, bool statement);

	/// In the original grammar, forms that start with an operand make conditions, except "PROPERTY of Thing" which
	/// makes a number. So they apply where a condition or a number is expected.
	[[nodiscard]] static bool PostfixAllowed(const CompiledForm& form, ArgType expected);

	/// Postfix forms applied to `operand`
	ExprPtr ParsePostfixForms(ExprPtr operand, ArgType expected, bool statement, SourceLocation location);

	// ---------------------------------------------------------------- expressions

	/// An argument of a native function, read up to the next keyword of the form
	ExprPtr ParseArgument(ArgType type);

	ExprPtr ParseExpression(uint8_t minPrecedence, ArgType expected);

	ExprPtr ParseUnary(uint8_t minPrecedence, ArgType expected);

	ExprPtr ParsePrimary(ArgType expected);

	/// "A near B [radius R]", "A at B" and their "not" forms: distance tests between two positions, compiled as
	/// get distance from A to B < R (R is 1 by default) and get distance from A to B == 0
	ExprPtr ParseDistanceTest(const ExprPtr& left, SourceLocation location);

	/// [x, y, z], [x, z] or [Thing]
	ExprPtr ParseCoordinate();

	/// native NAME(arguments): a call without a statement form
	ExprPtr ParseNativeCall();

	/// A complete condition or value, reporting a problem when it can't be read
	ExprPtr ParseFullExpression(ArgType expected, std::string_view context);

	// ---------------------------------------------------------------- statements

	StmtPtr NewStmt(StmtKind kind, SourceLocation location);

	/// The line starts with one of the words that end a statement list
	[[nodiscard]] bool AtBlockEnd() const;

	StmtList ParseStatements();

	/// when / until handlers, up to the "end" of the guarded block
	StmtList ParseHandlers();

	bool ExpectEnd(std::initializer_list<std::string_view> words, SourceLocation opened, Stmt* stmt);

	StmtPtr ParseStatement();

	StmtPtr ParseAssignment();

	/// PROPERTY of Thing = value (and +=, -=, *=, /=, ++, --)
	StmtPtr TryPropertyAssignment();

	StmtPtr ParseFormStatement();

	/// An argument of a compound statement, reporting a problem when it can't be read
	ExprPtr ParseOperand(ArgType type, std::string_view context);

	/// A compound statement: several native calls the original compiler emits for one statement. `op` names it.
	StmtPtr NewCompound(std::string_view op, SourceLocation location);

	/// Thing play ANIMATION [loop N]
	StmtPtr TryPlay();

	/// state Thing STATE, then lines of "position [Pos]", "float N" and "ulong N, M"
	StmtPtr ParseState();

	/// snapshot (quest|challenge) [at position Pos] [focus Pos] success N alignment N TITLE Script[(args)]
	/// update snapshot success N alignment N TITLE Script[(args)]
	/// update snapshot details [at position Pos] [focus Pos] success N alignment N TITLE [taking picture]
	StmtPtr ParseSnapshot();

	/// set camera to ENUM / move camera to ENUM time N: a camera position and focus from the challenge's camera table
	StmtPtr ParseCameraEnumStatement();

	/// A camera enum, named with or without its challenge's prefix
	ExprPtr ParseCameraEnum();

	ExprPtr MakeCall(std::string_view name, std::vector<ExprPtr> args, SourceLocation location);

	StmtPtr ParseRunScript();

	StmtPtr ParseIf();

	StmtPtr ParseWhile();

	StmtPtr ParseBegin();

	// ---------------------------------------------------------------- top level

	void ParseGlobal(ParsedFile& file);

	void ParseDefine(ParsedFile& file);

	void ParseAutorun(ParsedFile& file);

	/// "script", "help script", "challenge help script"...: the words up to and including "script"
	std::optional<ScriptKind> ParseScriptKindWords();

	std::vector<std::string> ParseParameters();

	void ParseScript(ParsedFile& file);

	std::vector<Token> _tokens;
	std::string_view _fileName;
	ParseEnvironment& _env;
	const FormTable& _forms;
	DiagnosticSink& _sink;
	size_t _pos {0};
	size_t _furthest {0};
	/// Line of the last error reported, to report one error per line
	uint32_t _lastErrorLine {0};
	std::set<std::string> _expected;
	std::set<std::string, std::less<>> _locals;
	std::map<std::string, int32_t, std::less<>> _localConstants;
	/// Name of the current challenge ("challenge NAME"), used to look up its id and its camera enums
	std::string _challenge;
	SourceMap* _map {nullptr};
};

} // namespace openblack::lhvm::chl::parser
