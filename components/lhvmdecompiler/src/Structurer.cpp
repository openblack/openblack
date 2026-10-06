/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Structurer.h"

#include <cmath>

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include "ChlSyntax.h"

namespace openblack::lhvm::detail
{

namespace
{

constexpr uint8_t k_VectorWidth = 3;

[[nodiscard]] Op ArithmeticOp(Opcode code)
{
	switch (code)
	{
	case Opcode::Add:
		return Op::Add;
	case Opcode::Sub:
		return Op::Sub;
	case Opcode::Mul:
		return Op::Mul;
	case Opcode::Div:
		return Op::Div;
	case Opcode::Mod:
		return Op::Mod;
	case Opcode::Eq:
		return Op::Eq;
	case Opcode::Ne:
		return Op::Ne;
	case Opcode::Ge:
		return Op::Ge;
	case Opcode::Le:
		return Op::Le;
	case Opcode::Gt:
		return Op::Gt;
	case Opcode::Lt:
		return Op::Lt;
	case Opcode::And:
		return Op::And;
	case Opcode::Or:
		return Op::Or;
	default:
		return Op::None;
	}
}

[[nodiscard]] ValueType ToValueType(DataType type)
{
	switch (type)
	{
	case DataType::Int:
		return ValueType::Int;
	case DataType::Float:
		return ValueType::Float;
	case DataType::Vector:
		return ValueType::Vector;
	case DataType::Object:
		return ValueType::Object;
	case DataType::Boolean:
		return ValueType::Bool;
	default:
		return ValueType::Unknown;
	}
}

[[nodiscard]] ValueType ToValueType(ArgType type)
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

[[nodiscard]] ExprPtr MakePlaceholder(std::string text, uint32_t ip)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Placeholder;
	expr->text = std::move(text);
	expr->ips = {ip};
	return expr;
}

[[nodiscard]] ExprPtr MakeVariable(std::string name, uint32_t ip)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Variable;
	expr->type = ValueType::Unknown;
	expr->text = std::move(name);
	expr->ips = {ip};
	return expr;
}

[[nodiscard]] ExprPtr MakeComponent(const ExprPtr& vector, uint8_t index)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Component;
	expr->type = ValueType::Float;
	expr->component = index;
	expr->sideEffects = vector->sideEffects;
	// Only the first component owns the vector's instructions, so they're counted once
	if (index == 0)
	{
		expr->args = {vector};
		return expr;
	}
	auto shared = std::make_shared<Expr>();
	shared->kind = ExprKind::Duplicate;
	shared->type = vector->type;
	shared->args = {vector};
	expr->args = {shared};
	return expr;
}

[[nodiscard]] std::string EscapeString(std::string_view text)
{
	std::string out;
	out.reserve(text.size() + 2);
	out += '"';
	for (const char c : text)
	{
		switch (c)
		{
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			if (static_cast<unsigned char>(c) < 0x20)
			{
				out += fmt::format("\\x{:02x}", static_cast<unsigned char>(c));
			}
			else
			{
				out += c;
			}
		}
	}
	out += '"';
	return out;
}

/// The expression without a cast to `type` on top, keeping the cast's instructions
[[nodiscard]] ExprPtr StripCast(const ExprPtr& expr, ValueType type)
{
	if (expr->kind == ExprKind::Cast && expr->type == type && !expr->args.empty())
	{
		return WithIps(expr->args[0], expr->ips);
	}
	return expr;
}

} // namespace

Structurer::Structurer(const ProgramView& program, const VMScript& script, std::span<const NativeSignature> natives,
                       const DecompileOptions& options, uint32_t firstIp, uint32_t endIp, std::set<uint32_t> labels)
    : _program(program)
    , _script(script)
    , _natives(natives)
    , _options(options)
    , _firstIp(firstIp)
    , _endIp(endIp)
    , _labels(std::move(labels))
{
	for (uint32_t ip = firstIp; ip < endIp; ++ip)
	{
		const auto& instruction = At(ip);
		if (instruction.code == Opcode::Jmp && instruction.data.uintVal <= ip && InScript(instruction.data.uintVal))
		{
			_backJumps[instruction.data.uintVal].push_back(ip);
		}
	}
}

std::string Structurer::LabelName(uint32_t ip)
{
	return fmt::format("label_{}", ip);
}

void Structurer::Diagnose(DiagnosticSeverity severity, uint32_t ip, std::string message)
{
	_diagnostics.push_back({.severity = severity, .ip = ip, .message = std::move(message)});
}

std::string Structurer::VariableName(uint32_t id) const
{
	if (IsLocal(id))
	{
		const auto index = id - _script.variablesOffset - 1;
		if (index < _script.variables.size())
		{
			return _script.variables[index];
		}
		return fmt::format("local_{}", index);
	}
	if (id >= 1 && id - 1 < _program.globalNames.size())
	{
		return _program.globalNames[id - 1];
	}
	return id == 0 ? std::string("null_variable") : fmt::format("global_{}", id);
}

// Expression stack ----------------------------------------------------------------------------------------------------

bool Structurer::SplitTopVector(std::vector<StackEntry>& stack, size_t entryIndex)
{
	if (entryIndex >= stack.size() || stack[entryIndex].width != k_VectorWidth)
	{
		return false;
	}
	const auto vector = stack[entryIndex].expr;
	std::vector<StackEntry> parts;
	for (uint8_t i = 0; i < k_VectorWidth; ++i)
	{
		parts.push_back({.expr = MakeComponent(vector, i), .width = 1});
	}
	stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(entryIndex));
	stack.insert(stack.begin() + static_cast<std::ptrdiff_t>(entryIndex), parts.begin(), parts.end());
	return true;
}

bool Structurer::AlignSlots(std::vector<StackEntry>& stack, size_t slots, size_t& entryIndex)
{
	// Find the entry boundary `slots` slots below the top, splitting a position that straddles it
	for (int attempt = 0; attempt < 8; ++attempt)
	{
		size_t covered = 0;
		size_t index = stack.size();
		while (covered < slots && index > 0)
		{
			--index;
			covered += stack[index].width;
		}
		if (covered == slots)
		{
			entryIndex = index;
			return true;
		}
		if (covered < slots || !SplitTopVector(stack, index))
		{
			return false;
		}
	}
	return false;
}

