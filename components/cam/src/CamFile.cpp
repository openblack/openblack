/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <CamFile.h>

#include <cassert>
#include <fstream>

using namespace openblack::cam;

bool CamFile::Open(const std::string& file)
{
	assert(!_isLoaded);

	_filename = file;
	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		return false;
	}

	_isLoaded = true;
	return ReadFile(stream);
}

bool CamFile::ReadFile(std::istream& stream)
{
	std::size_t fileSize = 0;

	if (stream.seekg(0, std::ios_base::end))
	{
		fileSize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fileSize < sizeof(CamHeader))
	{
		return false;
	}

	stream.read(reinterpret_cast<char*>(&_header), sizeof(_header));

	if (_header.numberOfPoints == 0 || _header.numberOfPoints * sizeof(CamPoint) > fileSize)
	{
		return false;
	}

	for (size_t i = 0; i < _header.numberOfPoints; i++)
	{
		CamPoint point;
		stream.read(reinterpret_cast<char*>(&point), sizeof(CamPoint));
		_points.emplace_back(point);
	}

	return true;
}
