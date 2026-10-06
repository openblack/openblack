/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Splits Challenge Language source into tokens, following the original compiler's lexer:
///
/// - Words are letters, digits and underscores. An all-digit word is an integer, digits followed by '.' and more
///   digits a real ("3." too); every other word is an identifier, even one starting with a digit ("3d"). Keywords are
///   identifiers here: the parser decides from the context.
/// - Strings are written between double quotes on one line, with the escapes \n, \t and \".
/// - Comments run from // to the end of the line, or between /* and */, which nest.
/// - The end of a line is a token: statements end there. CRLF and lone CR count as line ends.
/// - Operators and punctuation: ( ) [ ] , : = += -= *= /= %= ++ -- + - * / % == != < <= > >=

#include <cstdint>

#include <string>
#include <string_view>
#include <vector>

#include "ChlDiagnostics.h"

namespace openblack::lhvm::chl
{

enum class TokenKind : uint8_t
{
	Identifier,
	Number,
	String,
	Symbol,
	EndOfLine,
	EndOfFile
};

struct Token
{
	TokenKind kind {TokenKind::EndOfFile};
	/// The spelling; for strings, the text between the quotes
	std::string text;
	/// Numbers: the value
	double number {0.0};
	/// Numbers: written without a fraction
	bool integer {false};
	SourceLocation location;

	[[nodiscard]] bool Is(std::string_view spelling) const
	{
		return (kind == TokenKind::Identifier || kind == TokenKind::Symbol) && text == spelling;
	}
};

/// Tokens of `text`, always ending with EndOfFile. Problems (bad characters, unterminated strings) are reported to
/// `sink` and skipped.
[[nodiscard]] std::vector<Token> Tokenize(std::string_view text, std::string_view fileName, DiagnosticSink& sink);

} // namespace openblack::lhvm::chl