ExprPtr Structurer::PopSlot(std::vector<StackEntry>& stack, uint32_t ip, bool strict, bool& failed)
{
	if (stack.empty())
	{
		failed = true;
		if (!strict)
		{
			Diagnose(DiagnosticSeverity::Warning, ip, "Value taken from an empty stack");
		}
		return MakePlaceholder("__stack_value", ip);
	}
	if (stack.back().width != 1 && !SplitTopVector(stack, stack.size() - 1))
	{
		failed = true;
		return MakePlaceholder("__stack_value", ip);
	}
	auto expr = stack.back().expr;
	stack.pop_back();
	return expr;
}

ExprPtr Structurer::PopVector(std::vector<StackEntry>& stack, uint32_t ip, bool strict, bool& failed)
{
	if (!stack.empty() && stack.back().width == k_VectorWidth)
	{
		auto expr = stack.back().expr;
		stack.pop_back();
		return expr;
	}
	// Three single slots, usually numbers cast to position components: make them a vector literal
	std::array<ExprPtr, k_VectorWidth> parts;
	bool flat = false;
	for (int i = k_VectorWidth - 1; i >= 0; --i)
	{
		auto part = PopSlot(stack, ip, strict, failed);
		flat = flat || (i == 1 && part->kind == ExprKind::Literal && part->text == "flat");
		if (part->type == ValueType::Vector)
		{
			if (part->kind == ExprKind::Cast)
			{
				part = StripCast(part, ValueType::Vector);
			}
			else
			{
				auto copy = std::make_shared<Expr>(*part);
				copy->type = ValueType::Float;
				part = copy;
			}
		}
		parts[static_cast<size_t>(i)] = std::move(part);
	}
	// Three components of one vector in order are just that vector
	if (parts[0]->kind == ExprKind::Component && parts[1]->kind == ExprKind::Component &&
	    parts[2]->kind == ExprKind::Component && parts[0]->component == 0 && parts[1]->component == 1 &&
	    parts[2]->component == 2 && parts[1]->args[0]->args[0] == parts[0]->args[0] &&
	    parts[2]->args[0]->args[0] == parts[0]->args[0])
	{
		std::vector<uint32_t> ips = parts[0]->ips;
		ips.insert(ips.end(), parts[1]->ips.begin(), parts[1]->ips.end());
		ips.insert(ips.end(), parts[2]->ips.begin(), parts[2]->ips.end());
		return WithIps(parts[0]->args[0], ips);
	}
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::VectorLiteral;
	expr->type = ValueType::Vector;
	if (flat)
	{
		expr->text = "flat";
	}
	for (auto& part : parts)
	{
		expr->sideEffects = expr->sideEffects || part->sideEffects;
		expr->args.push_back(std::move(part));
	}
	return expr;
}

ExprPtr Structurer::PopWidth(std::vector<StackEntry>& stack, uint8_t width, uint32_t ip, bool strict, bool& failed)
{
	return width == k_VectorWidth ? PopVector(stack, ip, strict, failed) : PopSlot(stack, ip, strict, failed);
}

ExprPtr Structurer::ConvertArgument(const ExprPtr& arg, ArgType type)
{
	// Conversions to objects and truth values belong to the statement forms that need them; conversions to integers
	// and numbers are written in the source ("constant X", "variable X") and stay in the tree
	auto result = arg;
	const auto valueType = ToValueType(type);
	if (valueType == ValueType::Object || valueType == ValueType::Bool)
	{
		result = StripCast(result, valueType);
	}
	if (type == ArgType::String && result->kind == ExprKind::Literal && result->type == ValueType::Int && result->number >= 0 &&
	    result->number < static_cast<double>(_program.data.size()))
	{
		const auto offset = static_cast<size_t>(result->number);
		const auto* begin = _program.data.data() + offset;
		const auto* end = std::find(begin, _program.data.data() + _program.data.size(), '\0');
		return MakeLiteral(EscapeString(std::string_view(begin, static_cast<size_t>(end - begin))), ValueType::String,
		                   result->number, result->ips);
	}
	return result;
}

ExprPtr Structurer::MakeNativeCall(uint32_t ip, std::vector<StackEntry>& stack, bool strict, bool& failed, uint32_t& stackOut)
{
	const auto id = At(ip).data.uintVal;
	auto call = std::make_shared<Expr>();
	call->kind = ExprKind::NativeCall;
	call->sideEffects = true;
	call->ips = {ip};
	// Which native it is, to tell apart the few that share a name
	call->number = id;
	if (id >= _natives.size())
	{
		call->text = fmt::format("native_{}", id);
		stackOut = 0;
		Diagnose(DiagnosticSeverity::Error, ip, fmt::format("Unknown native function {}", id));
		return call;
	}
	const auto& native = _natives[id];
	call->text = std::string(native.name);
	call->type = ToValueType(native.returnType);
	stackOut = native.stackOut;
	if (native.stackOut == k_VectorWidth)
	{
		call->type = ValueType::Vector;
	}

	// Slots the declared parameters fill, to check the table against the stack use
	uint32_t declaredSlots = 0;
	std::optional<size_t> varArgsIndex;
	for (size_t i = 0; i < native.params.size(); ++i)
	{
		if (native.params[i].type == ArgType::VarArgs)
		{
			varArgsIndex = i;
		}
		else
		{
			declaredSlots += native.params[i].type == ArgType::Coord ? k_VectorWidth : 1;
		}
	}
	const bool typed = !native.params.empty() &&
	                   (native.stackIn < 0 ? varArgsIndex.has_value() : declaredSlots == static_cast<uint32_t>(native.stackIn));
	if (!typed)
	{
		if (native.stackIn < 0)
		{
			Diagnose(DiagnosticSeverity::Warning, ip, fmt::format("Can't tell how many arguments {} takes", native.name));
			return call;
		}
		std::vector<ExprPtr> args(static_cast<size_t>(native.stackIn));
		for (auto i = static_cast<int32_t>(args.size()) - 1; i >= 0; --i)
		{
			args[static_cast<size_t>(i)] = PopSlot(stack, ip, strict, failed);
		}
		for (auto& arg : args)
		{
			call->args.push_back(std::move(arg));
		}
		return call;
	}

	// Arguments come off the stack last first
	std::vector<ExprPtr> args(native.params.size());
	std::array<const Expr*, 2> rawFirst {};
	std::vector<ExprPtr> varArgs;
	for (auto i = static_cast<int32_t>(native.params.size()) - 1; i >= 0; --i)
	{
		const auto& param = native.params[static_cast<size_t>(i)];
		if (param.type == ArgType::VarArgs)
		{
			// The count is the argument after the run
			const auto& countArg = args[static_cast<size_t>(i) + 1];
			size_t count = 0;
			if (countArg != nullptr && countArg->kind == ExprKind::Literal && countArg->number >= 0 && countArg->number < 32)
			{
				count = static_cast<size_t>(countArg->number);
			}
			else
			{
				Diagnose(DiagnosticSeverity::Warning, ip, fmt::format("Argument count of {} is not a constant", native.name));
			}
			varArgs.resize(count);
			for (auto j = static_cast<int32_t>(count) - 1; j >= 0; --j)
			{
				varArgs[static_cast<size_t>(j)] = PopSlot(stack, ip, strict, failed);
			}
			continue;
		}
		auto raw = PopWidth(stack, param.type == ArgType::Coord ? k_VectorWidth : 1, ip, strict, failed);
		if (i <= 1)
		{
			rawFirst[static_cast<size_t>(i)] = raw.get();
		}
		args[static_cast<size_t>(i)] = ConvertArgument(raw, param.type);
	}
	// Some forms push their first two arguments the other way round and swap them just before the call
	call->swapped = _swapIp + 1 == ip && rawFirst[0] != nullptr && rawFirst[0] == _swapBelow && rawFirst[1] == _swapTop;
	for (size_t i = 0; i < args.size(); ++i)
	{
		if (native.params[i].type == ArgType::VarArgs)
		{
			for (auto& arg : varArgs)
			{
				call->args.push_back(std::move(arg));
			}
		}
		else
		{
			call->args.push_back(std::move(args[i]));
		}
	}
	return call;
}

