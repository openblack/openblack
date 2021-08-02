/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>
#include <string>
#include <vector>

namespace openblack::cam
{

struct CamHeader
{
	uint32_t unk1; // No perceptible difference when altered. Some values cause the game to crash
	uint32_t movementSpeed;  // The larger this integer, the slower the movement speed
	uint32_t numberOfPoints; // How many path vertices there are in the file
};

struct CamPoint
{
	float x;
	float y;
	float z;
	float pitch;
	float roll;
	float yaw;
};

class CamFile
{
public:
	CamFile() : _isLoaded(false) {};
	bool Open(const std::string& file);
	std::vector<CamPoint> GetPoints() { return _points; };
private:
	bool ReadFile(std::istream& stream);
	bool _isLoaded;
	std::string _filename;
	CamHeader _header;
	std::vector<CamPoint> _points;
};

} // namespace openblack::l3d
