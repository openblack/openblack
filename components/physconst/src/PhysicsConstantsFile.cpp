/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PhysicsConstantsFile.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

using namespace openblack::physconst;

std::optional<PhysicsConstantsFile> openblack::physconst::Parse(std::istream& stream, std::size_t maxRows)
{
	PhysicsConstantsFile file;
	if (!(stream >> file.version >> file.declaredRows))
	{
		return std::nullopt;
	}
	const auto wanted = file.declaredRows > 0 ? std::min(static_cast<std::size_t>(file.declaredRows), maxRows) : 0;
	for (std::size_t i = 0; i < wanted && stream; ++i)
	{
		// The numbers read before one that can't be are kept; the rest of the row stays zero
		Row row {};
		for (float* value : {&row.density, &row.springK, &row.dampK, &row.friction, &row.spinKept, &row.drag})
		{
			if (!(stream >> *value))
			{
				*value = 0.0f;
				break;
			}
		}
		file.rows.push_back(row);
	}
	return file;
}

std::optional<PhysicsConstantsFile> openblack::physconst::Parse(std::span<const uint8_t> buffer, std::size_t maxRows)
{
	std::istringstream stream(std::string(buffer.begin(), buffer.end()));
	return Parse(stream, maxRows);
}

std::optional<PhysicsConstantsFile> openblack::physconst::Open(const std::filesystem::path& path, std::size_t maxRows)
{
	std::ifstream stream(path);
	if (!stream)
	{
		return std::nullopt;
	}
	return Parse(stream, maxRows);
}
