/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
#include <vector>

#include <glm/fwd.hpp>

namespace openblack
{
namespace anm
{
class ANMFile;
} // namespace anm

namespace gui
{
class MeshViewer;
}

class L3DAnim
{
public:
	struct Frame
	{
		uint32_t time;
		std::vector<glm::mat4> bones;
	};

	L3DAnim() = default;
	virtual ~L3DAnim() = default;

	void Load(const anm::ANMFile& anm);
	bool LoadFromFile(const fs::path& path);
	void LoadFromBuffer(const std::vector<uint8_t>& data);

	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] uint32_t GetDuration() const { return _duration; }
	[[nodiscard]] const std::vector<Frame>& GetFrames() const { return _frames; }
	[[nodiscard]] const std::vector<glm::mat4> GetBoneMatrices(uint32_t time) const;

private:
	std::string _name;
	uint32_t _unknown_0x20; // Seems to be a uint16_t padded
	float _unknown_0x24;
	float _unknown_0x28;
	float _unknown_0x2C;
	float _unknown_0x30;
	float _unknown_0x34;
	uint32_t _unknown_0x3C; // Always 1 in Body Block, a count
	uint32_t _duration;
	uint32_t _unknown_0x44; // Always 1 in Body Block
	uint32_t _unknown_0x48; // Always 0 in Body Block
	uint32_t _unknown_0x50; // Seems to be a uint16_t padded

	std::vector<Frame> _frames;

	friend gui::MeshViewer; // TODO: Remove me once the unknowns are known and replace with getters
};

} // namespace openblack
