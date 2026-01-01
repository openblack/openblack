/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>
#include <vector>

namespace openblack::anm
{

enum class ANMResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrFileTooSmall,
};

std::string_view ResultToStr(ANMResult result);

struct ANMHeader
{
	std::array<char, 0x20> name;
	uint32_t unknown0x20; // TODO(#471): Seems to be a uint16_t padded
	float unknown0x24;    // TODO(#471)
	float unknown0x28;    // TODO(#471)
	float unknown0x2C;    // TODO(#471)
	float unknown0x30;    // TODO(#471)
	float unknown0x34;    // TODO(#471)
	uint32_t frameCount;
	uint32_t unknown0x3C; // TODO(#471): Always 1 in Body Block, a count
	uint32_t animationDuration;
	uint32_t unknown0x44; // TODO(#471): Always 1 in Body Block
	uint32_t unknown0x48; // TODO(#471): Always 0 in Body Block
	uint32_t framesBase;
	uint32_t unknown0x50; // TODO(#471): Seems to be a uint16_t padded
};
static_assert(sizeof(ANMHeader) == 0x54);

struct ANMBone
{
	std::array<float, 12> matrix;
};

struct ANMFrame
{
	uint32_t time;
	std::vector<ANMBone> bones;
};

/**
  This class is used to read ANMs.
 */
class ANMFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	ANMHeader _header;
	std::vector<ANMFrame> _keyframes;

	/// Write file to the input source
	ANMResult WriteFile(std::ostream& stream) const noexcept;

public:
	ANMFile() noexcept;
	virtual ~ANMFile() noexcept;

	/// Read file from the input source
	ANMResult ReadFile(std::istream& stream) noexcept;

	/// Read anm file from the filesystem
	ANMResult Open(const std::filesystem::path& filepath) noexcept;

	/// Read anm file from a span
	ANMResult Open(const std::span<const char>& span) noexcept;

	/// Write anm file to path on the filesystem
	ANMResult Write(const std::filesystem::path& filepath) noexcept;

	[[nodiscard]] const ANMHeader& GetHeader() const noexcept { return _header; }
	[[nodiscard]] ANMHeader& GetHeader() noexcept { return _header; }
	[[nodiscard]] const std::vector<ANMFrame>& GetKeyframes() const noexcept { return _keyframes; }
	[[nodiscard]] const ANMFrame& GetKeyframe(uint32_t index) const noexcept { return _keyframes[index]; }
};

} // namespace openblack::anm