bool Structurer::IsPure(uint32_t ip) const
{
	const auto& instruction = At(ip);
	switch (instruction.code)
	{
	case Opcode::Push:
	case Opcode::Add:
	case Opcode::Sub:
	case Opcode::Mul:
	case Opcode::Div:
	case Opcode::Mod:
	case Opcode::Neg:
	case Opcode::Not:
	case Opcode::And:
	case Opcode::Or:
	case Opcode::Eq:
	case Opcode::Ne:
	case Opcode::Ge:
	case Opcode::Le:
	case Opcode::Gt:
	case Opcode::Lt:
	case Opcode::Sleep:
	case Opcode::Swap:
	case Opcode::Line:
		return true;
	case Opcode::Cast:
		return instruction.mode != VMMode::Zero;
	case Opcode::Sys:
		return instruction.data.uintVal < _natives.size() && _natives[instruction.data.uintVal].stackOut > 0;
	default:
		return false;
	}
}

uint32_t Structurer::FirstControl(uint32_t lo, uint32_t hi) const
{
	uint32_t ip = lo;
	while (ip < hi && IsPure(ip))
	{
		++ip;
	}
	return ip;
}

bool Structurer::ExecPure(uint32_t ip, std::vector<StackEntry>& stack, bool strict)
{
	if (!IsPure(ip))
	{
		return false;
	}
	const auto& instruction = At(ip);
	bool failed = false;
	switch (instruction.code)
	{
	case Opcode::Push:
	{
		if (instruction.mode == VMMode::Reference)
		{
			stack.push_back({.expr = MakeVariable(VariableName(instruction.data.uintVal), ip), .width = 1});
			break;
		}
		ExprPtr literal;
		switch (instruction.type)
		{
		case DataType::Int:
			literal = MakeLiteral(std::to_string(instruction.data.intVal), ValueType::Int, instruction.data.intVal, {ip});
			break;
		case DataType::Float:
			literal = MakeLiteral(FormatNumber(instruction.data.floatVal), ValueType::Float, instruction.data.floatVal, {ip});
			break;
		case DataType::Vector:
			// One component of a position
			literal = MakeLiteral(FormatNumber(instruction.data.floatVal), ValueType::Vector, instruction.data.floatVal, {ip});
			break;
		case DataType::Boolean:
			literal = MakeLiteral(instruction.data.intVal != 0 ? "true" : "false", ValueType::Bool,
			                      instruction.data.intVal != 0 ? 1.0 : 0.0, {ip});
			break;
		case DataType::Object:
			literal = MakeLiteral(instruction.data.uintVal == 0 ? "null" : std::to_string(instruction.data.uintVal),
			                      ValueType::Object, instruction.data.uintVal, {ip});
			break;
		default:
			literal = MakeLiteral(std::to_string(instruction.data.intVal), ValueType::Unknown, instruction.data.intVal, {ip});
			break;
		}
		stack.push_back({.expr = literal, .width = 1});
		break;
	}
	case Opcode::Add:
	case Opcode::Sub:
	case Opcode::Mul:
	case Opcode::Div:
	case Opcode::Mod:
	{
		const auto op = ArithmeticOp(instruction.code);
		if (instruction.type == DataType::Vector)
		{
			ExprPtr left;
			ExprPtr right;
			if (instruction.code == Opcode::Add || instruction.code == Opcode::Sub)
			{
				right = PopVector(stack, ip, strict, failed);
				left = PopVector(stack, ip, strict, failed);
			}
			else if (instruction.code == Opcode::Mul)
			{
				// The vector is on top of the scalar
				right = PopVector(stack, ip, strict, failed);
				left = PopSlot(stack, ip, strict, failed);
			}
			else
			{
				right = PopSlot(stack, ip, strict, failed);
				left = PopVector(stack, ip, strict, failed);
			}
			stack.push_back({.expr = MakeBinary(op, left, right, ValueType::Vector, {ip}), .width = k_VectorWidth});
			break;
		}
		auto right = PopSlot(stack, ip, strict, failed);
		auto left = PopSlot(stack, ip, strict, failed);
		const auto type = instruction.type == DataType::Int     ? ValueType::Int
		                  : instruction.type == DataType::Float ? ValueType::Float
		                                                        : ValueType::Unknown;
		stack.push_back({.expr = MakeBinary(op, left, right, type, {ip}), .width = 1});
		break;
	}
	case Opcode::Neg:
	{
		if (instruction.type == DataType::Vector)
		{
			auto operand = PopVector(stack, ip, strict, failed);
			stack.push_back({.expr = MakeUnary(Op::Neg, operand, {ip}), .width = k_VectorWidth});
			break;
		}
		auto operand = PopSlot(stack, ip, strict, failed);
		// Only a constant pushed as it is: negating an already negative one is written -(-N)
		if (operand->kind == ExprKind::Literal && (operand->type == ValueType::Int || operand->type == ValueType::Float) &&
		    !std::signbit(operand->number))
		{
			// Negative constants are compiled as a positive one and a negation
			auto ips = operand->ips;
			ips.push_back(ip);
			const auto value = -operand->number;
			auto text = operand->type == ValueType::Int ? std::to_string(static_cast<int32_t>(value))
			                                            : FormatNumber(static_cast<float>(value));
			stack.push_back({.expr = MakeLiteral(text, operand->type, value, ips), .width = 1});
			break;
		}
		stack.push_back({.expr = MakeUnary(Op::Neg, operand, {ip}), .width = 1});
		break;
	}
	case Opcode::Not:
		// Kept as written: "not (A == B)" and "A != B" compile differently
		stack.push_back({.expr = MakeUnary(Op::Not, PopSlot(stack, ip, strict, failed), {ip}), .width = 1});
		break;
	case Opcode::And:
	case Opcode::Or:
	{
		auto right = PopSlot(stack, ip, strict, failed);
		auto left = PopSlot(stack, ip, strict, failed);
		stack.push_back({.expr = MakeBinary(ArithmeticOp(instruction.code), left, right, ValueType::Bool, {ip}), .width = 1});
		break;
	}
	case Opcode::Eq:
	case Opcode::Ne:
	case Opcode::Ge:
	case Opcode::Le:
	case Opcode::Gt:
	case Opcode::Lt:
	{
		const bool equality = instruction.code == Opcode::Eq || instruction.code == Opcode::Ne;
		ExprPtr left;
		ExprPtr right;
		switch (instruction.type)
		{
		case DataType::Int:
		case DataType::Float:
		case DataType::Boolean:
		case DataType::Object:
			right = PopSlot(stack, ip, strict, failed);
			left = PopSlot(stack, ip, strict, failed);
			break;
		case DataType::Vector:
			if (equality)
			{
				right = PopVector(stack, ip, strict, failed);
				left = PopVector(stack, ip, strict, failed);
				break;
			}
			[[fallthrough]];
		default:
			// The machine compares nothing and answers false; equality tests still take their two values
			if (equality)
			{
				(void)PopSlot(stack, ip, strict, failed);
				(void)PopSlot(stack, ip, strict, failed);
			}
			Diagnose(DiagnosticSeverity::Warning, ip, "Comparison of an unsupported type, always false");
			stack.push_back({.expr = MakeLiteral("false", ValueType::Bool, 0.0, {ip}), .width = 1});
			return !(failed && strict);
		}
		stack.push_back({.expr = MakeBinary(ArithmeticOp(instruction.code), left, right, ValueType::Bool, {ip}), .width = 1});
		break;
	}
	case Opcode::Sleep:
	{
		auto seconds = PopSlot(stack, ip, strict, failed);
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Elapsed;
		expr->type = ValueType::Bool;
		expr->sideEffects = true;
		expr->args = {seconds};
		expr->ips = {ip};
		stack.push_back({.expr = expr, .width = 1});
		break;
	}
	case Opcode::Cast:
	{
		const auto type = ToValueType(instruction.type);
		if (type == ValueType::Unknown)
		{
			// Leaves the value as it was: give the instruction to the value
			auto value = PopSlot(stack, ip, strict, failed);
			stack.push_back({.expr = WithIps(value, {ip}), .width = 1});
			break;
		}
		auto value = PopSlot(stack, ip, strict, failed);
		auto expr = std::make_shared<Expr>();
		expr->kind = ExprKind::Cast;
		expr->type = type;
		expr->sideEffects = value->sideEffects;
		expr->args = {value};
		expr->ips = {ip};
		stack.push_back({.expr = expr, .width = 1});
		break;
	}
	case Opcode::Sys:
	{
		uint32_t stackOut = 0;
		auto call = MakeNativeCall(ip, stack, strict, failed, stackOut);
		if (stackOut == k_VectorWidth || call->type == ValueType::Vector)
		{
			stack.push_back({.expr = call, .width = k_VectorWidth});
			for (uint32_t i = k_VectorWidth; i < stackOut; ++i)
			{
				stack.push_back({.expr = MakePlaceholder("__extra_result", ip), .width = 1});
			}
		}
		else
		{
			stack.push_back({.expr = call, .width = 1});
			for (uint32_t i = 1; i < stackOut; ++i)
			{
				stack.push_back({.expr = MakePlaceholder("__extra_result", ip), .width = 1});
			}
		}
		break;
	}
	case Opcode::Swap:
	{
		if (instruction.type == DataType::Int)
		{
			// Exchange the two topmost slots
			size_t index = 0;
			if (!AlignSlots(stack, 2, index) || stack.size() - index != 2)
			{
				if (stack.size() >= 2 && stack.back().width == 1)
				{
					(void)SplitTopVector(stack, stack.size() - 2);
				}
				if (!AlignSlots(stack, 2, index) || stack.size() - index != 2)
				{
					failed = true;
					if (!strict)
					{
						Diagnose(DiagnosticSeverity::Warning, ip, "Swap of values that aren't on the stack");
					}
					break;
				}
			}
			std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
			stack.back().expr = WithIps(stack.back().expr, {ip});
			// A zero height moved under the last number: the compiler's way of writing a ground position [x, z]
			auto& moved = stack[stack.size() - 2];
			if (moved.expr->kind == ExprKind::Literal && moved.expr->type == ValueType::Vector && moved.expr->number == 0.0 &&
			    stack.back().expr->type == ValueType::Vector)
			{
				auto flat = std::make_shared<Expr>(*moved.expr);
				flat->text = "flat";
				moved.expr = flat;
			}
			_swapIp = ip;
			_swapTop = stack.back().expr.get();
			_swapBelow = moved.expr.get();
			break;
		}
		const auto offset = static_cast<size_t>(instruction.data.uintVal);
		size_t index = 0;
		if (offset == 0 || !AlignSlots(stack, offset, index) || index >= stack.size())
		{
			failed = true;
			if (!strict)
			{
				Diagnose(DiagnosticSeverity::Warning, ip, "Copy of a value that isn't on the stack");
			}
			break;
		}
		auto duplicate = std::make_shared<Expr>();
		duplicate->kind = ExprKind::Duplicate;
		duplicate->ips = {ip};
		if (instruction.mode == VMMode::CopyFrom)
		{
			// Push a copy of the value `offset` slots down
			const auto& source = stack[index];
			duplicate->type = source.expr->type;
			duplicate->sideEffects = source.expr->sideEffects;
			duplicate->args = {source.expr};
			stack.push_back({.expr = duplicate, .width = source.width});
		}
		else
		{
			// Insert a copy of the top value under the `offset` slots at the top
			const auto& source = stack.back();
			duplicate->type = source.expr->type;
			duplicate->sideEffects = source.expr->sideEffects;
			duplicate->args = {source.expr};
			const auto width = source.width;
			stack.insert(stack.begin() + static_cast<std::ptrdiff_t>(index), {.expr = duplicate, .width = width});
		}
		if (duplicate->sideEffects && !strict)
		{
			Diagnose(DiagnosticSeverity::Info, ip, "A value with side effects is used twice");
		}
		break;
	}
	case Opcode::Line:
		// Source line markers carry no code; they're credited to whatever comes next
		if (!stack.empty())
		{
			stack.back().expr = WithIps(stack.back().expr, {ip});
		}
		else
		{
			failed = true;
		}
		break;
	default:
		return false;
	}
	return !(failed && strict);
}

