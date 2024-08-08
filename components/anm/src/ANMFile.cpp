/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 * The layout of a ANM File is as follows:
 *
 * - 84 byte header
 *         animation name, containing 32 chars
 *         1 unknown 32-bit int, likely a short // TODO
 *         5 unknown floats // TODO
 *         frame count
 *         1 unknown 32-bit int, likely a bool // TODO
 *         animation duration
 *         2 unknown 32-bit ints, likely bools // TODO
 *         keyframe start offset - offset of keyframe offset block (see below)
 *         1 unknown 32-bit int, likely a short // TODO
 *
 * ------------------------ start of keyframe offsets block --------------------
 *
 * - 4 bytes * frame count, each record contains:
 *         offset into keyframe pointer block (see below)
 *
 * ------------------------ start of keyframe pointer block --------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone offset block (see below)
 *
 * ------------------------ start of bone offset block -------------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone block (see below)
 *
 * ------------------------ start of bone block --------------------------------
 *
 * - 8 byte header containing:
 *         number of bones in the block
 *         timestamp of animation
 * - Bone matrix list of length of number of bones each record containing:
 *         a 4x3 matrix
 *
 */

#include "ANMFile.h"

#include <cassert>

#include <fstream>
#include <spanstream>
#include <utility>

using namespace openblack::anm;

std::string_view openblack::anm::ResultToStr(ANMResult result)
{
	switch (result)
	{
	case ANMResult::Success:
		return "Success";
	case ANMResult::ErrCantOpen:
		return "Could not open file.";
	case ANMResult::ErrFileTooSmall:
		return "File too small to be a valid ANM file.";
	}
	std::unreachable();
}

ANMFile::ANMFile() noexcept = default;
ANMFile::~ANMFile() noexcept = default;

ANMResult ANMFile::ReadFile(std::istream& stream) noexcept
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(ANMHeader))
	{
		return ANMResult::ErrFileTooSmall;
	}

	// First 84 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(ANMHeader));

	_keyframes.resize(_header.frameCount);
	for (uint32_t i = 0; i < _header.frameCount; ++i)
	{
		stream.seekg(_header.framesBase + i * sizeof(uint32_t));

		// In Keyframe offset block
		uint32_t offset;
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Keyframe pointer
		stream.seekg(offset);
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Bone offset block
		stream.seekg(offset);
		stream.read(reinterpret_cast<char*>(&offset), sizeof(offset));

		// Bone block
		stream.seekg(offset);
		uint32_t boneCount;
		stream.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));

		stream.read(reinterpret_cast<char*>(&_keyframes[i].time), sizeof(_keyframes[i].time));

		_keyframes[i].bones.resize(boneCount);
		stream.read(reinterpret_cast<char*>(_keyframes[i].bones.data()),
		            _keyframes[i].bones.size() * sizeof(_keyframes[i].bones[0]));
	}

	_isLoaded = true;

	return ANMResult::Success;
}

ANMResult ANMFile::WriteFile([[maybe_unused]] std::ostream& stream) const noexcept
{
	assert(!_isLoaded);
	stream.write(reinterpret_cast<const char*>(&_header), sizeof(_header));

	return ANMResult::Success;
}

ANMResult ANMFile::Open(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return ANMResult::ErrCantOpen;
	}

	return ReadFile(stream);
}

ANMResult ANMFile::Open(const std::span<const char>& span) noexcept
{
	assert(!_isLoaded);

	auto stream = std::ispanstream(span);

	return ReadFile(stream);
}

ANMResult ANMFile::Write(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ofstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return ANMResult::ErrCantOpen;
	}

	return WriteFile(stream);
}
