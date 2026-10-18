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
#include <span>
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

struct Glow
{
	uint32_t size; // Must be 196
	uint32_t unk1; // Always seems to be 0
	float red;     // The emitter size and colour. 0 is invisible. Can be greater than 1
	float green;   // The emitter size and colour. 0 is invisible. Can be greater than 1
	float blue;    // The emitter size and colour. 0 is invisible. Can be greater than 1
	float posX;    // The emitter x coordinate. These are world coordinates
	float posY;    // The emitter y coordinate. These are world coordinates
	float posZ;    // The emitter z coordinate. These are world coordinates
	float unkX;    // Unknown coordinate, sometimes copies glowX
	float unkY;    // Unknown coordinate, sometimes copies glowY
	float unkZ;    // Unknown coordinate, sometimes copies glowZ
	float unkX2;   // Unknown coordinate, sometimes copies glowX
	float unkY2;   // Unknown coordinate, sometimes copies glowY
	float unkZ2;   // Unknown coordinate, sometimes copies glowZ
	float unk14;
	float unk15;
	float unk16;
	float unk17;
	float unk18;
	float unk19;
	float unk20;
	float unk21;
	float unk22;
	float unk23;
	float unk24;
	float unk25;
	float unk26;
	float dirX; // Unknown coordinate
	float dirY; // Unknown coordinate
	float dirZ; // Unknown coordinate
	float unk27;
	float unk28;
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

	/// Read glw file from a span
	GLWResult Open(const std::span<char>& span) noexcept;

	/// Read file from the input source
	GLWResult ReadFile(std::istream& stream) noexcept;

	/// Write glw file to path on the filesystem
	GLWResult Write(const std::filesystem::path& filepath) noexcept;

	[[nodiscard]] const std::vector<Glow>& GetGlows() const noexcept { return _glows; }
	[[nodiscard]] const Glow& GetGlow(uint32_t index) const noexcept { return _glows[index]; }
	void AddGlow(const Glow& glow) noexcept { _glows.push_back(glow); }
};

} // namespace openblack::glw
