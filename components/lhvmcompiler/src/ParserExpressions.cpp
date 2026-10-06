/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <functional>

#include <fmt/format.h>

#include "Parser.h"

namespace openblack::lhvm::chl::parser
{

bool Parser::AtLineEnd() const
{
	return Current().kind == TokenKind::EndOfLine || Current().kind == TokenKind::EndOfFile;
}

void Parser::Advance()
{
	if (!AtEnd())
	{
		++_pos;
	}
}

bool Parser::Accept(std::string_view spelling)
{
	if (Current().Is(spelling))
	{
		Advance();
		return true;
	}
	return false;
}

void Parser::SkipNewlines()
{
	while (Current().kind == TokenKind::EndOfLine)
	{
		Advance();
	}
}

void Parser::SkipLine()
{
	while (!AtLineEnd())
	{
		Advance();
	}
	SkipNewlines();
}

std::string Parser::Describe(const Token& token)
{
	switch (token.kind)
	{
	case TokenKind::EndOfLine:
		return "the end of the line";
	case TokenKind::EndOfFile:
		return "the end of the file";
	case TokenKind::String:
		return fmt::format("\"{}\"", token.text);
	default:
		return fmt::format("'{}'", token.text);
	}
}

bool Parser::Expect(std::string_view spelling)
{
	if (Accept(spelling))
	{
		return true;
	}
	Error(Current().location, fmt::format("expected '{}', not {}", spelling, Describe(Current())));
	return false;
}

void Parser::ExpectLineEnd()
{
	if (!AtLineEnd())
	{
		// One error per line: the rest of a line that already has one is skipped quietly
		if (_lastErrorLine != Current().location.line)
		{
			Error(Current().location, fmt::format("unexpected {} at the end of the statement", Describe(Current())));
		}
		SkipLine();
		return;
	}
	SkipNewlines();
}

std::string Parser::ExpectIdentifier(std::string_view what)
{
	if (Current().kind != TokenKind::Identifier)
	{
		Error(Current().location, fmt::format("expected {}, not {}", what, Describe(Current())));
		return {};
	}
	auto name = Current().text;
	Advance();
	return name;
}

void Parser::NoteExpected(std::string what)
{
	if (_pos > _furthest)
	{
		_furthest = _pos;
		_expected.clear();
	}
	if (_pos == _furthest)
	{
		_expected.insert(std::move(what));
	}
}

void Parser::ResetFurthest()
{
	_furthest = _pos;
	_expected.clear();
}

void Parser::ReportFurthest(std::string_view context)
{
	const auto& token = At(_furthest);
	std::string message;
	if (_expected.empty())
	{
		message = fmt::format("can't make sense of {} here", Describe(token));
	}
	else
	{
		std::string list;
		size_t count = 0;
		for (const auto& item : _expected)
		{
			if (count == 6)
			{
				list += ", ...";
				break;
			}
			list += (count == 0 ? "" : count + 1 == _expected.size() ? " or " : ", ") + item;
			++count;
		}
		message = fmt::format("expected {} before {}", list, Describe(token));
	}
	if (!context.empty())
	{
		message += fmt::format(" (in {})", context);
	}
	Error(token.location, message);
}

std::optional<int32_t> Parser::LookupConstant(std::string_view name) const
{
	if (const auto it = _localConstants.find(name); it != _localConstants.end())
	{
		return it->second;
	}
	if (const auto value = _env.FindAlias(name))
	{
		return value;
	}
	if (const auto* constants = _env.Constants(); constants != nullptr)
	{
		return constants->ValueOf(name);
	}
	return std::nullopt;
}

std::optional<int32_t> Parser::ChallengeId() const
{
	if (!_challenge.empty() && std::ranges::all_of(_challenge, [](char c) { return c >= '0' && c <= '9'; }))
	{
		return std::stoi(_challenge);
	}
	return LookupConstant("CHALLENGE_" + _challenge);
}

std::optional<std::pair<std::string, int32_t>> Parser::ParseAlias()
{
	const auto location = Current().location;
	Advance();
	auto name = ExpectIdentifier("a constant name");
	if (name.empty() || !Expect("="))
	{
		SkipLine();
		return std::nullopt;
	}
	const auto valueLocation = Current().location;
	const auto valueName = ExpectIdentifier("a constant");
	const auto value = valueName.empty() ? std::nullopt : LookupConstant(valueName);
	if (!valueName.empty() && !value)
	{
		Error(valueLocation, fmt::format("unknown constant {}", valueName));
	}
	ExpectLineEnd();
	(void)location;
	if (!value)
	{
		return std::nullopt;
	}
	return std::make_pair(std::move(name), *value);
}

ExprPtr Parser::Finish(std::shared_ptr<Expr> expr, SourceLocation location)
{
	if (_map != nullptr)
	{
		_map->expressions[expr.get()] = location;
		_map->endLines[expr.get()] = LastConsumedLine();
	}
	return expr;
}

uint32_t Parser::LastConsumedLine() const
{
	for (size_t i = _pos; i > 0; --i)
	{
		if (_tokens[i - 1].kind != TokenKind::EndOfLine)
		{
			return _tokens[i - 1].location.line;
		}
	}
	return Current().location.line;
}

uint32_t Parser::NextTokenLine() const
{
	size_t i = _pos;
	while (i + 1 < _tokens.size() && _tokens[i].kind == TokenKind::EndOfLine)
	{
		++i;
	}
	return _tokens[i].location.line;
}

ExprPtr Parser::MakeNumber(double value, ValueType type, std::string text, SourceLocation location)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Literal;
	expr->type = type;
	expr->number = value;
	expr->text = std::move(text);
	return Finish(expr, location);
}

