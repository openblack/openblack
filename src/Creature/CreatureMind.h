/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <MindFile.h>

namespace openblack::creature
{

/// A creature mind file as the resource cache holds it: what was read, or why it couldn't be, in which case the
/// creature starts with a fresh mind of its species
struct CreatureMind
{
	creaturemind::MindResult result {creaturemind::MindResult::ErrCantOpen};
	creaturemind::MindFileData data;

	[[nodiscard]] bool Loaded() const { return result == creaturemind::MindResult::Success; }
};

} // namespace openblack::creature
