/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ChlAst.h"

namespace openblack::lhvm::detail
{

/// Rewrites instruction idioms into the Challenge Language constructs they were compiled from: camera and dialogue
/// blocks, the clean-up an until handler starts with, compound assignments, property assignments, delete and play.
void Simplify(chl::StmtList& list);

/// Native function name of an expression statement's call, or empty
[[nodiscard]] const std::string& CallName(const chl::Stmt& stmt);

} // namespace openblack::lhvm::detail
