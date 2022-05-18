/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Zip.h"

#include <cassert>

#include <limits>
#include <stdexcept>

#include <fmt/format.h>
#include <zlib.h>

constexpr auto MAX_BUFFER_SIZE = std::numeric_limits<decltype(z_stream::avail_out)>::max();

std::string GetZlibError(int statusCode)
{
	switch (statusCode)
	{
	case Z_OK:
		return "no error";
	case Z_ERRNO:
		return "reading/writing data";
	case Z_STREAM_ERROR:
		return "invalid compression level";
	case Z_DATA_ERROR:
		return "invalid or incomplete deflate data";
	case Z_MEM_ERROR:
		return "out of memory";
	case Z_BUF_ERROR:
		return "buffer error";
	case Z_VERSION_ERROR:
		return "zlib version mismatch";
	default:
		return "unknown error";
	}
}

/*
 * More information found here https://www.zlib.net/zlib_how.html
 */
std::vector<uint8_t> openblack::zip::Inflate(const std::vector<uint8_t>& deflatedData, size_t inflatedSize)
{
	auto deflatedSize = deflatedData.size();
	auto inflatedData = std::vector<uint8_t>(inflatedSize);
	int returnStatus;

	if (deflatedSize > MAX_BUFFER_SIZE || inflatedSize > MAX_BUFFER_SIZE)
	{
		throw std::runtime_error("Data is too large to inflate");
	}

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	returnStatus = inflateInit(&strm);

	if (returnStatus != Z_OK)
	{
		throw std::runtime_error(fmt::format("Failed to initialise: {}", GetZlibError(returnStatus)));
	}

	strm.avail_in = static_cast<decltype(z_stream::avail_in)>(deflatedSize);
	strm.avail_out = static_cast<decltype(z_stream::avail_out)>(inflatedSize);
	strm.next_in = const_cast<uint8_t*>(deflatedData.data());
	strm.next_out = inflatedData.data();
	returnStatus = inflate(&strm, Z_NO_FLUSH);

	switch (returnStatus)
	{
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
		inflateEnd(&strm);
		throw std::runtime_error(fmt::format("Failed to inflate: {}", GetZlibError(returnStatus)));
	}

	inflateEnd(&strm);
	return inflatedData;
}