ExprPtr Structurer::EvalCondition(uint32_t lo, uint32_t hi)
{
	std::vector<StackEntry> stack;
	for (uint32_t ip = lo; ip < hi; ++ip)
	{
		if (!ExecPure(ip, stack, true))
		{
			return nullptr;
		}
	}
	if (stack.size() != 1 || stack.back().width != 1)
	{
		return nullptr;
	}
	return stack.back().expr;
}

// Statements ----------------------------------------------------------------------------------------------------------

void Structurer::AddStmt(SequenceState& state, StmtPtr stmt)
{
	if (!state.pending.empty())
	{
		stmt->ips.insert(stmt->ips.begin(), state.pending.begin(), state.pending.end());
		stmt->startIp = std::min(stmt->startIp, state.stmtStart);
		state.pending.clear();
	}
	state.droppedLoad.clear();
	state.out.push_back(std::move(stmt));
}

void Structurer::FlushStack(SequenceState& state)
{
	for (auto& entry : state.stack)
	{
		const auto ip = entry.expr->ips.empty() ? state.stmtStart : entry.expr->ips.front();
		Diagnose(DiagnosticSeverity::Warning, ip, "Value left on the stack");
		auto stmt = MakeStmt(StmtKind::Expression, state.stmtStart);
		stmt->name = "value left on the stack";
		CollectIps(entry.expr, stmt->ips);
		stmt->expr = std::move(entry.expr);
		AddStmt(state, std::move(stmt));
	}
	state.stack.clear();
}

