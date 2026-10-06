/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Simplify.h"

#include <array>
#include <string_view>

namespace openblack::lhvm::detail
{

using namespace openblack::lhvm::chl;

namespace
{

const std::string k_NoName;

[[nodiscard]] bool IsCall(const StmtList& list, size_t i, std::string_view name)
{
	return i < list.size() && CallName(*list[i]) == name;
}

[[nodiscard]] bool IsWaitFor(const StmtList& list, size_t i, std::string_view name)
{
	if (i >= list.size())
	{
		return false;
	}
	const auto& stmt = *list[i];
	return stmt.kind == StmtKind::WaitUntil && stmt.expr != nullptr && stmt.expr->kind == ExprKind::NativeCall &&
	       stmt.expr->text == name && stmt.expr->args.empty();
}

[[nodiscard]] bool CallArgIs(const StmtList& list, size_t i, size_t arg, double value)
{
	const auto& args = list[i]->expr->args;
	return arg < args.size() && IsNumber(args[arg], value);
}

/// The two expressions compute the same value
[[nodiscard]] bool SameValue(const ExprPtr& a, const ExprPtr& b)
{
	if (a == nullptr || b == nullptr)
	{
		return a == b;
	}
	const auto unwrap = [](const ExprPtr& e) {
		return (e->kind == ExprKind::Cast || e->kind == ExprKind::Duplicate) && !e->args.empty() ? e->args[0] : e;
	};
	const auto& x = unwrap(a);
	const auto& y = unwrap(b);
	if (x->kind != y->kind || x->text != y->text || x->op != y->op || x->args.size() != y->args.size())
	{
		return false;
	}
	if (x->kind == ExprKind::Literal && x->number != y->number)
	{
		return false;
	}
	for (size_t i = 0; i < x->args.size(); ++i)
	{
		if (!SameValue(x->args[i], y->args[i]))
		{
			return false;
		}
	}
	return true;
}

void AppendIps(std::vector<uint32_t>& to, const Stmt& from)
{
	to.insert(to.end(), from.ips.begin(), from.ips.end());
	to.insert(to.end(), from.closeIps.begin(), from.closeIps.end());
}

/// Replace `count` statements from `i` with `replacement`, which takes their instructions
void Collapse(StmtList& list, size_t i, size_t count, StmtPtr replacement)
{
	replacement->startIp = list[i]->startIp;
	for (size_t j = i; j < i + count; ++j)
	{
		AppendIps(replacement->ips, *list[j]);
	}
	list.erase(list.begin() + static_cast<std::ptrdiff_t>(i), list.begin() + static_cast<std::ptrdiff_t>(i + count));
	list.insert(list.begin() + static_cast<std::ptrdiff_t>(i), std::move(replacement));
}

[[nodiscard]] StmtPtr MakeMarker(std::string name)
{
	auto marker = MakeStmt(StmtKind::Marker, 0);
	marker->name = std::move(name);
	return marker;
}

/// An until handler begins by giving up any camera and dialogue the script holds; the language does that implicitly
void StripUntilCleanup(Stmt& until)
{
	auto& body = until.body;
	if (IsCall(body, 0, "SET_WIDESCREEN") && CallArgIs(body, 0, 0, 0.0) && IsCall(body, 1, "END_GAME_SPEED") &&
	    IsCall(body, 2, "END_DIALOGUE") && IsCall(body, 3, "END_CAMERA_CONTROL"))
	{
		for (size_t i = 0; i < 4; ++i)
		{
			AppendIps(until.ips, *body[i]);
		}
		body.erase(body.begin(), body.begin() + 4);
	}
}

void RewriteIdioms(StmtList& list)
{
	for (size_t i = 0; i < list.size(); ++i)
	{
		auto& stmt = *list[i];

		// "PROPERTY of Object = value" reads the property and throws it away before setting it
		if (IsCall(list, i, "GET_PROPERTY") && IsCall(list, i + 1, "SET_PROPERTY"))
		{
			const auto& get = list[i]->expr->args;
			const auto& set = list[i + 1]->expr->args;
			if (get.size() == 2 && set.size() == 3 && SameValue(get[0], set[0]) && SameValue(get[1], set[1]))
			{
				auto merged = std::move(list[i + 1]);
				merged->ips.insert(merged->ips.begin(), stmt.ips.begin(), stmt.ips.end());
				merged->discardedLoad = true;
				merged->startIp = stmt.startIp;
				list.erase(list.begin() + static_cast<std::ptrdiff_t>(i), list.begin() + static_cast<std::ptrdiff_t>(i + 2));
				list.insert(list.begin() + static_cast<std::ptrdiff_t>(i), std::move(merged));
				continue;
			}
		}

		// "delete X" deletes the object and clears the variable
		if (IsCall(list, i, "OBJECT_DELETE") && i + 1 < list.size() && list[i + 1]->kind == StmtKind::Assign &&
		    list[i + 1]->op == "zero")
		{
			const auto& target = stmt.expr->args;
			if (!target.empty() && target[0]->kind == ExprKind::Variable && target[0]->text == list[i + 1]->name)
			{
				AppendIps(stmt.ips, *list[i + 1]);
				stmt.discardedLoad = true;
				list.erase(list.begin() + static_cast<std::ptrdiff_t>(i + 1));
				continue;
			}
		}

		// "Object play ANIMATION loop N" sets the animation and then the scripted play state
		if (IsCall(list, i, "SET_SCRIPT_ULONG") && IsCall(list, i + 1, "SET_SCRIPT_STATE") && CallArgIs(list, i + 1, 1, 200.0))
		{
			const auto& ulong = stmt.expr->args;
			const auto& state = list[i + 1]->expr->args;
			// The state is pushed after the animation is set (the "state" statement pushes it before)
			if (ulong.size() == 3 && state.size() == 2 && SameValue(ulong[0], state[0]) && !state[1]->ips.empty() &&
			    state[1]->ips.front() > stmt.expr->ips.front())
			{
				auto play = MakeStmt(StmtKind::Expression, stmt.startIp);
				play->name = "play";
				play->expr = stmt.expr;
				play->args = {state[0], ulong[1], ulong[2]};
				Collapse(list, i, 2, std::move(play));
				continue;
			}
		}

		// "move camera to POSITION time T" and "set camera to POSITION" convert a camera position constant twice
		if ((IsCall(list, i, "MOVE_CAMERA_POSITION") && IsCall(list, i + 1, "MOVE_CAMERA_FOCUS")) ||
		    (IsCall(list, i, "SET_CAMERA_POSITION") && IsCall(list, i + 1, "SET_CAMERA_FOCUS")))
		{
			const auto& position = stmt.expr->args;
			const auto& focus = list[i + 1]->expr->args;
			const bool move = CallName(stmt) == "MOVE_CAMERA_POSITION";
			if (!position.empty() && position.size() == focus.size() && position[0]->kind == ExprKind::NativeCall &&
			    position[0]->text == "CONVERT_CAMERA_POSITION" && focus[0]->kind == ExprKind::NativeCall &&
			    focus[0]->text == "CONVERT_CAMERA_FOCUS" && position[0]->args.size() == 1 && focus[0]->args.size() == 1 &&
			    SameValue(position[0]->args[0], focus[0]->args[0]) && (!move || SameValue(position[1], focus[1])))
			{
				auto camera = MakeStmt(StmtKind::Expression, stmt.startIp);
				camera->name = move ? "move camera to" : "set camera to";
				camera->args = {position[0]->args[0]};
				if (move)
				{
					camera->args.push_back(position[1]);
				}
				Collapse(list, i, 2, std::move(camera));
				continue;
			}
		}

		// "state Object STATE [position P] [float F] [ulong A, L]" pushes the object and state, sets the extras, then the
		// state
		if (IsCall(list, i, "SET_SCRIPT_STATE_POS") || IsCall(list, i, "SET_SCRIPT_FLOAT") ||
		    IsCall(list, i, "SET_SCRIPT_ULONG") || IsCall(list, i, "SET_SCRIPT_STATE"))
		{
			static constexpr std::array<std::string_view, 3> k_Extras = {"SET_SCRIPT_STATE_POS", "SET_SCRIPT_FLOAT",
			                                                             "SET_SCRIPT_ULONG"};
			size_t j = i;
			std::array<const Stmt*, 3> extras {};
			for (size_t e = 0; e < k_Extras.size(); ++e)
			{
				if (IsCall(list, j, k_Extras[e]))
				{
					extras[e] = list[j++].get();
				}
			}
			if (IsCall(list, j, "SET_SCRIPT_STATE") && list[j]->expr->args.size() == 2)
			{
				const auto& state = list[j]->expr->args;
				const auto stateIp = state[1]->ips.empty() ? 0 : state[1]->ips.front();
				bool matches = true;
				for (const auto* extra : extras)
				{
					matches =
					    matches && (extra == nullptr || (SameValue(extra->expr->args[0], state[0]) &&
					                                     !extra->expr->ips.empty() && stateIp < extra->expr->ips.front()));
				}
				if (matches && j > i)
				{
					auto composite = MakeStmt(StmtKind::Expression, stmt.startIp);
					composite->name = "state";
					composite->args = {state[0], state[1], nullptr, nullptr, nullptr, nullptr};
					if (extras[0] != nullptr)
					{
						composite->args[2] = extras[0]->expr->args[1];
					}
					if (extras[1] != nullptr)
					{
						composite->args[3] = extras[1]->expr->args[1];
					}
					if (extras[2] != nullptr)
					{
						composite->args[4] = extras[2]->expr->args[1];
						composite->args[5] = extras[2]->expr->args[2];
					}
					Collapse(list, i, j - i + 1, std::move(composite));
					continue;
				}
			}
		}

		// X = X + 1 written without the throw-away load is X++ (or X += 1)
		if (stmt.kind == StmtKind::Assign && stmt.op == "=" && !stmt.discardedLoad && stmt.expr != nullptr &&
		    stmt.expr->kind == ExprKind::Binary && stmt.expr->args[0]->kind == ExprKind::Variable &&
		    stmt.expr->args[0]->text == stmt.name)
		{
			const auto op = stmt.expr->op;
			const char* compound = op == Op::Add   ? "+="
			                       : op == Op::Sub ? "-="
			                       : op == Op::Mul ? "*="
			                       : op == Op::Div ? "/="
			                                       : nullptr;
			if (compound != nullptr)
			{
				// The statement already lists every instruction of the expression
				const auto expr = stmt.expr;
				if ((op == Op::Add || op == Op::Sub) && expr->args[1]->kind == ExprKind::Literal &&
				    expr->args[1]->type == ValueType::Float && expr->args[1]->number == 1.0)
				{
					stmt.op = op == Op::Add ? "++" : "--";
					stmt.expr = nullptr;
				}
				else
				{
					stmt.op = compound;
					stmt.expr = expr->args[1];
				}
			}
		}
	}
}

void FindMarkers(StmtList& list)
{
	for (size_t i = 0; i < list.size(); ++i)
	{
		if (IsWaitFor(list, i, "START_CAMERA_CONTROL") && IsWaitFor(list, i + 1, "START_DIALOGUE") &&
		    IsCall(list, i + 2, "START_GAME_SPEED"))
		{
			const bool cinema = IsCall(list, i + 3, "SET_WIDESCREEN") && CallArgIs(list, i + 3, 0, 1.0);
			Collapse(list, i, cinema ? 4 : 3, MakeMarker(cinema ? "begin cinema" : "begin camera"));
		}
		else if (IsWaitFor(list, i, "START_DIALOGUE"))
		{
			Collapse(list, i, 1, MakeMarker("begin dialogue"));
		}
		else if (IsCall(list, i, "START_DUAL_CAMERA") && list[i]->expr->args.size() == 2)
		{
			auto marker = MakeMarker("begin dual camera");
			marker->args = list[i]->expr->args;
			Collapse(list, i, 1, std::move(marker));
		}
		else if (IsCall(list, i, "SET_WIDESCREEN") && CallArgIs(list, i, 0, 0.0) && IsCall(list, i + 1, "END_GAME_SPEED") &&
		         IsCall(list, i + 2, "END_CAMERA_CONTROL"))
		{
			const bool dialogue = IsCall(list, i + 3, "END_DIALOGUE");
			Collapse(list, i, dialogue ? 4 : 3, MakeMarker(dialogue ? "end cinema" : "end cinema with dialogue"));
		}
		else if (IsCall(list, i, "END_GAME_SPEED") && IsCall(list, i + 1, "END_CAMERA_CONTROL"))
		{
			const bool dialogue = IsCall(list, i + 2, "END_DIALOGUE");
			Collapse(list, i, dialogue ? 3 : 2, MakeMarker(dialogue ? "end camera" : "end camera with dialogue"));
		}
		else if (IsCall(list, i, "END_DIALOGUE"))
		{
			Collapse(list, i, 1, MakeMarker("end dialogue"));
		}
		else if (IsCall(list, i, "RELEASE_DUAL_CAMERA"))
		{
			Collapse(list, i, 1, MakeMarker("end dual camera"));
		}
	}
}

/// "cinema" for "begin cinema" and "end cinema with dialogue"
[[nodiscard]] std::string BlockName(const std::string& marker, std::string_view prefix)
{
	auto name = marker.substr(prefix.size());
	if (const auto with = name.find(" with "); with != std::string::npos)
	{
		name.resize(with);
	}
	return name;
}

void WrapBlocks(StmtList& list)
{
	StmtList out;
	std::vector<std::pair<std::string, size_t>> open;
	for (auto& stmt : list)
	{
		if (stmt->kind == StmtKind::Marker && stmt->name.starts_with("begin "))
		{
			open.emplace_back(BlockName(stmt->name, "begin "), out.size());
			out.push_back(std::move(stmt));
			continue;
		}
		if (stmt->kind == StmtKind::Marker && stmt->name.starts_with("end ") && !open.empty() &&
		    open.back().first == BlockName(stmt->name, "end "))
		{
			const auto start = open.back().second;
			open.pop_back();
			auto block = std::move(out[start]);
			block->kind = StmtKind::Block;
			block->op = stmt->name.substr(4 + BlockName(stmt->name, "end ").size());
			block->name = BlockName(block->name, "begin ");
			for (size_t j = start + 1; j < out.size(); ++j)
			{
				block->body.push_back(std::move(out[j]));
			}
			out.resize(start);
			AppendIps(block->closeIps, *stmt);
			out.push_back(std::move(block));
			continue;
		}
		out.push_back(std::move(stmt));
	}
	list = std::move(out);
}

} // namespace

const std::string& CallName(const Stmt& stmt)
{
	if (stmt.kind == StmtKind::Expression && stmt.name.empty() && stmt.expr != nullptr &&
	    stmt.expr->kind == ExprKind::NativeCall)
	{
		return stmt.expr->text;
	}
	return k_NoName;
}

void Simplify(StmtList& list)
{
	for (auto& stmt : list)
	{
		if (stmt->kind == StmtKind::Until)
		{
			StripUntilCleanup(*stmt);
		}
		Simplify(stmt->body);
		Simplify(stmt->handlers);
		for (auto& branch : stmt->branches)
		{
			Simplify(branch.body);
		}
	}
	RewriteIdioms(list);
	FindMarkers(list);
	WrapBlocks(list);
}

} // namespace openblack::lhvm::detail
