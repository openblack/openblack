// Copyright 2018-2020 the openblack authors. See COPYING.md for legal info.

#pragma once

#include "Common/File.h"

#include <string>
#include <unordered_map>

namespace openblack
{
	/**
	 * LHSegmentFile is a File wrapper for Lionhead's segmented files.
	 * These files are split up into different sections that can be seeked by string key.
	 *
	 * These files have the magic header: "LiOnHeAd".
	 */
	class LHSegmentFile : public File
	{
	public:
		LHSegmentFile(const std::filesystem::path& path, Mode mode);

	public:
		// File overrides (everything is local to its section)
		// or maybe we don't even need to do this.
		bool Read(std::uint8_t* dest, std::size_t bytesToRead) override;
		bool Write(std::uint8_t* source, std::size_t bytesToWrite) override;
		bool Seek(std::size_t position, SeekOrigin origin) override;
		[[nodiscard]] std::size_t GetPosition() override;

	public:
		bool SeekSegment(const std::string& name);

		/**
		 * Returns a list of segments in the file.
		 */
		std::vector<std::string> GetSegments() const;

		/**
		 * Opens a segment for reading.
		 *
		 * @param name Name of segment to read.
		 * @return     True if segment found and opened.
		 */
		bool OpenSegment(const std::string& name);

		/**
		 * Returns a list of segments in the file.
		 */
		void CloseSegment();
	private:
		bool verifyFile();

	private:
		std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> _segments;
	};

} // namespace openblack