ExprPtr Parser::MakeValue(double value, ArgType type, SourceLocation location)
{
	auto valueType = ToValueType(type);
	if (valueType == ValueType::Unknown)
	{
		valueType = ValueType::Int;
	}
	std::string text = type == ArgType::Bool ? (value != 0.0 ? "true" : "false") : FormatNumber(static_cast<float>(value));
	return MakeNumber(value, valueType, std::move(text), location);
}

bool Parser::MatchWords(std::string_view text)
{
	for (const auto word : SplitWords(text))
	{
		if (!IsWord(Current(), word))
		{
			NoteExpected(fmt::format("'{}'", word));
			return false;
		}
		Advance();
	}
	return true;
}

void Parser::DefaultArguments(const std::vector<PatternItem>& items, MatchState& state)
{
	for (const auto& item : items)
	{
		if (item.kind == PatternItemKind::Argument && item.argument >= 0)
		{
			const auto type = ParameterType(*state.form->signature, static_cast<size_t>(item.argument));
			state.args[static_cast<size_t>(item.argument)] = MakeValue(item.value.value_or(0.0), type, state.location);
		}
		else if (item.kind == PatternItemKind::Optional)
		{
			DefaultArguments(item.items, state);
		}
		else if ((item.kind == PatternItemKind::Flag || item.kind == PatternItemKind::Fixed ||
		          item.kind == PatternItemKind::Choice) &&
		         item.argument >= 0)
		{
			const auto type = ParameterType(*state.form->signature, static_cast<size_t>(item.argument));
			const double value = item.kind == PatternItemKind::Fixed && item.value ? *item.value : 0.0;
			state.args[static_cast<size_t>(item.argument)] = MakeValue(value, type, state.location);
		}
	}
}

