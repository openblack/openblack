/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlLexer.h"

#include <algorithm>
#include <array>

#include <fmt/format.h>

#include "ChlSyntax.h"

namespace openblack::lhvm::chl
{

namespace
{

constexpr std::array<std::string_view, 11> k_TwoCharSymbols = {"+=", "-=", "*=", "/=", "%=", "++",
                                                               "--", "==", "!=", "<=", ">="};
constexpr std::string_view k_OneCharSymbols = "()[],:=+-*/%<>";

[[nodiscard]] bool IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

[[nodiscard]] bool IsIdentifierChar(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || IsDigit(c);
}

class Lexer
{
public:
	Lexer(std::string_view text, std::string_view fileName, DiagnosticSink& sink)
	    : _text(text)
	    , _fileName(fileName)
	    , _sink(sink)
	{
	}

	std::vector<Token> Run()
	{
		while (_pos < _text.size())
		{
			const char c = _text[_pos];
			if (c == '\n')
			{
				Push(TokenKind::EndOfLine, "\n", Here());
				Advance();
			}
			else if (c == ' ' || c == '\t')
			{
				Advance();
			}
			else if (c == '/' && Peek(1) == '/')
			{
				while (_pos < _text.size() && _text[_pos] != '\n')
				{
					Advance();
				}
			}
			else if (c == '/' && Peek(1) == '*')
			{
				SkipBlockComment();
			}
			else if (IsIdentifierChar(c))
			{
				LexWord();
			}
			else if (c == '"')
			{
				LexString();
			}
			else
			{
				LexSymbol();
			}
		}
		Push(TokenKind::EndOfLine, "\n", Here());
		Push(TokenKind::EndOfFile, "", Here());
		return std::move(_tokens);
	}

private:
	[[nodiscard]] char Peek(size_t offset) const { return _pos + offset < _text.size() ? _text[_pos + offset] : '\0'; }

	[[nodiscard]] SourceLocation Here() const { return {.line = _line, .column = _column}; }

	void Advance()
	{
		if (_text[_pos] == '\n')
		{
			++_line;
			_column = 1;
		}
		else
		{
			++_column;
		}
		++_pos;
	}

	void Push(TokenKind kind, std::string text, SourceLocation location)
	{
		Token token;
		token.kind = kind;
		token.text = std::move(text);
		token.location = location;
		_tokens.push_back(std::move(token));
	}

	/// Block comments nest
	void SkipBlockComment()
	{
		const auto start = Here();
		int depth = 0;
		while (_pos < _text.size())
		{
			if (_text[_pos] == '/' && Peek(1) == '*')
			{
				++depth;
				Advance();
				Advance();
			}
			else if (_text[_pos] == '*' && Peek(1) == '/')
			{
				Advance();
				Advance();
				if (--depth == 0)
				{
					return;
				}
			}
			else
			{
				Advance();
			}
		}
		_sink.Error(_fileName, start, "unterminated comment");
	}

	/// Digits followed by 'e': the start of a number with an exponent
	[[nodiscard]] static bool IsExponentStart(std::string_view word)
	{
		return word.size() >= 2 && word.back() == 'e' && std::ranges::all_of(word.substr(0, word.size() - 1), IsDigit);
	}

