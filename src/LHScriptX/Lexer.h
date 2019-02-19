#pragma once
#ifndef OPENBLACK_LHSCRIPTX_LEXER_H
#define OPENBLACK_LHSCRIPTX_LEXER_H


#include <cctype>
#include <list>
#include <iostream>

#include <LHScriptX/Token.h>

namespace OpenBlack {
namespace LHScriptX {

class Lexer {
public:
	Lexer(const std::string &input);

	Token GetToken();
private:
	void advance();

	inline void skip_whitespace();
	inline void skip_comments();

	inline Token make_identifier();
	inline Token make_string_literal();
	inline Token make_number_literal();
	inline Token make_symbol(TokenType type);

	std::string _source;
	std::string::iterator _current;
	std::string::iterator _end;

	int _currentLine;

	inline bool is_letter(const char c)
	{
		return (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z'));
	}

	inline bool is_digit(const char c)
	{
		return (c >= '0' && c <= '9');
	}

	inline bool is_whitespace(const char c)
	{
		return (' ' == c) || ('\n' == c) ||
			('\r' == c) || ('\t' == c) ||
			('\b' == c) || ('\v' == c) ||
			('\f' == c);
	}

	inline bool imatch(const char c1, const char c2)
	{
		return std::tolower(c1) == std::tolower(c2);
	}

};

}
}

#endif