bool Parser::MatchItems(const std::vector<PatternItem>& items, size_t index, MatchState& state,
                        const std::function<bool()>& rest)
{
	if (index == items.size())
	{
		return rest();
	}
	const auto& item = items[index];
	const auto saved = _pos;
	const auto next = [&]() { return MatchItems(items, index + 1, state, rest); };
	const auto argType = [&](int argument) { return ParameterType(*state.form->signature, static_cast<size_t>(argument)); };
	switch (item.kind)
	{
	case PatternItemKind::Word:
		if (!MatchWords(item.text))
		{
			_pos = saved;
			return false;
		}
		if (next())
		{
			return true;
		}
		_pos = saved;
		return false;
	case PatternItemKind::Argument:
	{
		const auto type = argType(item.argument);
		ExprPtr value;
		if (state.operand != nullptr && index == 0 && &items == &state.form->items)
		{
			value = state.operand;
		}
		else if (item.enumName == "CURRENT_CHALLENGE")
		{
			// The id of the challenge being compiled: nothing is written
			const auto id = ChallengeId();
			if (!id)
			{
				NoteExpected(fmt::format("a known challenge (\"CHALLENGE_{}\" is not a constant)", _challenge));
				_pos = saved;
				return false;
			}
			value = MakeNumber(*id, ValueType::Int, "CHALLENGE_" + _challenge, state.location);
		}
		else if (item.enumName == "CAMERA" && Current().kind == TokenKind::Identifier &&
		         LookupConstant(_challenge + Current().text))
		{
			// Camera enums are named after their challenge
			const auto name = _challenge + Current().text;
			value = MakeNumber(*LookupConstant(name), ValueType::Int, Current().text, Current().location);
			Advance();
		}
		else
		{
			value = ParseArgument(type);
			if (value == nullptr)
			{
				NoteExpected(state.form->signature->params.size() > static_cast<size_t>(item.argument)
				                 ? fmt::format("<{}>", state.form->signature->params[item.argument].name)
				                 : "an argument");
				_pos = saved;
				return false;
			}
		}
		if (!Fits(value->type, type))
		{
			_pos = saved;
			return false;
		}
		const auto previous = state.args[static_cast<size_t>(item.argument)];
		state.args[static_cast<size_t>(item.argument)] = value;
		if (next())
		{
			return true;
		}
		state.args[static_cast<size_t>(item.argument)] = previous;
		_pos = saved;
		return false;
	}
	case PatternItemKind::Fixed:
		state.args[static_cast<size_t>(item.argument)] =
		    MakeValue(item.value.value_or(0.0), argType(item.argument), state.location);
		return next();
	case PatternItemKind::Flag:
		if (MatchWords(item.text))
		{
			state.args[static_cast<size_t>(item.argument)] = MakeValue(1.0, argType(item.argument), state.location);
			if (next())
			{
				return true;
			}
		}
		_pos = saved;
		state.args[static_cast<size_t>(item.argument)] = MakeValue(0.0, argType(item.argument), state.location);
		if (next())
		{
			return true;
		}
		_pos = saved;
		return false;
	case PatternItemKind::Choice:
		for (const auto& [words, value] : item.choices)
		{
			_pos = saved;
			if (!MatchWords(words))
			{
				continue;
			}
			state.args[static_cast<size_t>(item.argument)] = MakeValue(value, argType(item.argument), state.location);
			if (next())
			{
				return true;
			}
		}
		_pos = saved;
		return false;
	case PatternItemKind::Optional:
	{
		// Present first, then absent
		const bool present = MatchItems(item.items, 0, state, [&]() {
			if (item.argument >= 0 && !item.optionalArgument)
			{
				state.args[static_cast<size_t>(item.argument)] = MakeValue(1.0, argType(item.argument), state.location);
			}
			return next();
		});
		if (present)
		{
			return true;
		}
		_pos = saved;
		DefaultArguments(item.items, state);
		if (item.argument >= 0)
		{
			state.args[static_cast<size_t>(item.argument)] = MakeValue(0.0, argType(item.argument), state.location);
		}
		if (next())
		{
			return true;
		}
		_pos = saved;
		return false;
	}
	}
	return false;
}