std::optional<uint32_t> Structurer::FindLoopBackJump(uint32_t ip, uint32_t hi) const
{
	if (std::ranges::any_of(_loops, [ip](const auto& loop) { return loop.head == ip; }))
	{
		return std::nullopt;
	}
	const auto it = _backJumps.find(ip);
	if (it == _backJumps.end())
	{
		return std::nullopt;
	}
	std::optional<uint32_t> best;
	for (const auto source : it->second)
	{
		if (source >= ip && source < hi && (!best || source > *best))
		{
			best = source;
		}
	}
	return best;
}

uint32_t Structurer::ParseLoop(SequenceState& state, uint32_t ip, uint32_t backJump)
{
	const auto exit = backJump + 1;
	auto loop = MakeStmt(StmtKind::While, state.stmtStart);
	uint32_t bodyStart = ip;

	// A while loop starts with its condition, which jumps out past the backward jump
	const auto control = FirstControl(ip, backJump);
	if (control < backJump && At(control).code == Opcode::Wait && Target(control) == exit)
	{
		if (auto cond = EvalCondition(ip, control))
		{
			loop->expr = std::move(cond);
			for (uint32_t i = ip; i <= control; ++i)
			{
				loop->ips.push_back(i);
			}
			bodyStart = control + 1;
		}
	}

	std::vector<uint32_t> trailing;
	_loops.push_back({.head = ip, .exit = exit});
	loop->body = ParseSequence(bodyStart, backJump, trailing);
	_loops.pop_back();
	loop->closeIps = std::move(trailing);
	loop->closeIps.push_back(backJump);
	AddStmt(state, std::move(loop));
	return exit;
}

void Structurer::AddGoto(SequenceState& state, ExprPtr negatedCond, uint32_t ip, uint32_t target)
{
	++_gotoCount;
	_wantedLabels.insert(target);
	if (!InScript(target))
	{
		Diagnose(DiagnosticSeverity::Error, ip, "Jump out of the script");
	}
	else
	{
		Diagnose(DiagnosticSeverity::Info, ip, "Control flow needs a goto");
	}
	auto jump = MakeStmt(StmtKind::Goto, state.stmtStart);
	jump->name = LabelName(target);
	if (negatedCond == nullptr)
	{
		jump->ips.push_back(ip);
		AddStmt(state, std::move(jump));
		return;
	}
	auto branchIf = MakeStmt(StmtKind::If, state.stmtStart);
	Branch branch;
	branch.cond = std::move(negatedCond);
	CollectIps(branch.cond, branch.ips);
	branch.body.push_back(std::move(jump));
	branchIf->branches.push_back(std::move(branch));
	AddStmt(state, std::move(branchIf));
}

