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

#include <filesystem>
#include <vector>

#include <glm/fwd.hpp>

namespace openblack
{
namespace anm
{
class ANMFile;
} // namespace anm

namespace debug::gui
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
	bool LoadFromFilesystem(const std::filesystem::path& path);
	bool LoadFromFile(const std::filesystem::path& path);
	void LoadFromBuffer(const std::vector<uint8_t>& data);

	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] uint32_t GetDuration() const { return _duration; }
	[[nodiscard]] const std::vector<Frame>& GetFrames() const { return _frames; }
	[[nodiscard]] std::vector<glm::mat4> GetBoneMatrices(uint32_t time) const;

private:
	std::string _name;
	uint32_t _unknown_0x20; // TODO(#471): Seems to be a uint16_t padded
	float _unknown_0x24;    // TODO(#471)
	float _unknown_0x28;    // TODO(#471)
	float _unknown_0x2C;    // TODO(#471)
	float _unknown_0x30;    // TODO(#471)
	float _unknown_0x34;    // TODO(#471)
	uint32_t _unknown_0x3C; // TODO(#471): Always 1 in Body Block, a count
	uint32_t _duration;
	uint32_t _unknown_0x44; // TODO(#471): Always 1 in Body Block
	uint32_t _unknown_0x48; // TODO(#471): Always 0 in Body Block
	uint32_t _unknown_0x50; // TODO(#471): Seems to be a uint16_t padded

	std::vector<Frame> _frames;

	friend debug::gui::MeshViewer; // TODO(#471): Remove me once the unknowns are known and replace with getters
};

} // namespace openblack
