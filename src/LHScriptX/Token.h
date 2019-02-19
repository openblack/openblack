#pragma once
#ifndef OPENBLACK_LHSCRIPTX_TOKEN_H
#define OPENBLACK_LHSCRIPTX_TOKEN_H

#include <stdint.h>
#include <string>

namespace OpenBlack {
namespace LHScriptX {

enum TokenType : uint8_t
{
	Unknown,

	Identifier, // abc123
	StringLiteral, // "123"
	NumberLiteral, // 1234
	FloatLiteral, // 12.34

	ListSeparator, // ,
	OpenParanthesis, // (
	CloseParanethesis, // )

	Assignment, // =

	EndOfFile
};

struct TokenWord {
	const char* word;
	size_t wordLength;
	TokenType tokenType;
};

/*#define asTokenDef(str, tok) {str, sizeof(str)-1, tok}

TokenWord const tokenWords[] =
{
	asTokenDef("=", TokenType::Assignment)

};*/

struct Token {
	Token() : Token(Unknown) {};
	Token(TokenType _type) : type(_type), value("") {};
	Token(TokenType _type, std::string _value) : type(_type), value(_value) {};
	Token(TokenType _type, std::string _value, int _line) : type(_type), value(_value), line(_line) {};

	TokenType type;
	std::string value;
	uint32_t line;

	TokenType GetType() const { return type; }
	std::string GetValue() const { return value; }
	int GetLine() const { return line;  }
};

inline std::ostream& operator<< (std::ostream& os, TokenType tt)
{
	switch (tt)
	{
	case TokenType::Identifier:	       return os << "<identifier>";
	case TokenType::StringLiteral:     return os << "<string literal>";
	case TokenType::NumberLiteral:     return os << "<number literal>";
	case TokenType::FloatLiteral:      return os << "<float literal>";
	case TokenType::ListSeparator:     return os << "<list separator>";
	case TokenType::OpenParanthesis:   return os << "<open paranthesis>";
	case TokenType::CloseParanethesis: return os << "<close paranethesis>";
	case TokenType::Assignment:		   return os << "<assignment>";
	case TokenType::EndOfFile:		   return os << "<end of file>";
	};
	return os << "<unknown token: " << static_cast<std::uint8_t>(tt) << ">";
}

}
}

#endif
