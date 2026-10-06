/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include <iosfwd>
#include <string>
#include <vector>

namespace openblack::glw
{

enum class GLWResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	ErrItemCountMismatch,
};

std::string_view ResultToStr(GLWResult result);

/// A light of a room of the temple, as the game reads them and as they were exported from the lights of the rooms'
/// scenes. The game draws a glow at each and a beam down each spot light that has one.
struct Glow
{
	uint32_t size; // Must be 196, how far on the next light is
	/// 0 for an omni light, 1 for a spot light, which can draw a beam
	uint32_t type;
	float red;   // The emitter size and colour. 0 is invisible. Can be greater than 1
	float green; // The emitter size and colour. 0 is invisible. Can be greater than 1
	float blue;  // The emitter size and colour. 0 is invisible. Can be greater than 1
	float posX;  // The emitter x coordinate. These are world coordinates
	float posY;  // The emitter y coordinate. These are world coordinates
	float posZ;  // The emitter z coordinate. These are world coordinates
	/// The point a spot light aims at, the light's own position for an omni light. The game doesn't read it.
	float targetX;
	float targetY;
	float targetZ;
	/// A spot light's direction to its target, a unit long. The game doesn't read it.
	float targetDirectionX;
	float targetDirectionY;
	float targetDirectionZ;
	/// The light's axes and position: a spot light's beam goes down its y axis, and a glow aligned to a wall lies along
	/// its x and z axes
	float xAxisX;
	float xAxisY;
	float xAxisZ;
	float yAxisX;
	float yAxisY;
	float yAxisZ;
	float zAxisX;
	float zAxisY;
	float zAxisZ;
	float originX;
	float originY;
	float originZ;
	/// How long a spot light's beam is
	float coneLength;
	/// Bits: 1 draws a spot light's beam, 8 aligns the glow to the light's axes
	uint32_t flags;
	/// A spot light's inner and outer cone angles, in degrees, -1 for an omni light. The beam spreads by the inner
	/// one; the outer one, always at least as wide, the game doesn't read.
	float hotspotAngle;
	float falloffAngle;
	/// Where the light starts and stops fading with distance, the start never past the end. The game doesn't read them.
	float attenuationStart;
	float attenuationEnd;
	std::array<char, 64> name;
	float emitterSize; // Usually a number between 1 and 10. Multiplies the size
};

static_assert(sizeof(Glow) == 196);

/**
This class is used to read and write GLW files.
*/
class GLWFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	std::vector<Glow> _glows;

	/// Write file to the input source
	GLWResult WriteFile(std::ostream& stream) const noexcept;

public:
	GLWFile() noexcept;
	virtual ~GLWFile() noexcept;

	/// Read glw file from the filesystem
	GLWResult Open(const std::filesystem::path& filepath) noexcept;

	/// Read glw file from a buffer
	GLWResult Open(const std::vector<uint8_t>& buffer) noexcept;

	/// Read file from the input source
	GLWResult ReadFile(std::istream& stream) noexcept;

	/// Write glw file to path on the filesystem
	GLWResult Write(const std::filesystem::path& filepath) noexcept;

	[[nodiscard]] const std::vector<Glow>& GetGlows() const noexcept { return _glows; }
	[[nodiscard]] const Glow& GetGlow(uint32_t index) const noexcept { return _glows[index]; }
	void AddGlow(const Glow& glow) noexcept { _glows.push_back(glow); }
};

} // namespace openblack::glw
