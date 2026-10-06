/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <set>

#include "LHVMDecompiler.h"

namespace openblack::lhvm
{

namespace
{

[[nodiscard]] bool IsJump(Opcode code)
{
	return code == Opcode::Jmp || code == Opcode::Wait;
}

/// Instructions after which control doesn't simply continue with the next one
[[nodiscard]] bool EndsBlock(Opcode code)
{
	switch (code)
	{
	case Opcode::Jmp:
	case Opcode::Wait:
	case Opcode::End:
	case Opcode::Except:
	case Opcode::RetExcept:
	case Opcode::FailExcept:
	case Opcode::BrkExcept:
		return true;
	default:
		return false;
	}
}

} // namespace

std::optional<size_t> ControlFlowGraph::BlockAt(uint32_t ip) const
{
	const auto it = std::ranges::upper_bound(blocks, ip, {}, &Block::begin);
	if (it == blocks.begin())
	{
		return std::nullopt;
	}
	const auto index = static_cast<size_t>(std::distance(blocks.begin(), it)) - 1;
	return ip < blocks[index].end ? std::optional<size_t>(index) : std::nullopt;
}

ControlFlowGraph BuildControlFlowGraph(std::span<const VMInstruction> instructions, uint32_t firstIp, uint32_t endIp)
{
	ControlFlowGraph graph;
	endIp = std::min<uint32_t>(endIp, static_cast<uint32_t>(instructions.size()));
	if (firstIp >= endIp)
	{
		return graph;
	}
	const auto inRange = [firstIp, endIp](uint32_t ip) { return ip >= firstIp && ip < endIp; };

	// Leaders: the first instruction, jump and handler targets, and whatever follows a block-ending instruction
	std::set<uint32_t> leaders {firstIp};
	for (uint32_t ip = firstIp; ip < endIp; ++ip)
	{
		const auto& instruction = instructions[ip];
		if ((IsJump(instruction.code) || instruction.code == Opcode::Except) && inRange(instruction.data.uintVal))
		{
			leaders.insert(instruction.data.uintVal);
		}
		if (EndsBlock(instruction.code) && ip + 1 < endIp)
		{
			leaders.insert(ip + 1);
		}
	}
	for (auto it = leaders.begin(); it != leaders.end(); ++it)
	{
		const auto next = std::next(it);
		graph.blocks.push_back({.begin = *it,
		                        .end = next == leaders.end() ? endIp : *next,
		                        .successors = {},
		                        .predecessors = {},
		                        .handler = std::nullopt});
	}

	for (size_t i = 0; i < graph.blocks.size(); ++i)
	{
		auto& block = graph.blocks[i];
		const auto last = block.end - 1;
		const auto& instruction = instructions[last];
		const auto fallThrough = [&] {
			if (i + 1 < graph.blocks.size())
			{
				block.successors.push_back(i + 1);
			}
		};
		const auto target = [&](uint32_t ip) {
			if (const auto index = graph.BlockAt(ip))
			{
				block.successors.push_back(*index);
			}
		};
		switch (instruction.code)
		{
		case Opcode::Jmp:
			target(instruction.data.uintVal);
			break;
		case Opcode::Wait:
			fallThrough();
			target(instruction.data.uintVal);
			break;
		case Opcode::Except:
			fallThrough();
			block.handler = graph.BlockAt(instruction.data.uintVal);
			break;
		case Opcode::End:
		case Opcode::RetExcept:
		case Opcode::FailExcept:
			// Control returns to wherever the task was, which the graph doesn't model
			break;
		default:
			fallThrough();
			break;
		}
	}
	for (size_t i = 0; i < graph.blocks.size(); ++i)
	{
		for (const auto successor : graph.blocks[i].successors)
		{
			graph.blocks[successor].predecessors.push_back(i);
		}
	}

	// Reachable from the start, or from an installed exception handler
	std::vector<size_t> work {0};
	while (!work.empty())
	{
		const auto index = work.back();
		work.pop_back();
		auto& block = graph.blocks[index];
		if (block.reachable)
		{
			continue;
		}
		block.reachable = true;
		work.insert(work.end(), block.successors.begin(), block.successors.end());
		if (block.handler)
		{
			work.push_back(*block.handler);
		}
	}
	return graph;
}

} // namespace openblack::lhvm
