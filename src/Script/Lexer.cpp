#include "Lexer.h"
#include "Token.h"

using namespace OpenBlack::Script;

Lexer::Lexer(std::string &input) : m_source(input) {
	m_current = m_source.begin();
	m_end = m_source.end();
};

std::list<Token> Lexer::GetTokens()
{
	std::list<Token> tokens;

	while (m_current != m_end)
	{
		auto token = GetToken();
		tokens.push_back(token);
	}

	return tokens;
}

Token Lexer::GetToken()
{
	if (m_current == m_end) {
		return Token(TokenType::EndOfFile);
	}

	if (*m_current == '\r' || *m_current == '\n') {
		return MakeEndOfLine();
	}
	//else if (IsWhitespace(*m_current))
	//{
	//	return MakeWhitespace();
	//}
	//else if (m_current == "rem")
	//{
	//	return MakeComment()
	//}
	else if (IsLetter(*m_current))
	{
		return MakeIdentifier();
	}
	else if (*m_current == '"')
	{
		return MakeStringLiteral();
	}
	else if (IsDigit(*m_current))
	{
		return MakeNumberLiteral();
	}

	// consume symbols: , ( )
	switch (*m_current)
	{
	case ',': return MakePunctuation(TokenType::Comma);
	case '(': return MakePunctuation(TokenType::LeftParen);
	case ')': return MakePunctuation(TokenType::RightParen);
	}

	// default
	char value = *m_current;
	m_current++;
	return Token(TokenType::Unknown, std::string(1, value));
}

Token Lexer::MakeEndOfLine()
{
	if (*m_current == '\n') { // consume \n
		m_current++;
	} else if (*m_current == '\r') { // consume \r\n
		m_current++;

		if (*m_current == '\n') {
			m_current++;
		}
	}

	return Token(TokenType::EndOfLine);
}

Token Lexer::MakeIdentifier()
{
	auto identifier_begin = m_current;

	// A-Z 0-9 _ continue
	while (IsLetter(*m_current) || IsDigit(*m_current) || *m_current == '_')
	{
		m_current++;
	}

	std::string value(identifier_begin, m_current);
	return Token(TokenType::Identifier, value);
}

Token Lexer::MakeStringLiteral()
{
	auto string_begin = m_current + 1;

	m_current++;
	while (*m_current != '"')
	{
		m_current++;
	}

	std::string value(string_begin, m_current);

	m_current++;
	return Token(TokenType::StringLiteral, value);
}

Token Lexer::MakeNumberLiteral()
{
	// todo: change token type for float.
	auto number_begin = m_current;

	while (IsDigit(*m_current) || *m_current == '.')
	{
		m_current++;
	}

	std::string value(number_begin, m_current);
	return Token(TokenType::NumberLiteral, value);
}

Token Lexer::MakePunctuation(TokenType type)
{
	m_current++;

	return Token(type);
}
