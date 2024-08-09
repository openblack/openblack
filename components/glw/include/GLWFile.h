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
#include <iosfwd>
#include <string>
#include <vector>

namespace openblack::glw
{

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

	std::filesystem::path _filename;

	std::vector<Glow> _glows;

	/// Error handling
	void Fail(const std::string& msg);

	/// Write file to the input source
	void WriteFile(std::ostream& stream) const;

public:
	GLWFile();
	virtual ~GLWFile();

	/// Read glw file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read glw file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	/// Read file from the input source
	void ReadFile(std::istream& stream);

	/// Write glw file to path on the filesystem
	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }

	[[nodiscard]] const std::vector<Glow>& GetGlows() const { return _glows; }
	[[nodiscard]] const Glow& GetGlow(uint32_t index) const { return _glows[index]; }
	void AddGlow(const Glow& glow) { _glows.push_back(glow); }
};

} // namespace openblack::glw