std::optional<Parser::FormMatch> Parser::TryForm(const CompiledForm& form, const ExprPtr& operand, bool statement,
                                                 SourceLocation location)
{
	const auto start = _pos;
	MatchState state {.form = &form,
	                  .args = std::vector<ExprPtr>(form.signature->params.size()),
	                  .operand = operand,
	                  .location = location,
	                  .statement = statement};
	std::optional<FormMatch> result;
	const bool matched = MatchItems(form.items, 0, state, [&]() {
		if (statement && !AtLineEnd())
		{
			NoteExpected("the end of the line");
			return false;
		}
		result = FormMatch {.form = &form, .args = state.args, .end = _pos};
		return true;
	});
	_pos = start;
	if (!matched)
	{
		return std::nullopt;
	}
	// Arguments the pattern doesn't mention take their type's zero
	for (size_t i = 0; i < result->args.size(); ++i)
	{
		if (result->args[i] == nullptr)
		{
			result->args[i] = MakeValue(0.0, ParameterType(*form.signature, i), location);
		}
	}
	return result;
}

ExprPtr Parser::BuildCall(const FormMatch& match, SourceLocation location)
{
	auto call = std::make_shared<Expr>();
	call->kind = ExprKind::NativeCall;
	call->text = std::string(match.form->signature->name);
	call->type = ToValueType(ResultType(*match.form->signature));
	if (match.form->source->swapped && match.form->signature->name == "GET_PROPERTY")
	{
		// "Thing is PROPERTY" tests the property
		call->type = ValueType::Bool;
	}
	call->args = match.args;
	call->swapped = match.form->source->swapped;
	call->sideEffects = true;
	call->number = static_cast<double>(match.form->native);
	auto result = Finish(call, location);
	if (match.form->negated)
	{
		auto negation = std::make_shared<Expr>();
		negation->kind = ExprKind::Unary;
		negation->op = Op::Not;
		negation->type = ValueType::Bool;
		negation->args = {result};
		result = Finish(negation, location);
	}
	return result;
}

std::optional<Parser::FormMatch> Parser::Choose(std::vector<FormMatch>& matches, bool reportAmbiguity, SourceLocation location)
{
	if (matches.empty())
	{
		return std::nullopt;
	}
	size_t best = 0;
	for (size_t i = 1; i < matches.size(); ++i)
	{
		if (matches[i].end > matches[best].end)
		{
			best = i;
		}
	}
	if (reportAmbiguity)
	{
		for (size_t i = 0; i < matches.size(); ++i)
		{
			if (i != best && matches[i].end == matches[best].end &&
			    matches[i].form->signature->name != matches[best].form->signature->name)
			{
				_sink.Warning(_fileName, location,
				              fmt::format("ambiguous statement: reads as {} (\"{}\") and as {} (\"{}\"); using the first",
				                          matches[best].form->signature->name, matches[best].form->source->pattern,
				                          matches[i].form->signature->name, matches[i].form->source->pattern));
				break;
			}
		}
	}
	return std::move(matches[best]);
}

ExprPtr Parser::ParsePrefixForm(ArgType expected, bool statement)
{
	const auto location = Current().location;
	std::vector<FormMatch> matches;
	for (const auto* form : _forms.Prefix(Canonical(Current().text)))
	{
		const auto result = ResultType(*form->signature);
		if (!statement && !ResultFits(result, expected))
		{
			continue;
		}
		if (auto match = TryForm(*form, nullptr, statement, location))
		{
			matches.push_back(std::move(*match));
		}
	}
	auto best = Choose(matches, statement, location);
	if (!best)
	{
		return nullptr;
	}
	_pos = best->end;
	return BuildCall(*best, location);
}

bool Parser::PostfixAllowed(const CompiledForm& form, ArgType expected)
{
	const auto result = ResultType(*form.signature);
	switch (expected)
	{
	case ArgType::Any:
		return result != ArgType::None;
	case ArgType::Bool:
		return result == ArgType::Bool || (form.source->swapped && form.signature->name == "GET_PROPERTY");
	case ArgType::Float:
		return result == ArgType::Float && !form.source->swapped;
	default:
		return false;
	}
}