bool Structurer::LeadsTo(uint32_t from, uint32_t target) const
{
	// Control reaching `from` goes on to `target` through forward jumps alone
	for (int guard = 0; guard < 16 && from < target && InScript(from); ++guard)
	{
		if (At(from).code != Opcode::Jmp || Target(from) <= from)
		{
			return false;
		}
		from = Target(from);
	}
	return from == target;
}

uint32_t Structurer::ParseIf(SequenceState& state, ExprPtr cond, uint32_t jumpIp, uint32_t hi)
{
	auto target = Target(jumpIp);
	if (target > hi && LeadsTo(hi, target))
	{
		// Jumping straight to where the end of this block jumps to is the same as jumping to its end
		target = hi;
	}
	if (target > hi)
	{
		if (!_loops.empty() && target == _loops.back().exit)
		{
			// Leaving the loop when the condition fails
			auto branchIf = MakeStmt(StmtKind::If, state.stmtStart);
			Branch branch;
			branch.cond = Negate(cond, {jumpIp});
			CollectIps(branch.cond, branch.ips);
			branch.body.push_back(MakeStmt(StmtKind::Break, jumpIp));
			branchIf->branches.push_back(std::move(branch));
			AddStmt(state, std::move(branchIf));
		}
		else
		{
			AddGoto(state, Negate(cond, {jumpIp}), jumpIp, target);
		}
		return jumpIp + 1;
	}

	// The compiler ends a branch with a jump past the rest of the if/else chain, even when there's nothing to skip
	const auto closeIp = target - 1;
	const bool hasClose = closeIp > jumpIp && At(closeIp).code == Opcode::Jmp && Target(closeIp) > closeIp;
	uint32_t thenEnd = target;
	uint32_t chainEnd = target;
	if (hasClose)
	{
		// Follow the jumps that close inner branches to find where the whole chain ends
		uint32_t end = Target(closeIp);
		for (int guard = 0; guard < 64 && end < hi && At(end).code == Opcode::Jmp && Target(end) > end && Target(end) <= hi;
		     ++guard)
		{
			end = Target(end);
		}
		if (end <= hi)
		{
			thenEnd = closeIp;
			chainEnd = end;
		}
	}

	auto branchIf = MakeStmt(StmtKind::If, state.stmtStart);
	Branch first;
	first.cond = std::move(cond);
	first.ips = std::move(state.pending);
	state.pending.clear();
	CollectIps(first.cond, first.ips);
	first.ips.push_back(jumpIp);
	first.body = ParseSequence(jumpIp + 1, thenEnd, first.closeIps);
	if (thenEnd == closeIp)
	{
		first.closeIps.push_back(closeIp);
	}
	branchIf->branches.push_back(std::move(first));

	if (chainEnd > target)
	{
		std::vector<uint32_t> elseTrailing;
		auto elseBody = ParseSequence(target, chainEnd, elseTrailing);
		if (elseBody.size() == 1 && elseBody.front()->kind == StmtKind::If && !elseBody.front()->ips.empty() == false)
		{
			// else { if ... } is an else-if chain
			auto& inner = *elseBody.front();
			for (auto& branch : inner.branches)
			{
				branchIf->branches.push_back(std::move(branch));
			}
		}
		else if (!elseBody.empty())
		{
			Branch otherwise;
			otherwise.body = std::move(elseBody);
			branchIf->branches.push_back(std::move(otherwise));
		}
		auto& last = branchIf->branches.back();
		last.closeIps.insert(last.closeIps.end(), elseTrailing.begin(), elseTrailing.end());
	}

	// `else` is compiled as a final branch whose condition is the constant true
	auto& last = branchIf->branches.back();
	if (branchIf->branches.size() > 1 && IsLiteralTrue(last.cond))
	{
		last.cond = nullptr;
	}
	AddStmt(state, std::move(branchIf));
	return chainEnd;
}

void Structurer::ParseBackwardConditional(SequenceState& state, ExprPtr cond, uint32_t ip)
{
	const auto target = Target(ip);
	if (target >= state.stmtStart && target <= ip)
	{
		// Jumping back to re-test the condition until it holds ("wait N seconds" when it's a time)
		auto wait = MakeStmt(StmtKind::WaitUntil, state.stmtStart);
		CollectIps(cond, wait->ips);
		wait->expr = std::move(cond);
		wait->ips.push_back(ip);
		AddStmt(state, std::move(wait));
		return;
	}
	if (!_loops.empty() && target == _loops.back().head)
	{
		auto branchIf = MakeStmt(StmtKind::If, state.stmtStart);
		Branch branch;
		branch.cond = Negate(cond, {ip});
		branch.ips = std::move(state.pending);
		state.pending.clear();
		CollectIps(branch.cond, branch.ips);
		branch.body.push_back(MakeStmt(StmtKind::Continue, ip));
		branchIf->branches.push_back(std::move(branch));
		AddStmt(state, std::move(branchIf));
		return;
	}
	// Jumping back to an earlier statement of this sequence while the condition fails: a do-while loop
	for (size_t i = state.out.size(); i-- > 0;)
	{
		if (state.out[i]->startIp == target && state.out[i]->kind != StmtKind::Label)
		{
			auto loop = MakeStmt(StmtKind::DoWhile, target);
			for (size_t j = i; j < state.out.size(); ++j)
			{
				loop->body.push_back(std::move(state.out[j]));
			}
			state.out.resize(i);
			loop->closeIps = std::move(state.pending);
			state.pending.clear();
			loop->expr = Negate(cond, {});
			CollectIps(loop->expr, loop->closeIps);
			loop->closeIps.push_back(ip);
			state.out.push_back(std::move(loop));
			return;
		}
		if (state.out[i]->startIp < target)
		{
			break;
		}
	}
	AddGoto(state, Negate(cond, {ip}), ip, target);
}

void Structurer::ParseJump(SequenceState& state, uint32_t ip, uint32_t hi)
{
	const auto target = Target(ip);
	if (target == ip + 1 || (target == hi && ip + 1 == hi))
	{
		// Jumps to where control goes anyway
		state.pending.push_back(ip);
		return;
	}
	if (!_loops.empty() && target == _loops.back().exit)
	{
		auto stmt = MakeStmt(StmtKind::Break, state.stmtStart);
		stmt->ips.push_back(ip);
		AddStmt(state, std::move(stmt));
		return;
	}
	if (!_loops.empty() && target == _loops.back().head)
	{
		auto stmt = MakeStmt(StmtKind::Continue, state.stmtStart);
		stmt->ips.push_back(ip);
		AddStmt(state, std::move(stmt));
		return;
	}
	AddGoto(state, nullptr, ip, target);
}

