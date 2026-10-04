/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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
#include <ostream>
#include <string>
#include <vector>

namespace openblack::cam
{

struct CAMHeader
{
	uint32_t size;
	uint32_t durationMilliseconds;
	uint32_t pointCount;
};
static_assert(sizeof(CAMHeader) == 0xc);

struct CAMPoint
{
	std::array<float, 3> position;
	std::array<float, 3> heading;
};
static_assert(sizeof(CAMPoint) == 0x18);

class CAMFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	CAMHeader _header;
	std::vector<CAMPoint> _points;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Write file to the input source
	virtual void WriteFile(std::ostream& stream) const;

public:
	CAMFile();
	virtual ~CAMFile();

	/// Read cam file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read cam file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	/// Write cam file to path on the filesystem
	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] std::string GetFilename() const noexcept { return _filename.string(); }
	[[nodiscard]] const CAMHeader& GetHeader() const noexcept { return _header; }

	[[nodiscard]] uint32_t GetDuration() const noexcept { return _header.durationMilliseconds; }
	[[nodiscard]] const std::vector<CAMPoint>& GetPoints() const noexcept { return _points; };

	void SetMovementSpeed(uint32_t speed) noexcept { _header.durationMilliseconds = speed; }
	void SetPoints(std::vector<openblack::cam::CAMPoint>&& points) noexcept { _points = std::move(points); }
};

} // namespace openblack::cam