ExprPtr Parser::ParsePostfixForms(ExprPtr operand, ArgType expected, bool statement, SourceLocation location)
{
	while (true)
	{
		if (AtLineEnd())
		{
			return operand;
		}
		std::vector<FormMatch> matches;
		for (const auto* form : _forms.Postfix())
		{
			// Quick rejection on the word after the operand
			if (form->items.size() > 1 && form->items[1].kind == PatternItemKind::Word &&
			    !IsWord(Current(), SplitWords(form->items[1].text).front()))
			{
				continue;
			}
			if (!statement && !PostfixAllowed(*form, expected))
			{
				continue;
			}
			if (auto match = TryForm(*form, operand, statement, location))
			{
				if (match->end > _pos)
				{
					matches.push_back(std::move(*match));
				}
			}
		}
		auto best = Choose(matches, statement, location);
		if (!best)
		{
			return operand;
		}
		_pos = best->end;
		operand = BuildCall(*best, location);
		if (statement)
		{
			return operand;
		}
	}
}

ExprPtr Parser::ParseArgument(ArgType type)
{
	switch (type)
	{
	case ArgType::Object:
		return ParseExpression(k_PrecedenceAtom, ArgType::Object);
	case ArgType::Bool:
		return ParseExpression(k_PrecedenceNot, ArgType::Bool);
	case ArgType::String:
		return ParseExpression(k_PrecedenceAtom, ArgType::String);
	case ArgType::Int:
		return ParseExpression(k_PrecedenceUnary, ArgType::Int);
	default:
		return ParseExpression(k_PrecedenceAdditive, type);
	}
}

ExprPtr Parser::ParseExpression(uint8_t minPrecedence, ArgType expected)
{
	const auto location = Current().location;
	auto left = ParseUnary(minPrecedence, expected);
	if (left == nullptr)
	{
		return nullptr;
	}
	while (!AtLineEnd())
	{
		const auto& token = Current();
		if (minPrecedence <= k_PrecedenceComparison && IsWord(token, "seconds") &&
		    (expected == ArgType::Bool || expected == ArgType::Any) && left->type != ValueType::Bool)
		{
			Advance();
			auto elapsed = std::make_shared<Expr>();
			elapsed->kind = ExprKind::Elapsed;
			elapsed->type = ValueType::Bool;
			elapsed->args = {left};
			left = Finish(elapsed, location);
			continue;
		}
		if (minPrecedence <= k_PrecedenceComparison && left->type == ValueType::Vector &&
		    (expected == ArgType::Bool || expected == ArgType::Any))
		{
			if (auto test = ParseDistanceTest(left, location))
			{
				left = test;
				continue;
			}
		}
		if (token.kind != TokenKind::Symbol && !token.Is("and") && !token.Is("or"))
		{
			break;
		}
		const auto op = FindOperator(token.text, false);
		if (!op)
		{
			break;
		}
		const auto& info = GetOperator(*op);
		if (info.precedence < minPrecedence)
		{
			break;
		}
		ArgType operandType = ArgType::Float;
		ValueType resultType = ValueType::Float;
		if (*op == Op::And || *op == Op::Or)
		{
			if (expected != ArgType::Bool && expected != ArgType::Any)
			{
				break;
			}
			operandType = ArgType::Bool;
			resultType = ValueType::Bool;
		}
		else if (info.precedence == k_PrecedenceComparison)
		{
			if (expected != ArgType::Bool && expected != ArgType::Any)
			{
				break;
			}
			operandType = ArgType::Any;
			resultType = ValueType::Bool;
		}
		else if (left->type == ValueType::Vector)
		{
			operandType = (*op == Op::Mul || *op == Op::Div) ? ArgType::Float : ArgType::Coord;
			resultType = ValueType::Vector;
		}
		const auto saved = _pos;
		Advance();
		if (*op == Op::And || *op == Op::Or)
		{
			// A condition may continue on the next line after "and" or "or"
			while (Current().kind == TokenKind::EndOfLine)
			{
				Advance();
			}
		}
		auto right = ParseExpression(static_cast<uint8_t>(info.precedence + 1), operandType);
		if (right == nullptr)
		{
			NoteExpected("an operand");
			_pos = saved;
			return nullptr;
		}
		// Positions add to and subtract from positions, scale by a number on the left and divide by one. Anything else
		// isn't position arithmetic, so the operator belongs to an enclosing expression: in "get distance from A to
		// [B] - 1" the 1 is taken from the distance.
		const bool leftVector = left->type == ValueType::Vector;
		const bool rightVector = right->type == ValueType::Vector;
		if (resultType != ValueType::Bool && (leftVector || rightVector))
		{
			bool valid = false;
			if (*op == Op::Add || *op == Op::Sub)
			{
				valid = leftVector && rightVector;
			}
			else if (*op == Op::Mul)
			{
				valid = !leftVector && rightVector;
			}
			else if (*op == Op::Div)
			{
				valid = leftVector && !rightVector;
			}
			if (!valid)
			{
				_pos = saved;
				break;
			}
			resultType = ValueType::Vector;
		}
		auto binary = std::make_shared<Expr>();
		binary->kind = ExprKind::Binary;
		binary->op = *op;
		binary->type = resultType;
		binary->args = {left, right};
		binary->sideEffects = left->sideEffects || right->sideEffects;
		left = Finish(binary, token.location);
	}
	return left;
}