std::optional<Structurer::ExceptionLayout> Structurer::MatchException(uint32_t ip, uint32_t hi) const
{
	const auto handler = Target(ip);
	if (handler <= ip + 1 || handler > hi)
	{
		return std::nullopt;
	}
	// Guarded code; end of exceptions; jump over the handlers; handlers; end of handlers
	if (handler >= ip + 3 && At(handler - 2).code == Opcode::EndExcept && At(handler - 2).mode == VMMode::EndExcept &&
	    At(handler - 1).code == Opcode::Jmp)
	{
		const auto after = Target(handler - 1);
		if (after > handler && after <= hi && At(after - 1).code == Opcode::FailExcept)
		{
			return ExceptionLayout {.handler = handler, .after = after, .explicitEnd = true};
		}
		return std::nullopt;
	}
	// A loop that only exceptions can leave: guarded loop; handlers; end of handlers
	if (At(handler - 1).code == Opcode::Jmp && Target(handler - 1) == ip + 1)
	{
		for (uint32_t i = handler; i < hi; ++i)
		{
			if (At(i).code == Opcode::FailExcept)
			{
				return ExceptionLayout {.handler = handler, .after = i + 1, .explicitEnd = false};
			}
			if (At(i).code == Opcode::Except)
			{
				break;
			}
		}
	}
	return std::nullopt;
}

std::optional<uint32_t> Structurer::ParseException(SequenceState& state, uint32_t ip, uint32_t hi)
{
	const auto layout = MatchException(ip, hi);
	if (!layout)
	{
		return std::nullopt;
	}
	auto stmt = MakeStmt(StmtKind::Exception, state.stmtStart);
	stmt->ips.push_back(ip);
	std::vector<uint32_t> trailing;
	if (layout->explicitEnd)
	{
		stmt->body = ParseSequence(ip + 1, layout->handler - 2, trailing);
		stmt->midIps = std::move(trailing);
		stmt->midIps.push_back(layout->handler - 2);
		stmt->midIps.push_back(layout->handler - 1);
	}
	else
	{
		stmt->body = ParseSequence(ip + 1, layout->handler, trailing);
		stmt->midIps = std::move(trailing);
	}
	std::vector<uint32_t> handlerTrailing;
	stmt->handlers = ParseHandlers(layout->handler, layout->after - 1, layout->after, handlerTrailing);
	stmt->closeIps = std::move(handlerTrailing);
	stmt->closeIps.push_back(layout->after - 1);
	if (stmt->body.size() == 1 && stmt->body.front()->kind == StmtKind::While && stmt->body.front()->handlers.empty())
	{
		// The compiler wraps every loop in an exception scope holding the loop's when and until handlers
		auto loop = std::move(stmt->body.front());
		loop->guarded = true;
		loop->startIp = stmt->startIp;
		loop->ips.insert(loop->ips.begin(), stmt->ips.begin(), stmt->ips.end());
		loop->closeIps.insert(loop->closeIps.end(), stmt->midIps.begin(), stmt->midIps.end());
		loop->midIps = std::move(stmt->closeIps);
		loop->handlers = std::move(stmt->handlers);
		AddStmt(state, std::move(loop));
		return layout->after;
	}
	AddStmt(state, std::move(stmt));
	return layout->after;
}

StmtList Structurer::ParseHandlers(uint32_t lo, uint32_t hi, uint32_t after, std::vector<uint32_t>& trailing)
{
	StmtList out;
	uint32_t ip = lo;
	while (ip < hi)
	{
		const auto control = FirstControl(ip, hi);
		if (control >= hi || At(control).code != Opcode::Wait || Target(control) <= control || Target(control) > hi)
		{
			break;
		}
		auto cond = EvalCondition(ip, control);
		if (cond == nullptr)
		{
			break;
		}
		const auto next = Target(control);
		StmtPtr handler;
		std::vector<uint32_t> bodyTrailing;
		if (next >= control + 3 && At(next - 2).code == Opcode::BrkExcept && At(next - 1).code == Opcode::Jmp &&
		    Target(next - 1) == after)
		{
			handler = MakeStmt(StmtKind::Until, ip);
			handler->body = ParseSequence(control + 1, next - 2, bodyTrailing);
			handler->closeIps = std::move(bodyTrailing);
			handler->closeIps.push_back(next - 2);
			handler->closeIps.push_back(next - 1);
		}
		else
		{
			handler = MakeStmt(StmtKind::When, ip);
			const bool hasClose = next - 1 > control && At(next - 1).code == Opcode::Jmp && Target(next - 1) == next;
			handler->body = ParseSequence(control + 1, hasClose ? next - 1 : next, bodyTrailing);
			handler->closeIps = std::move(bodyTrailing);
			if (hasClose)
			{
				handler->closeIps.push_back(next - 1);
			}
		}
		handler->expr = std::move(cond);
		for (uint32_t i = ip; i <= control; ++i)
		{
			handler->ips.push_back(i);
		}
		out.push_back(std::move(handler));
		ip = next;
	}
	if (ip < hi)
	{
		// Handler code that doesn't follow the when/until layout: show it as plain code
		++_fallbackCount;
		Diagnose(DiagnosticSeverity::Warning, ip, "Exception handler code that isn't a when or until clause");
		auto rest = ParseSequence(ip, hi, trailing);
		for (auto& stmt : rest)
		{
			out.push_back(std::move(stmt));
		}
	}
	return out;
}