	/// An identifier, a keyword or a number: letters, digits and underscores. An all-digit run is an integer, followed
	/// by '.' and digits a real; anything else is an identifier, even when it starts with a digit ("3d").
	void LexWord()
	{
		const auto start = Here();
		const auto begin = _pos;
		while (_pos < _text.size() && IsIdentifierChar(_text[_pos]))
		{
			Advance();
		}
		auto word = _text.substr(begin, _pos - begin);
		bool exponent = false;
		if (IsExponentStart(word) && (Peek(0) == '+' || Peek(0) == '-') && IsDigit(Peek(1)))
		{
			exponent = true;
			// openblack extension: a number with an exponent ("2e+06"), which the original lexer doesn't read
			Advance();
			while (_pos < _text.size() && IsDigit(_text[_pos]))
			{
				Advance();
			}
			word = _text.substr(begin, _pos - begin);
		}
		if (!std::ranges::all_of(word, IsDigit))
		{
			const auto value = ParseDouble(word);
			if (exponent && value.has_value())
			{
				Token token;
				token.kind = TokenKind::Number;
				token.location = start;
				token.number = *value;
				token.text = std::string(word);
				_tokens.push_back(std::move(token));
				return;
			}
			Push(TokenKind::Identifier, std::string(word), start);
			return;
		}
		Token token;
		token.kind = TokenKind::Number;
		token.location = start;
		token.integer = true;
		if (_pos < _text.size() && _text[_pos] == '.')
		{
			token.integer = false;
			Advance();
			while (_pos < _text.size() && IsDigit(_text[_pos]))
			{
				Advance();
			}
			if (Peek(0) == 'e' && (Peek(1) == '+' || Peek(1) == '-') && IsDigit(Peek(2)))
			{
				Advance();
				Advance();
				while (_pos < _text.size() && IsDigit(_text[_pos]))
				{
					Advance();
				}
			}
		}
		token.text = std::string(_text.substr(begin, _pos - begin));
		token.number = ParseDouble(token.text).value_or(0.0);
		_tokens.push_back(std::move(token));
	}

	/// "text", with the escapes \n, \t and \"; any other escaped character stands for itself
	void LexString()
	{
		const auto start = Here();
		Advance();
		std::string value;
		while (_pos < _text.size() && _text[_pos] != '"' && _text[_pos] != '\n')
		{
			char c = _text[_pos];
			if (c == '\\' && _pos + 1 < _text.size() && _text[_pos + 1] != '\n')
			{
				Advance();
				c = _text[_pos];
				if (c == 'n')
				{
					c = '\n';
				}
				else if (c == 't')
				{
					c = '\t';
				}
			}
			value.push_back(c);
			Advance();
		}
		if (_pos >= _text.size() || _text[_pos] != '"')
		{
			_sink.Error(_fileName, start, "unterminated string");
		}
		else
		{
			Advance();
		}
		Push(TokenKind::String, std::move(value), start);
	}

	void LexSymbol()
	{
		const auto start = Here();
		const auto two = _text.substr(_pos, 2);
		for (const auto symbol : k_TwoCharSymbols)
		{
			if (two == symbol)
			{
				Advance();
				Advance();
				Push(TokenKind::Symbol, std::string(symbol), start);
				return;
			}
		}
		const char c = _text[_pos];
		Advance();
		if (k_OneCharSymbols.find(c) != std::string_view::npos)
		{
			Push(TokenKind::Symbol, std::string(1, c), start);
			return;
		}
		_sink.Error(_fileName, start, fmt::format("unexpected character '{}'", c));
	}

	std::string_view _text;
	std::string_view _fileName;
	DiagnosticSink& _sink;
	std::vector<Token> _tokens;
	size_t _pos {0};
	uint32_t _line {1};
	uint32_t _column {1};
};

} // namespace

std::vector<Token> Tokenize(std::string_view text, std::string_view fileName, DiagnosticSink& sink)
{
	// The original read sources in text mode: CRLF and lone CR are line ends
	std::string normalised;
	normalised.reserve(text.size());
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (text[i] == '\r')
		{
			normalised.push_back('\n');
			if (i + 1 < text.size() && text[i + 1] == '\n')
			{
				++i;
			}
		}
		else
		{
			normalised.push_back(text[i]);
		}
	}
	return Lexer(normalised, fileName, sink).Run();
}

std::string CompileDiagnostic::ToString() const
{
	std::string_view severityName = "error";
	if (severity == CompileSeverity::Warning)
	{
		severityName = "warning";
	}
	else if (severity == CompileSeverity::Note)
	{
		severityName = "note";
	}
	return fmt::format("{}:{}:{}: {}: {}", file, location.line, location.column, severityName, message);
}

void DiagnosticSink::Report(CompileSeverity severity, std::string_view file, SourceLocation location, std::string message)
{
	if (severity == CompileSeverity::Error)
	{
		++_errorCount;
	}
	_diagnostics.push_back(
	    {.severity = severity, .file = std::string(file), .location = location, .message = std::move(message)});
}

} // namespace openblack::lhvm::chl