ExprPtr Parser::ParseUnary(uint8_t minPrecedence, ArgType expected)
{
	const auto location = Current().location;
	const auto saved = _pos;
	if (Current().Is("not") && minPrecedence <= k_PrecedenceNot && (expected == ArgType::Bool || expected == ArgType::Any))
	{
		Advance();
		auto operand = ParseExpression(k_PrecedenceNot, ArgType::Bool);
		if (operand == nullptr)
		{
			_pos = saved;
			return nullptr;
		}
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Unary;
		expr->op = Op::Not;
		expr->type = ValueType::Bool;
		expr->args = {operand};
		return Finish(expr, location);
	}
	if (Current().Is("-"))
	{
		Advance();
		auto operand = ParseExpression(k_PrecedenceUnary, expected == ArgType::Coord ? ArgType::Coord : ArgType::Float);
		if (operand == nullptr)
		{
			_pos = saved;
			return nullptr;
		}
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Unary;
		expr->op = Op::Neg;
		expr->type = operand->type == ValueType::Vector ? ValueType::Vector : ValueType::Float;
		expr->args = {operand};
		return Finish(expr, location);
	}
	// Conditions compare and test values of every type
	const auto operandType = expected == ArgType::Bool ? ArgType::Any : expected;
	auto primary = ParsePrimary(operandType);
	if (primary == nullptr)
	{
		return nullptr;
	}
	return ParsePostfixForms(primary, operandType, false, location);
}

