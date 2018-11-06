#include "Token.h"

std::string OpenBlack::Script::GetTokenTypeName(TokenType type)
{
	switch (type)
	{
	case Unknown: return "Unknown";
	case EndOfFile: return "EndOfFile";
	case EndOfLine: return "EndOfLine";
	case WhiteSpace: return "WhiteSpace";
	case Comment: return "Comment";
	case Identifier: return "Identifier";
	case StringLiteral: return "StringLiteral";
	case NumberLiteral: return "NumberLiteral";
	case Comma: return "Comma";
	case LeftParen: return "LeftParen";
	case RightParen: return "RightParen";
	case Assign: return "Assign";
	}

	return "Undefined Token Type Name";
}
