/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlAst.h"

namespace openblack::lhvm::chl
{

void CollectIps(const ExprPtr& expr, std::vector<uint32_t>& out)
{
	if (expr == nullptr)
	{
		return;
	}
	out.insert(out.end(), expr->ips.begin(), expr->ips.end());
	if (expr->kind == ExprKind::Duplicate)
	{
		// The copied value's instructions belong to its first use
		return;
	}
	for (const auto& arg : expr->args)
	{
		CollectIps(arg, out);
	}
}

ExprPtr MakeLiteral(std::string text, ValueType type, double number, std::vector<uint32_t> ips)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Literal;
	expr->type = type;
	expr->text = std::move(text);
	expr->number = number;
	expr->ips = std::move(ips);
	return expr;
}

ExprPtr MakeUnary(Op op, ExprPtr operand, std::vector<uint32_t> ips)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Unary;
	expr->op = op;
	expr->type = op == Op::Not ? ValueType::Bool : operand->type;
	expr->sideEffects = operand->sideEffects;
	expr->args = {std::move(operand)};
	expr->ips = std::move(ips);
	return expr;
}

ExprPtr MakeBinary(Op op, ExprPtr left, ExprPtr right, ValueType type, std::vector<uint32_t> ips)
{
	auto expr = std::make_shared<Expr>();
	expr->kind = ExprKind::Binary;
	expr->op = op;
	expr->type = type;
	expr->sideEffects = left->sideEffects || right->sideEffects;
	expr->args = {std::move(left), std::move(right)};
	expr->ips = std::move(ips);
	return expr;
}

ExprPtr WithIps(const ExprPtr& expr, const std::vector<uint32_t>& ips)
{
	if (ips.empty())
	{
		return expr;
	}
	auto copy = std::make_shared<Expr>(*expr);
	copy->ips.insert(copy->ips.end(), ips.begin(), ips.end());
	return copy;
}

ExprPtr Negate(const ExprPtr& expr, std::vector<uint32_t> ips)
{
	auto merged = expr->ips;
	merged.insert(merged.end(), ips.begin(), ips.end());
	if (expr->kind == ExprKind::Binary && (expr->op == Op::Eq || expr->op == Op::Ne))
	{
		auto copy = std::make_shared<Expr>(*expr);
		copy->op = expr->op == Op::Eq ? Op::Ne : Op::Eq;
		copy->ips = std::move(merged);
		return copy;
	}
	if (expr->kind == ExprKind::Unary && expr->op == Op::Not && expr->args[0]->type == ValueType::Bool)
	{
		return WithIps(expr->args[0], merged);
	}
	if (expr->kind == ExprKind::Literal && expr->type == ValueType::Bool)
	{
		const bool value = expr->number != 0.0;
		return MakeLiteral(value ? "false" : "true", ValueType::Bool, value ? 0.0 : 1.0, merged);
	}
	return MakeUnary(Op::Not, expr, std::move(ips));
}

bool IsLiteralTrue(const ExprPtr& expr)
{
	return expr != nullptr && expr->kind == ExprKind::Literal &&
	       (expr->type == ValueType::Bool || expr->type == ValueType::Int) && expr->number != 0.0;
}

bool IsNumber(const ExprPtr& expr, double value)
{
	return expr != nullptr && (expr->kind == ExprKind::Literal || expr->kind == ExprKind::Constant) && expr->number == value;
}

} // namespace openblack::lhvm::chl
