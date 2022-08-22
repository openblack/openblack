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

#include <glm/glm.hpp>

namespace openblack::cam
{

struct CamHeader
{
	uint32_t unk1;           // No perceptible difference when altered. Some values cause the game to crash
	uint32_t movementSpeed;  // The larger this integer, the slower the movement speed
	uint32_t numberOfPoints; // How many path vertices there are in the file
};

struct CamPoint
{
	glm::vec3 position;
	glm::vec3 lookAtPosition;
};

class CamFile
{
public:
	CamFile();
	virtual ~CamFile();
	bool Open(const std::string& file);
	[[nodiscard]] std::vector<CamPoint> GetPoints() { return _points; };
	[[nodiscard]] uint32_t GetMovementSpeed() const { return _header.movementSpeed; };
	[[nodiscard]] uint32_t GetUnknown1() const { return _header.unk1; };

private:
	/// True when a file has been loaded
	bool _isLoaded {false};
	CamHeader _header;
	bool ReadFile(std::istream& stream);
	std::string _filename;
	std::vector<CamPoint> _points;
};

} // namespace openblack::cam
