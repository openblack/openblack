/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <filesystem>
#include <istream>
#include <optional>
#include <span>
#include <vector>

/// The physics materials text file: a version and a row count, then one row of six numbers per kind of material
namespace openblack::physconst
{

/// The six numbers of one row, in the file's order
struct Row
{
	float density;
	/// Contact stiffness per unit of mass
	float springK;
	/// Contact damping per unit of mass
	float dampK;
	float friction;
	/// The share of a body's spin it keeps each second
	float spinKept;
	/// Air drag
	float drag;
};

/// What a file holds: its version, the row count it declares and the rows it could read (at most that many)
struct PhysicsConstantsFile
{
	int32_t version {0};
	int32_t declaredRows {0};
	std::vector<Row> rows;
};

/// Reads the text from a stream. Reading stops at the first row that isn't six numbers; none when the header can't be
/// read.
[[nodiscard]] std::optional<PhysicsConstantsFile> Parse(std::istream& stream, std::size_t maxRows);
/// Reads the text held in a buffer
[[nodiscard]] std::optional<PhysicsConstantsFile> Parse(std::span<const uint8_t> buffer, std::size_t maxRows);
/// Reads a file; none when it can't be opened or read
[[nodiscard]] std::optional<PhysicsConstantsFile> Open(const std::filesystem::path& path, std::size_t maxRows);

} // namespace openblack::physconst