ExprPtr Parser::ParsePrimary(ArgType expected)
{
	const auto& token = Current();
	const auto location = token.location;
	const auto saved = _pos;
	switch (token.kind)
	{
	case TokenKind::Number:
		Advance();
		return MakeNumber(token.number, token.integer ? ValueType::Int : ValueType::Float, token.text, location);
	case TokenKind::String:
	{
		Advance();
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Literal;
		expr->type = ValueType::String;
		expr->text = token.text;
		return Finish(expr, location);
	}
	case TokenKind::Symbol:
		if (token.Is("("))
		{
			Advance();
			auto inner = ParseExpression(k_PrecedenceOr, expected == ArgType::Object ? ArgType::Any : expected);
			if (inner == nullptr || !Accept(")"))
			{
				NoteExpected("')'");
				_pos = saved;
				return nullptr;
			}
			return inner;
		}
		if (token.Is("["))
		{
			return ParseCoordinate();
		}
		NoteExpected("an expression");
		return nullptr;
	case TokenKind::Identifier:
		break;
	default:
		NoteExpected("an expression");
		return nullptr;
	}

	const auto& name = token.text;
	if (name == "true" || name == "false")
	{
		Advance();
		return MakeNumber(name == "true" ? 1.0 : 0.0, ValueType::Bool, name, location);
	}
	if (name == "variable")
	{
		Advance();
		auto operand = ParsePrimary(ArgType::Int);
		if (operand == nullptr)
		{
			_pos = saved;
			return nullptr;
		}
		auto cast = std::make_shared<Expr>();
		cast->kind = ExprKind::Cast;
		cast->type = ValueType::Float;
		cast->args = {operand};
		return Finish(cast, location);
	}
	if (name == "constant" && !At(_pos + 1).Is("from"))
	{
		// "constant" takes a whole arithmetic expression: constant A + B is constant (A + B)
		Advance();
		auto operand = ParseExpression(k_PrecedenceAdditive, ArgType::Float);
		if (operand == nullptr)
		{
			_pos = saved;
			return nullptr;
		}
		auto cast = std::make_shared<Expr>();
		cast->kind = ExprKind::Cast;
		cast->type = ValueType::Int;
		cast->args = {operand};
		return Finish(cast, location);
	}
	if (name == "native")
	{
		return ParseNativeCall();
	}
	if (IsKeyword(name) && !IsVariable(name))
	{
		auto form = ParsePrefixForm(expected, false);
		if (form == nullptr)
		{
			_pos = saved;
		}
		return form;
	}
	if (IsVariable(name))
	{
		Advance();
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Variable;
		expr->type = ValueType::Unknown;
		expr->text = name;
		return Finish(expr, location);
	}
	if (const auto value = LookupConstant(name))
	{
		Advance();
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Constant;
		expr->type = ValueType::Int;
		expr->text = name;
		expr->number = static_cast<double>(*value);
		return Finish(expr, location);
	}
	if (expected == ArgType::Int)
	{
		// Camera enums are named after their challenge
		if (const auto value = LookupConstant(_challenge + name))
		{
			Advance();
			return MakeNumber(*value, ValueType::Int, name, location);
		}
	}
	NoteExpected(fmt::format("a known name (\"{}\" is not a variable or constant)", name));
	return nullptr;
}

ExprPtr Parser::ParseDistanceTest(const ExprPtr& left, SourceLocation location)
{
	const auto saved = _pos;
	const bool negated = Accept("not");
	const bool near = Current().Is("near");
	if (!near && !Current().Is("at"))
	{
		_pos = saved;
		return nullptr;
	}
	Advance();
	auto right = ParseExpression(k_PrecedenceAdditive, ArgType::Coord);
	if (right == nullptr || right->type != ValueType::Vector)
	{
		NoteExpected("a position");
		_pos = saved;
		return nullptr;
	}
	const auto native = _env.FindNative("GET_DISTANCE", 2);
	auto distance = std::make_shared<Expr>();
	distance->kind = ExprKind::NativeCall;
	distance->text = "GET_DISTANCE";
	distance->type = ValueType::Float;
	distance->args = {left, right};
	distance->sideEffects = true;
	distance->number = native ? static_cast<double>(*native) : 0.0;
	ExprPtr limit;
	if (near && Accept("radius"))
	{
		limit = ParseExpression(k_PrecedenceAdditive, ArgType::Float);
		if (limit == nullptr)
		{
			NoteExpected("a radius");
			_pos = saved;
			return nullptr;
		}
	}
	else
	{
		limit = MakeNumber(near ? 1.0 : 0.0, ValueType::Float, near ? "1" : "0", location);
	}
	auto test = std::make_shared<Expr>();
	test->kind = ExprKind::Binary;
	test->op = near ? Op::Lt : Op::Eq;
	test->type = ValueType::Bool;
	test->args = {Finish(distance, location), limit};
	test->sideEffects = true;
	ExprPtr result = Finish(test, location);
	if (negated)
	{
		auto negation = std::make_shared<Expr>();
		negation->kind = ExprKind::Unary;
		negation->op = Op::Not;
		negation->type = ValueType::Bool;
		negation->args = {result};
		result = Finish(negation, location);
	}
	return result;
}

