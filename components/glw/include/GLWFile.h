/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <filesystem>
#include <vector>

namespace openblack::glw
{

struct GlowEntry
{
	uint32_t size;     // Changing this crashes the game. Value is almost always the size of this struct in bytes
	uint16_t unk1;     // Always seems to be 0
	float red;         // The emitter size and colour. 0 is invisible. Can be greater than 1
	float green;       // The emitter size and colour. 0 is invisible. Can be greater than 1
	float blue;        // The emitter size and colour. 0 is invisible. Can be greater than 1
	float glowX;       // The emitter x coordinate. These are world coordinates
	float glowY;       // The emitter y coordinate. These are world coordinates
	float glowZ;       // The emitter z coordinate. These are world coordinates
	float unkX;        // Unknown coordinate, copies glowX
	float unkY;        // Unknown coordinate, copies glowY
	float unkZ;        // Unknown coordinate, copies glowZ
	float unkX2;       // Unknown coordinate, copies glowX
	float unkY2;       // Unknown coordinate, copies glowY
	float unkZ2;       // Unknown coordinate, copies glowZ
	float unk14;       //
	uint32_t unk15;    //
	uint8_t other[44]; //
	float dirX;        // Affect the direction of the emitter e.g. god-rays coming from the stained-glass windows
	float dirY;        // Affect the direction of the emitter e.g. god-rays coming from the stained-glass windows
	float dirZ;        // Affect the direction of the emitter e.g. god-rays coming from the stained-glass windows
	uint8_t unkA1;
	uint8_t unkA2;
	uint8_t unkA3;
	uint8_t unkA4;
	uint8_t unkA5;
	uint8_t unkA6;
	uint8_t unkA7;
	uint8_t unkA8;
	char name[68];
};

/**
This class is used to read and write GLW files.
*/
class GLWFile
{
protected:
	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	std::vector<GlowEntry> _glows;

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Write file to the input source
	virtual void WriteFile(std::ostream& stream) const;

public:
	GLWFile();
	virtual ~GLWFile();

	/// Read glw file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Write glw file to path on the filesystem
	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] std::string GetFilename() const { return _filename.string(); }

	[[nodiscard]] const std::vector<GlowEntry>& GetGlows() const { return _glows; }
	[[nodiscard]] const GlowEntry& GetGlow(uint32_t index) const { return _glows[index]; }
};

} // namespace openblack::glw
