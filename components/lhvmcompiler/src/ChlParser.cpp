/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlParser.h"

#include <algorithm>

#include <fmt/format.h>

#include "Parser.h"

namespace openblack::lhvm::chl
{

namespace parser
{

ParsedFile Parser::Run()
{
	ParsedFile file;
	file.fileName = std::string(_fileName);
	while (!AtEnd())
	{
		SkipNewlines();
		if (AtEnd())
		{
			break;
		}
		const auto& token = Current();
		if (token.Is("challenge") && !IsScriptKindStart())
		{
			Advance();
			if (Current().kind == TokenKind::Identifier || Current().kind == TokenKind::Number)
			{
				file.challenge = Current().text;
				_challenge = Current().text;
				Advance();
			}
			ExpectLineEnd();
		}
		else if (token.Is("global"))
		{
			ParseGlobal(file);
		}
		else if (token.Is("define"))
		{
			ParseDefine(file);
		}
		else if (token.Is("run"))
		{
			ParseAutorun(file);
		}
		else if (token.Is("begin"))
		{
			ParseScript(file);
		}
		else
		{
			Error(token.location,
			      fmt::format("expected 'global', 'define', 'run script' or 'begin script', not {}", Describe(token)));
			SkipLine();
		}
	}
	return file;
}

StmtPtr Parser::NewStmt(StmtKind kind, SourceLocation location)
{
	auto stmt = MakeStmt(kind, 0);
	_map->statements[stmt.get()] = location;
	return stmt;
}

bool Parser::AtBlockEnd() const
{
	const auto& token = Current();
	return token.Is("end") || token.Is("elsif") || token.Is("else") || token.Is("when") || token.Is("until") || AtEnd();
}

StmtList Parser::ParseStatements()
{
	StmtList list;
	while (true)
	{
		SkipNewlines();
		if (AtBlockEnd())
		{
			return list;
		}
		if (auto stmt = ParseStatement())
		{
			if (!_map->endLines.contains(stmt.get()))
			{
				_map->endLines[stmt.get()] = LastConsumedLine();
			}
			list.push_back(std::move(stmt));
		}
	}
}

StmtList Parser::ParseHandlers()
{
	StmtList handlers;
	while (true)
	{
		SkipNewlines();
		const auto location = Current().location;
		const bool when = Current().Is("when");
		if (!when && !Current().Is("until"))
		{
			return handlers;
		}
		Advance();
		auto stmt = NewStmt(when ? StmtKind::When : StmtKind::Until, location);
		stmt->expr = ParseFullExpression(ArgType::Bool, when ? "a when condition" : "an until condition");
		ExpectLineEnd();
		stmt->body = ParseStatements();
		handlers.push_back(std::move(stmt));
	}
}

bool Parser::ExpectEnd(std::initializer_list<std::string_view> words, SourceLocation opened, Stmt* stmt)
{
	SkipNewlines();
	const auto location = Current().location;
	if (!Current().Is("end") || !At(_pos + 1).Is(*words.begin()))
	{
		std::string expected = "end";
		for (const auto word : words)
		{
			expected += " " + std::string(word);
		}
		std::string found = Describe(Current());
		if (Current().Is("end") && At(_pos + 1).kind == TokenKind::Identifier)
		{
			found = fmt::format("'end {}'", At(_pos + 1).text);
		}
		Error(location,
		      fmt::format("expected '{}' to close the block opened at line {}, not {}", expected, opened.line, found));
		// Leave a closing "end" for the enclosing block
		if (!Current().Is("end") && !AtEnd())
		{
			SkipLine();
		}
		return false;
	}
	Advance();
	for (const auto word : words)
	{
		if (!Expect(word))
		{
			SkipLine();
			return false;
		}
	}
	if (stmt != nullptr)
	{
		_map->closings[stmt] = location;
	}
	ExpectLineEnd();
	return true;
}

StmtPtr Parser::ParseStatement()
{
	const auto& token = Current();
	const auto location = token.location;
	if (token.Is("if"))
	{
		return ParseIf();
	}
	if (token.Is("while"))
	{
		return ParseWhile();
	}
	if (token.Is("begin"))
	{
		return ParseBegin();
	}
	if (token.Is("wait"))
	{
		Advance();
		Accept("until");
		auto stmt = NewStmt(StmtKind::WaitUntil, location);
		stmt->expr = ParseFullExpression(ArgType::Bool, "wait");
		ExpectLineEnd();
		return stmt;
	}
	if (token.Is("run") && (At(_pos + 1).Is("script") || At(_pos + 1).Is("background")))
	{
		return ParseRunScript();
	}
	if (token.Is("challenge") && (At(_pos + 1).kind == TokenKind::Identifier || At(_pos + 1).kind == TokenKind::Number) &&
	    At(_pos + 2).kind == TokenKind::EndOfLine)
	{
		Advance();
		_challenge = Current().text;
		Advance();
		ExpectLineEnd();
		return nullptr;
	}
	if (token.Is("goto"))
	{
		Advance();
		auto stmt = NewStmt(StmtKind::Goto, location);
		stmt->name = ExpectIdentifier("a label name");
		ExpectLineEnd();
		return stmt;
	}
	if (token.kind == TokenKind::Identifier && At(_pos + 1).Is(":") && At(_pos + 2).kind == TokenKind::EndOfLine)
	{
		auto stmt = NewStmt(StmtKind::Label, location);
		stmt->name = token.text;
		Advance();
		Advance();
		ExpectLineEnd();
		return stmt;
	}
	if (token.kind == TokenKind::Identifier && IsVariable(token.text))
	{
		const auto& next = At(_pos + 1);
		if (next.Is("=") || next.Is("+=") || next.Is("-=") || next.Is("*=") || next.Is("/=") || next.Is("++") || next.Is("--"))
		{
			return ParseAssignment();
		}
	}
	if (auto property = TryPropertyAssignment())
	{
		return property;
	}
	if (token.Is("state"))
	{
		if (auto state = ParseState())
		{
			return state;
		}
	}
	if (token.Is("snapshot") || (token.Is("update") && At(_pos + 1).Is("snapshot")))
	{
		return ParseSnapshot();
	}
	if ((token.Is("set") || token.Is("move")) && At(_pos + 1).Is("camera") && At(_pos + 2).Is("to") && !At(_pos + 3).Is("face"))
	{
		return ParseCameraEnumStatement();
	}
	if (auto play = TryPlay())
	{
		return play;
	}
	return ParseFormStatement();
}

StmtPtr Parser::ParseAssignment()
{
	const auto location = Current().location;
	auto stmt = NewStmt(StmtKind::Assign, location);
	stmt->name = Current().text;
	Advance();
	stmt->op = Current().text;
	Advance();
	if (stmt->op != "++" && stmt->op != "--")
	{
		stmt->expr = ParseFullExpression(ArgType::Float, fmt::format("the value assigned to {}", stmt->name));
		stmt->discardedLoad = stmt->op == "=";
	}
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::TryPropertyAssignment()
{
	const auto& token = Current();
	const bool name = token.kind == TokenKind::Identifier && !IsVariable(token.text) && !IsKeyword(token.text);
	if (!(name || (token.kind == TokenKind::Number && token.integer)) || !At(_pos + 1).Is("of"))
	{
		return nullptr;
	}
	const auto saved = _pos;
	const auto location = token.location;
	auto property = ParsePrimary(ArgType::Int);
	if (property == nullptr || !Accept("of"))
	{
		_pos = saved;
		return nullptr;
	}
	auto object = ParseExpression(k_PrecedenceAtom, ArgType::Object);
	const auto& op = Current();
	if (object == nullptr ||
	    !(op.Is("=") || op.Is("+=") || op.Is("-=") || op.Is("*=") || op.Is("/=") || op.Is("++") || op.Is("--")))
	{
		_pos = saved;
		return nullptr;
	}
	auto stmt = NewStmt(StmtKind::Assign, location);
	stmt->op = op.text;
	Advance();
	ExprPtr value;
	if (stmt->op != "++" && stmt->op != "--")
	{
		value = ParseFullExpression(ArgType::Float, "a property assignment");
	}
	const auto native = _env.FindNative("SET_PROPERTY", 3);
	auto call = std::make_shared<Expr>();
	call->kind = ExprKind::NativeCall;
	call->text = "SET_PROPERTY";
	call->type = ValueType::Unknown;
	call->args = {property, object, value};
	call->sideEffects = true;
	call->number = native ? static_cast<double>(*native) : 0.0;
	stmt->expr = Finish(call, location);
	stmt->discardedLoad = stmt->op == "=";
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::ParseFormStatement()
{
	const auto location = Current().location;
	ResetFurthest();
	const auto start = _pos;
	ExprPtr call;
	if (Current().kind == TokenKind::Identifier && IsKeyword(Current().text) && !IsVariable(Current().text))
	{
		call = ParsePrefixForm(ArgType::None, true);
	}
	if (call == nullptr)
	{
		_pos = start;
		// A statement starting with an operand ("Boy play ..."), or a call written as an expression
		if (auto operand = ParsePrimary(ArgType::Any))
		{
			auto statement = ParsePostfixForms(operand, ArgType::None, true, location);
			if (statement != operand && AtLineEnd())
			{
				call = statement;
			}
			else if (statement == operand && operand->kind == ExprKind::NativeCall && AtLineEnd())
			{
				call = operand;
			}
		}
		if (call == nullptr)
		{
			_pos = start;
		}
	}
	if (call == nullptr)
	{
		ReportFurthest("");
		SkipLine();
		return nullptr;
	}
	auto stmt = NewStmt(StmtKind::Expression, location);
	stmt->expr = call;
	ExpectLineEnd();
	return stmt;
}

ExprPtr Parser::ParseOperand(ArgType type, std::string_view context)
{
	ResetFurthest();
	auto expr = ParseArgument(type);
	if (expr == nullptr || !Fits(expr->type, type))
	{
		if (expr == nullptr)
		{
			ReportFurthest(context);
		}
		else
		{
			Error(_map->Of(expr.get()), fmt::format("this value has the wrong type for {}", context));
		}
		return nullptr;
	}
	return expr;
}

StmtPtr Parser::NewCompound(std::string_view op, SourceLocation location)
{
	auto stmt = NewStmt(StmtKind::Expression, location);
	stmt->op = std::string(op);
	return stmt;
}

StmtPtr Parser::TryPlay()
{
	const auto& token = Current();
	if (token.kind != TokenKind::Identifier || !IsVariable(token.text) || !At(_pos + 1).Is("play"))
	{
		return nullptr;
	}
	const auto location = token.location;
	auto stmt = NewCompound("play", location);
	auto object = ParsePrimary(ArgType::Object);
	Advance();
	auto animation = ParseOperand(ArgType::Int, "play");
	ExprPtr loop;
	if (Accept("loop"))
	{
		loop = ParseOperand(ArgType::Float, "play loop");
	}
	else
	{
		loop = MakeNumber(1.0, ValueType::Float, "1", location);
	}
	if (animation == nullptr || loop == nullptr)
	{
		SkipLine();
		return stmt;
	}
	stmt->args = {object, animation, loop};
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::ParseState()
{
	const auto location = Current().location;
	const auto saved = _pos;
	Advance();
	if (Current().kind != TokenKind::Identifier || !IsVariable(Current().text))
	{
		// "state of Thing" and the like are expressions
		_pos = saved;
		return nullptr;
	}
	auto stmt = NewCompound("state", location);
	auto object = ParsePrimary(ArgType::Object);
	auto state = ParseOperand(ArgType::Int, "state");
	if (state == nullptr)
	{
		SkipLine();
		return stmt;
	}
	stmt->args = {object, state};
	while (true)
	{
		while (Current().kind == TokenKind::EndOfLine &&
		       (At(_pos + 1).Is("position") || At(_pos + 1).Is("float") || At(_pos + 1).Is("ulong")))
		{
			Advance();
		}
		const auto& word = Current();
		if (!word.Is("position") && !word.Is("float") && !word.Is("ulong"))
		{
			break;
		}
		auto function = NewCompound(word.text, word.location);
		Advance();
		if (function->op == "position")
		{
			function->args = {ParseOperand(ArgType::Coord, "state position")};
		}
		else if (function->op == "float")
		{
			function->args = {ParseOperand(ArgType::Float, "state float")};
		}
		else
		{
			auto first = ParseOperand(ArgType::Float, "state ulong");
			Expect(",");
			auto second = ParseOperand(ArgType::Float, "state ulong");
			function->args = {first, second};
		}
		if (std::ranges::any_of(function->args, [](const ExprPtr& arg) { return arg == nullptr; }))
		{
			SkipLine();
			return stmt;
		}
		_map->endLines[function.get()] = LastConsumedLine();
		stmt->body.push_back(std::move(function));
	}
	_map->closings[stmt.get()] = {.line = NextTokenLine(), .column = 0};
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::ParseSnapshot()
{
	const auto location = Current().location;
	const bool update = Accept("update");
	Advance();
	const bool details = update && Accept("details");
	const auto native = details ? "UPDATE_SNAPSHOT_PICTURE" : update ? "UPDATE_SNAPSHOT" : "SNAPSHOT";
	auto stmt = NewStmt(StmtKind::Expression, location);
	std::vector<ExprPtr> args;
	const auto fail = [&]() {
		SkipLine();
		return StmtPtr {};
	};
	if (!update)
	{
		if (!Current().Is("quest") && !Current().Is("challenge"))
		{
			Error(Current().location, fmt::format("expected 'quest' or 'challenge', not {}", Describe(Current())));
			return fail();
		}
		args.push_back(MakeNumber(Current().Is("quest") ? 1.0 : 0.0, ValueType::Bool, Current().text, location));
		Advance();
	}
	if (!update || details)
	{
		// The camera's position and focus when not given
		const auto camera = [&](std::string_view word, std::string_view getter) -> ExprPtr {
			if (Accept(word))
			{
				if (word == "at" && !Expect("position"))
				{
					return nullptr;
				}
				return ParseOperand(ArgType::Coord, "snapshot");
			}
			return MakeCall(getter, {}, location);
		};
		auto position = camera("at", "GET_CAMERA_POSITION");
		auto focus = camera("focus", "GET_CAMERA_FOCUS");
		if (position == nullptr || focus == nullptr)
		{
			return fail();
		}
		args.push_back(position);
		args.push_back(focus);
	}
	for (const auto word : {"success", "alignment"})
	{
		if (!Expect(word))
		{
			return fail();
		}
		auto value = ParseOperand(ArgType::Float, "snapshot");
		if (value == nullptr)
		{
			return fail();
		}
		args.push_back(value);
	}
	auto title = ParseOperand(ArgType::Int, "snapshot title");
	if (title == nullptr)
	{
		return fail();
	}
	args.push_back(title);
	if (details)
	{
		const bool picture = Current().Is("taking") && At(_pos + 1).Is("picture");
		if (picture)
		{
			Advance();
			Advance();
		}
		args.push_back(MakeNumber(picture ? 1.0 : 0.0, ValueType::Bool, picture ? "true" : "false", location));
	}
	else
	{
		// The reminder script, stored by name, and its arguments
		const auto scriptLocation = Current().location;
		const auto script = ExpectIdentifier("the reminder script's name");
		if (script.empty())
		{
			return fail();
		}
		auto name = std::make_shared<Expr>();
		name->kind = ExprKind::Literal;
		name->type = ValueType::String;
		name->text = script;
		args.push_back(Finish(name, scriptLocation));
		size_t count = 0;
		if (Accept("("))
		{
			do
			{
				auto arg = ParseOperand(ArgType::Float, "a reminder script argument");
				if (arg == nullptr)
				{
					return fail();
				}
				args.push_back(arg);
				++count;
			} while (Accept(","));
			if (!Expect(")"))
			{
				return fail();
			}
		}
		args.push_back(MakeNumber(static_cast<double>(count), ValueType::Int, std::to_string(count), location));
	}
	const auto id = ChallengeId();
	if (!id)
	{
		Error(location,
		      fmt::format("unknown challenge \"CHALLENGE_{}\": name the challenge with \"challenge NAME\"", _challenge));
		return fail();
	}
	args.push_back(MakeNumber(*id, ValueType::Int, "CHALLENGE_" + _challenge, location));
	stmt->expr = MakeCall(native, std::move(args), location);
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::ParseCameraEnumStatement()
{
	const auto location = Current().location;
	const bool move = Current().Is("move");
	Advance();
	Advance();
	Advance();
	auto stmt = NewCompound(move ? "move camera to" : "set camera to", location);
	auto camera = ParseCameraEnum();
	if (camera == nullptr)
	{
		SkipLine();
		return stmt;
	}
	stmt->args = {camera};
	if (move)
	{
		if (!Expect("time"))
		{
			SkipLine();
			return stmt;
		}
		auto time = ParseOperand(ArgType::Float, "move camera to");
		if (time == nullptr)
		{
			SkipLine();
			return stmt;
		}
		stmt->args.push_back(time);
	}
	ExpectLineEnd();
	return stmt;
}

ExprPtr Parser::ParseCameraEnum()
{
	const auto& token = Current();
	if (token.kind == TokenKind::Identifier)
	{
		for (const auto& name : {_challenge + token.text, token.text})
		{
			if (const auto value = LookupConstant(name))
			{
				Advance();
				return MakeNumber(*value, ValueType::Int, token.text, token.location);
			}
		}
	}
	return ParseOperand(ArgType::Int, "a camera enum");
}

ExprPtr Parser::MakeCall(std::string_view name, std::vector<ExprPtr> args, SourceLocation location)
{
	const auto native = _env.FindNative(name);
	auto call = std::make_shared<Expr>();
	call->kind = ExprKind::NativeCall;
	call->text = std::string(name);
	call->type = native ? ToValueType(ResultType(_env.Natives()[*native])) : ValueType::Unknown;
	call->args = std::move(args);
	call->sideEffects = true;
	call->number = native ? static_cast<double>(*native) : 0.0;
	return Finish(call, location);
}

StmtPtr Parser::ParseRunScript()
{
	const auto location = Current().location;
	Advance();
	auto stmt = NewStmt(StmtKind::RunScript, location);
	stmt->async = Accept("background");
	Expect("script");
	stmt->name = ExpectIdentifier("a script name");
	if (Accept("("))
	{
		if (!Accept(")"))
		{
			do
			{
				if (auto arg = ParseFullExpression(ArgType::Float, "a script argument"))
				{
					stmt->args.push_back(arg);
				}
				else
				{
					SkipLine();
					return stmt;
				}
			} while (Accept(","));
			Expect(")");
		}
	}
	ExpectLineEnd();
	return stmt;
}

StmtPtr Parser::ParseIf()
{
	const auto location = Current().location;
	auto stmt = NewStmt(StmtKind::If, location);
	std::vector<SourceLocation> branchLocations {location};
	Advance();
	{
		auto& branch = stmt->branches.emplace_back();
		branch.cond = ParseFullExpression(ArgType::Bool, "an if condition");
		ExpectLineEnd();
		branch.body = ParseStatements();
	}
	while (Current().Is("elsif") || Current().Is("else"))
	{
		const auto branchLocation = Current().location;
		const bool isElse = Current().Is("else");
		Advance();
		auto& branch = stmt->branches.emplace_back();
		if (!isElse)
		{
			branch.cond = ParseFullExpression(ArgType::Bool, "an elsif condition");
		}
		ExpectLineEnd();
		branch.body = ParseStatements();
		branchLocations.push_back(branchLocation);
		if (isElse)
		{
			break;
		}
	}
	// Branch addresses are stable now that the vector is complete
	for (size_t i = 0; i < stmt->branches.size(); ++i)
	{
		_map->branches[&stmt->branches[i]] = branchLocations[i];
	}
	ExpectEnd({"if"}, location, stmt.get());
	return stmt;
}

StmtPtr Parser::ParseWhile()
{
	const auto location = Current().location;
	auto stmt = NewStmt(StmtKind::While, location);
	stmt->guarded = true;
	Advance();
	stmt->expr = ParseFullExpression(ArgType::Bool, "a while condition");
	ExpectLineEnd();
	stmt->body = ParseStatements();
	stmt->handlers = ParseHandlers();
	ExpectEnd({"while"}, location, stmt.get());
	return stmt;
}

StmtPtr Parser::ParseBegin()
{
	const auto location = Current().location;
	Advance();
	if (Accept("loop"))
	{
		auto stmt = NewStmt(StmtKind::While, location);
		stmt->guarded = true;
		ExpectLineEnd();
		stmt->body = ParseStatements();
		stmt->handlers = ParseHandlers();
		ExpectEnd({"loop"}, location, stmt.get());
		return stmt;
	}
	auto stmt = NewStmt(StmtKind::Block, location);
	if (Accept("known"))
	{
		if (Accept("cinema"))
		{
			stmt->name = "known cinema";
		}
		else if (Accept("dialogue"))
		{
			stmt->name = "known dialogue";
		}
		else
		{
			Error(Current().location,
			      fmt::format("expected 'cinema' or 'dialogue' after 'begin known', not {}", Describe(Current())));
			SkipLine();
			return nullptr;
		}
	}
	else if (Accept("cinema"))
	{
		stmt->name = "cinema";
	}
	else if (Accept("camera"))
	{
		stmt->name = "camera";
	}
	else if (Accept("dialogue"))
	{
		stmt->name = "dialogue";
	}
	else if (Accept("dual"))
	{
		stmt->name = "dual camera";
		Expect("camera");
		Expect("to");
		for (int i = 0; i < 2; ++i)
		{
			ResetFurthest();
			auto object = ParseExpression(k_PrecedenceAtom, ArgType::Object);
			if (object == nullptr)
			{
				ReportFurthest("begin dual camera");
				SkipLine();
				return nullptr;
			}
			stmt->args.push_back(object);
		}
	}
	else
	{
		Error(Current().location,
		      fmt::format("expected 'loop', 'cinema', 'camera', 'dialogue', 'known' or 'dual' after 'begin', not {}",
		                  Describe(Current())));
		SkipLine();
		return nullptr;
	}
	ExpectLineEnd();
	stmt->body = ParseStatements();

	// "end cinema with dialogue ... end dialogue" closes the cinema early and keeps the dialogue open
	SkipNewlines();
	if ((stmt->name == "cinema" || stmt->name == "camera") && Current().Is("end") && At(_pos + 1).Is(stmt->name) &&
	    At(_pos + 2).Is("with") && At(_pos + 3).Is("dialogue"))
	{
		_map->closings[stmt.get()] = Current().location;
		for (int i = 0; i < 4; ++i)
		{
			Advance();
		}
		ExpectLineEnd();
		stmt->op = "with dialogue";
		stmt->handlers = ParseStatements();
		ExpectEnd({"dialogue"}, location, nullptr);
		return stmt;
	}
	std::vector<std::string_view> words;
	for (const auto word : SplitWords(stmt->name))
	{
		words.push_back(word);
	}
	if (words.size() == 1)
	{
		ExpectEnd({words[0]}, location, stmt.get());
	}
	else
	{
		ExpectEnd({words[0], words[1]}, location, stmt.get());
	}
	return stmt;
}

void Parser::ParseGlobal(ParsedFile& file)
{
	const auto location = Current().location;
	Advance();
	if (Current().Is("constant"))
	{
		// global constant NAME = CONSTANT: an alias resolved while compiling; it leaves no trace in the program
		if (auto alias = ParseAlias())
		{
			_env.AddAlias(alias->first, alias->second);
		}
		return;
	}
	auto name = ExpectIdentifier("a global variable name");
	if (Accept("="))
	{
		// Initial values of globals aren't stored in the program
		while (!AtLineEnd())
		{
			Advance();
		}
	}
	ExpectLineEnd();
	if (name.empty())
	{
		return;
	}
	if (_env.IsGlobal(name))
	{
		Error(location, fmt::format("global {} is declared twice", name));
		return;
	}
	_env.AddGlobal(name);
	file.items.emplace_back(GlobalDeclaration {.name = std::move(name), .location = location});
}

void Parser::ParseDefine(ParsedFile& file)
{
	const auto location = Current().location;
	Advance();
	ScriptDefinition definition;
	definition.location = location;
	const auto kind = ParseScriptKindWords();
	if (!kind)
	{
		SkipLine();
		return;
	}
	definition.kind = *kind;
	definition.name = ExpectIdentifier("a script name");
	definition.params = ParseParameters();
	ExpectLineEnd();
	file.items.emplace_back(std::move(definition));
}

void Parser::ParseAutorun(ParsedFile& file)
{
	const auto location = Current().location;
	Advance();
	if (!Expect("script"))
	{
		SkipLine();
		return;
	}
	auto name = ExpectIdentifier("a script name");
	ExpectLineEnd();
	file.items.emplace_back(AutorunDeclaration {.script = std::move(name), .location = location});
}

std::optional<ScriptKind> Parser::ParseScriptKindWords()
{
	std::string words;
	const auto location = Current().location;
	while (Current().kind == TokenKind::Identifier)
	{
		words += (words.empty() ? "" : " ") + Current().text;
		Advance();
		if (words.ends_with("script"))
		{
			break;
		}
	}
	const auto kind = ParseScriptKind(words);
	if (!kind)
	{
		Error(location, fmt::format("unknown script kind \"{}\"", words));
	}
	return kind;
}

std::vector<std::string> Parser::ParseParameters()
{
	std::vector<std::string> params;
	if (!Accept("("))
	{
		return params;
	}
	if (Accept(")"))
	{
		return params;
	}
	do
	{
		auto name = ExpectIdentifier("a parameter name");
		if (name.empty())
		{
			break;
		}
		params.push_back(std::move(name));
	} while (Accept(","));
	Expect(")");
	return params;
}

void Parser::ParseScript(ParsedFile& file)
{
	const auto location = Current().location;
	Advance();
	ParsedScript parsed;
	parsed.script = std::make_shared<Script>();
	_map = &parsed.sourceMap;
	_map->begin = location;
	auto& script = *parsed.script;
	const auto kind = ParseScriptKindWords();
	if (!kind)
	{
		SkipLine();
	}
	script.kind = kind.value_or(ScriptKind::Script);
	script.name = ExpectIdentifier("a script name");
	script.filename = std::string(_fileName);
	script.params = ParseParameters();
	if (!Current().Is("start"))
	{
		ExpectLineEnd();
	}
	_locals.clear();
	for (const auto& param : script.params)
	{
		if (!_locals.insert(param).second)
		{
			Error(location, fmt::format("parameter {} is declared twice", param));
		}
	}

	// Locals and their initial values, up to "start"
	while (true)
	{
		SkipNewlines();
		if (Current().Is("start") || AtEnd() || Current().Is("end"))
		{
			break;
		}
		const auto localLocation = Current().location;
		if (Current().Is("challenge") && (At(_pos + 1).kind == TokenKind::Identifier || At(_pos + 1).kind == TokenKind::Number))
		{
			// openblack extension: the decompiler names the challenge where a statement needs it
			Advance();
			_challenge = Current().text;
			Advance();
			ExpectLineEnd();
			continue;
		}
		if (Current().Is("constant"))
		{
			if (auto alias = ParseAlias())
			{
				_localConstants[alias->first] = alias->second;
			}
			continue;
		}
		if (Current().kind != TokenKind::Identifier || !At(_pos + 1).Is("="))
		{
			Error(localLocation,
			      fmt::format("expected a local variable declaration (Name = value) or 'start', not {}", Describe(Current())));
			SkipLine();
			continue;
		}
		auto stmt = NewStmt(StmtKind::Declaration, localLocation);
		stmt->name = Current().text;
		Advance();
		Advance();
		stmt->expr = ParseFullExpression(ArgType::Float, fmt::format("the initial value of {}", stmt->name));
		_map->endLines[stmt.get()] = LastConsumedLine();
		ExpectLineEnd();
		if (_locals.contains(stmt->name))
		{
			Error(localLocation, fmt::format("local {} is declared twice", stmt->name));
		}
		_locals.insert(stmt->name);
		script.locals.push_back(std::move(stmt));
	}
	_map->start = Current().location;
	if (!Expect("start"))
	{
		SkipLine();
	}
	script.body = ParseStatements();
	script.handlers = ParseHandlers();
	SkipNewlines();
	_map->end = Current().location;
	if (Accept("end"))
	{
		Expect("script");
		const auto endLocation = Current().location;
		const auto endName = ExpectIdentifier("the script name");
		if (!endName.empty() && endName != script.name)
		{
			Error(endLocation, fmt::format("'end script {}' closes script {}", endName, script.name));
		}
		ExpectLineEnd();
	}
	else
	{
		Error(Current().location, fmt::format("expected 'end script {}', not {}", script.name, Describe(Current())));
		// Resynchronise at the next script
		while (!AtEnd() && !(Current().Is("begin") && At(_pos + 1).kind == TokenKind::Identifier &&
		                     (At(_pos + 1).Is("script") || At(_pos + 1).Is("help") || At(_pos + 2).Is("script"))))
		{
			Advance();
		}
	}
	_locals.clear();
	_localConstants.clear();
	_map = nullptr;
	file.items.emplace_back(std::move(parsed));
}

} // namespace parser

SourceLocation SourceMap::Of(const Stmt* stmt) const
{
	const auto it = statements.find(stmt);
	return it != statements.end() ? it->second : SourceLocation {};
}

uint32_t SourceMap::EndLine(const Stmt* stmt) const
{
	const auto it = endLines.find(stmt);
	return it != endLines.end() ? it->second : Of(stmt).line;
}

uint32_t SourceMap::EndLine(const Expr* expr) const
{
	const auto it = endLines.find(expr);
	return it != endLines.end() ? it->second : Of(expr).line;
}

SourceLocation SourceMap::Of(const Expr* expr) const
{
	const auto it = expressions.find(expr);
	return it != expressions.end() ? it->second : SourceLocation {};
}

ParseEnvironment::ParseEnvironment(std::span<const NativeSignature> natives, const ConstantTable* constants)
    : _natives(natives)
    , _constants(constants)
{
	for (uint32_t i = 0; i < _natives.size(); ++i)
	{
		_nativeIndex.emplace(std::string(_natives[i].name), i);
	}
	_forms = std::make_unique<FormTable>(_natives, _nativeIndex);
}

ParseEnvironment::~ParseEnvironment() = default;

std::optional<uint32_t> ParseEnvironment::FindNative(std::string_view name, int argumentCount) const
{
	const auto [first, last] = _nativeIndex.equal_range(name);
	std::optional<uint32_t> found;
	for (auto it = first; it != last; ++it)
	{
		const auto& signature = _natives[it->second];
		if (argumentCount < 0 || signature.params.size() == static_cast<size_t>(argumentCount) ||
		    (signature.params.empty() && signature.stackIn == argumentCount))
		{
			return it->second;
		}
		found = found.value_or(it->second);
	}
	return argumentCount < 0 ? found : std::nullopt;
}

void ParseEnvironment::AddAlias(std::string name, int32_t value)
{
	_aliases[std::move(name)] = value;
}

std::optional<int32_t> ParseEnvironment::FindAlias(std::string_view name) const
{
	const auto it = _aliases.find(name);
	return it != _aliases.end() ? std::optional(it->second) : std::nullopt;
}

bool ParseEnvironment::IsGlobal(std::string_view name) const
{
	return std::ranges::find(_globals, name) != _globals.end();
}

ParsedFile ParseFile(std::string_view text, std::string_view fileName, ParseEnvironment& environment, DiagnosticSink& sink)
{
	auto tokens = Tokenize(text, fileName, sink);
	return parser::Parser(std::move(tokens), fileName, environment, sink).Run();
}

std::vector<std::string> DescribeFormsOf(const ParseEnvironment& environment, std::string_view native)
{
	std::vector<std::string> result;
	for (const auto* form : environment.Forms().FormsOf(native))
	{
		result.emplace_back(form->source->pattern);
	}
	return result;
}

} // namespace openblack::lhvm::chl