StmtList Structurer::ParseSequence(uint32_t lo, uint32_t hi, std::vector<uint32_t>& trailing)
{
	SequenceState state;
	state.stmtStart = lo;
	uint32_t ip = lo;
	while (ip < hi)
	{
		const bool wantsLabel = _labels.contains(ip) && !_emittedLabels.contains(ip);
		if (wantsLabel)
		{
			FlushStack(state);
			auto label = MakeStmt(StmtKind::Label, ip);
			label->name = LabelName(ip);
			_emittedLabels.insert(ip);
			// Pending instructions belong before the label
			if (!state.pending.empty())
			{
				trailing.insert(trailing.end(), state.pending.begin(), state.pending.end());
				state.pending.clear();
			}
			state.out.push_back(std::move(label));
		}
		if (state.stack.empty())
		{
			if (state.pending.empty())
			{
				state.stmtStart = ip;
			}
			if (const auto backJump = FindLoopBackJump(ip, hi))
			{
				ip = ParseLoop(state, ip, *backJump);
				continue;
			}
		}

		if (ExecPure(ip, state.stack, false))
		{
			++ip;
			continue;
		}

		const auto& instruction = At(ip);
		bool failed = false;
		switch (instruction.code)
		{
		case Opcode::Pop:
		{
			auto value = PopSlot(state.stack, ip, false, failed);
			if (instruction.mode == VMMode::Reference)
			{
				auto stmt = MakeStmt(StmtKind::Assign, state.stmtStart);
				stmt->name = VariableName(instruction.data.uintVal);
				stmt->op = "=";
				stmt->discardedLoad = state.droppedLoad == stmt->name;
				CollectIps(value, stmt->ips);
				stmt->ips.push_back(ip);
				stmt->expr = std::move(value);
				AddStmt(state, std::move(stmt));
			}
			else if (value->sideEffects)
			{
				auto stmt = MakeStmt(StmtKind::Expression, state.stmtStart);
				CollectIps(value, stmt->ips);
				stmt->ips.push_back(ip);
				stmt->expr = std::move(value);
				AddStmt(state, std::move(stmt));
			}
			else
			{
				// The compiler loads a variable and throws it away before every assignment: nothing to show
				state.droppedLoad = value->kind == ExprKind::Variable ? value->text : std::string();
				CollectIps(value, state.pending);
				state.pending.push_back(ip);
			}
			++ip;
			break;
		}
		case Opcode::Sys:
		{
			uint32_t stackOut = 0;
			auto call = MakeNativeCall(ip, state.stack, false, failed, stackOut);
			auto stmt = MakeStmt(StmtKind::Expression, state.stmtStart);
			CollectIps(call, stmt->ips);
			stmt->expr = std::move(call);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::Run:
		{
			const auto id = instruction.data.uintVal;
			auto stmt = MakeStmt(StmtKind::RunScript, state.stmtStart);
			stmt->async = instruction.mode == VMMode::Async;
			uint32_t paramCount = 0;
			if (id >= 1 && id <= _program.scripts.size())
			{
				stmt->name = _program.scripts[id - 1].name;
				paramCount = _program.scripts[id - 1].parameterCount;
			}
			else
			{
				stmt->name = fmt::format("script_{}", id);
				Diagnose(DiagnosticSeverity::Error, ip, fmt::format("Run of unknown script {}", id));
			}
			stmt->args.resize(paramCount);
			for (auto i = static_cast<int32_t>(paramCount) - 1; i >= 0; --i)
			{
				stmt->args[static_cast<size_t>(i)] = PopSlot(state.stack, ip, false, failed);
			}
			for (const auto& arg : stmt->args)
			{
				CollectIps(arg, stmt->ips);
			}
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::Cast:
		{
			// The zeroing form of the instruction clears a variable
			auto stmt = MakeStmt(StmtKind::Assign, state.stmtStart);
			stmt->name = VariableName(instruction.data.uintVal);
			stmt->op = "zero";
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::Wait:
		{
			auto cond = PopSlot(state.stack, ip, false, failed);
			FlushStack(state);
			if (Target(ip) > ip)
			{
				ip = ParseIf(state, std::move(cond), ip, hi);
			}
			else
			{
				ParseBackwardConditional(state, std::move(cond), ip);
				++ip;
			}
			break;
		}
		case Opcode::Jmp:
			FlushStack(state);
			ParseJump(state, ip, hi);
			++ip;
			break;
		case Opcode::Except:
		{
			FlushStack(state);
			if (const auto next = ParseException(state, ip, hi))
			{
				ip = *next;
				break;
			}
			++_fallbackCount;
			Diagnose(DiagnosticSeverity::Warning, ip, "Exception block with an unexpected layout");
			_wantedLabels.insert(Target(ip));
			auto stmt = MakeStmt(StmtKind::Raw, state.stmtStart);
			stmt->name = fmt::format("install_exception_handler({});", LabelName(Target(ip)));
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::EndExcept:
		{
			FlushStack(state);
			auto stmt = MakeStmt(StmtKind::Raw, state.stmtStart);
			if (instruction.mode == VMMode::Yield)
			{
				stmt->name = "yield();";
			}
			else
			{
				++_fallbackCount;
				stmt->name = "end_exception_handlers();";
			}
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::RetExcept:
		case Opcode::FailExcept:
		case Opcode::BrkExcept:
		{
			FlushStack(state);
			++_fallbackCount;
			auto stmt = MakeStmt(StmtKind::Raw, state.stmtStart);
			stmt->name = instruction.code == Opcode::RetExcept    ? "return_from_exception_handler();"
			             : instruction.code == Opcode::FailExcept ? "next_exception_handler();"
			                                                      : "break_exception_handlers();";
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::End:
		{
			FlushStack(state);
			auto stmt = MakeStmt(StmtKind::Return, state.stmtStart);
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		case Opcode::Line:
			state.pending.push_back(ip);
			++ip;
			break;
		default:
		{
			FlushStack(state);
			++_unsupportedCount;
			++_fallbackCount;
			Diagnose(DiagnosticSeverity::Error, ip,
			         fmt::format("Unsupported instruction {}", static_cast<uint32_t>(instruction.code)));
			auto stmt = MakeStmt(StmtKind::Raw, state.stmtStart);
			stmt->name = fmt::format("unsupported instruction {}", static_cast<uint32_t>(instruction.code));
			stmt->ips.push_back(ip);
			AddStmt(state, std::move(stmt));
			++ip;
			break;
		}
		}
	}
	FlushStack(state);
	trailing.insert(trailing.end(), state.pending.begin(), state.pending.end());
	return std::move(state.out);
}

} // namespace openblack::lhvm::detail