ExprPtr Parser::ParseCoordinate()
{
	const auto location = Current().location;
	const auto saved = _pos;
	Advance();
	auto first = ParseExpression(k_PrecedenceAdditive, ArgType::Any);
	if (first == nullptr)
	{
		_pos = saved;
		return nullptr;
	}
	if (Accept("]"))
	{
		if (!Fits(first->type, ArgType::Object))
		{
			NoteExpected("an object inside [ ]");
			_pos = saved;
			return nullptr;
		}
		const auto native = _env.FindNative("GET_POSITION", 1);
		auto call = std::make_shared<Expr>();
		call->kind = ExprKind::NativeCall;
		call->text = "GET_POSITION";
		call->type = ValueType::Vector;
		call->args = {first};
		call->number = native ? static_cast<double>(*native) : 0.0;
		return Finish(call, location);
	}
	std::vector<ExprPtr> components {first};
	while (Accept(","))
	{
		auto next = ParseExpression(k_PrecedenceAdditive, ArgType::Float);
		if (next == nullptr)
		{
			_pos = saved;
			return nullptr;
		}
		components.push_back(next);
	}
	if (!Accept("]") || components.size() < 2 || components.size() > 3)
	{
		NoteExpected("']'");
		_pos = saved;
		return nullptr;
	}
	auto vector = std::make_shared<Expr>();
	vector->kind = ExprKind::VectorLiteral;
	vector->type = ValueType::Vector;
	vector->args = std::move(components);
	return Finish(vector, location);
}

ExprPtr Parser::ParseNativeCall()
{
	const auto location = Current().location;
	const auto saved = _pos;
	Advance();
	if (Current().kind != TokenKind::Identifier)
	{
		NoteExpected("a native function name");
		_pos = saved;
		return nullptr;
	}
	const auto name = Current().text;
	Advance();
	std::vector<ExprPtr> args;
	if (Accept("("))
	{
		if (!Accept(")"))
		{
			do
			{
				auto arg = ParseExpression(k_PrecedenceOr, ArgType::Any);
				if (arg == nullptr)
				{
					_pos = saved;
					return nullptr;
				}
				args.push_back(arg);
			} while (Accept(","));
			if (!Accept(")"))
			{
				NoteExpected("')'");
				_pos = saved;
				return nullptr;
			}
		}
	}
	const auto native = _env.FindNative(name, static_cast<int>(args.size()));
	if (!native)
	{
		NoteExpected(fmt::format("a native function taking {} arguments (\"{}\" is not one)", args.size(), name));
		_pos = saved;
		return nullptr;
	}
	const auto& signature = _env.Natives()[*native];
	auto call = std::make_shared<Expr>();
	call->kind = ExprKind::NativeCall;
	call->text = name;
	call->type = ToValueType(ResultType(signature));
	call->args = std::move(args);
	call->sideEffects = true;
	call->number = static_cast<double>(*native);
	return Finish(call, location);
}

ExprPtr Parser::ParseFullExpression(ArgType expected, std::string_view context)
{
	ResetFurthest();
	auto expr = ParseExpression(k_PrecedenceOr, expected);
	if (expr == nullptr)
	{
		ReportFurthest(context);
		return nullptr;
	}
	if (expected == ArgType::Bool && expr->type != ValueType::Bool)
	{
		Error(_map->Of(expr.get()), fmt::format("expected a condition in {}", context));
	}
	return expr;
}

} // namespace openblack::lhvm::chl::parser
