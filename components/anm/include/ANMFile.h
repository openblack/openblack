/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace openblack::anm
{

struct ANMHeader
{
	std::array<char, 0x20> name;
	uint32_t unknown_0x20; // Seems to be a uint16_t padded
	float unknown_0x24;
	float unknown_0x28;
	float unknown_0x2C;
	float unknown_0x30;
	float unknown_0x34;
	uint32_t frame_count;
	uint32_t unknown_0x3C; // Always 1 in Body Block, a count
	uint32_t animation_duration;
	uint32_t unknown_0x44; // Always 1 in Body Block
	uint32_t unknown_0x48; // Always 0 in Body Block
	uint32_t frames_base;
	uint32_t unknown_0x50; // Seems to be a uint16_t padded
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

	std::filesystem::path _filename;

	ANMHeader _header;
	std::vector<ANMFrame> _keyframes;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Write file to the input source
	virtual void WriteFile(std::ostream& stream) const;

public:
	ANMFile();
	virtual ~ANMFile();

	/// Read anm file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read anm file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	/// Write anm file to path on the filesystem
	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }
	[[nodiscard]] const ANMHeader& GetHeader() const { return _header; }
	[[nodiscard]] ANMHeader& GetHeader() { return _header; }
	[[nodiscard]] const std::vector<ANMFrame>& GetKeyframes() const { return _keyframes; }
	[[nodiscard]] const ANMFrame& GetKeyframe(uint32_t index) const { return _keyframes[index]; }
};

} // namespace openblack::anm
