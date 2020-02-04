// Copyright 2018-2020 the openblack authors. See COPYING.md for legal info.

#include "LHSegmentFile.h"

namespace openblack
{
	LHSegmentFile::LHSegmentFile(const std::filesystem::path& path, Mode mode)
		: File(path, mode)
	{
		// Read 8 bytes
		// compare to "LiOnHeAd"
		// close and error
		// verify file
		// - read each block header
		// - generate linked list
	}

	bool LHSegmentFile::Read(std::uint8_t* dest, std::size_t bytesToRead)
	{
		return File::Read(dest, bytesToRead);
	}

	bool LHSegmentFile::Write(std::uint8_t* source, std::size_t bytesToWrite)
	{
		return File::Write(source, bytesToWrite);
	}

	bool LHSegmentFile::Seek(std::size_t position, SeekOrigin origin)
	{
		return File::Seek(position, origin);
	}

	[[nodiscard]] std::size_t LHSegmentFile::GetPosition()
	{
		return File::GetPosition();
	}

}
