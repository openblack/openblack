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

	L3DAnim() noexcept = default;
	virtual ~L3DAnim() noexcept = default;

	void Load(const anm::ANMFile& anm) noexcept;
	bool LoadFromFilesystem(const std::filesystem::path& path) noexcept;
	bool LoadFromFile(const std::filesystem::path& path) noexcept;
	bool LoadFromBuffer(const std::vector<uint8_t>& data) noexcept;

	[[nodiscard]] const std::string& GetName() const noexcept { return _name; }
	[[nodiscard]] uint32_t GetDuration() const noexcept { return _duration; }
	[[nodiscard]] const std::vector<Frame>& GetFrames() const noexcept { return _frames; }
	[[nodiscard]] std::vector<glm::mat4> GetBoneMatrices(uint32_t time) const noexcept;
	/// How long one play of the clip lasts in milliseconds, over which its keyframes are evenly spread
	[[nodiscard]] uint32_t GetPlayTime() const noexcept { return _unknown_0x20; }
	/// Whether the clip plays round and round, rather than holding its last pose
	[[nodiscard]] bool IsLooping() const noexcept { return (_unknown_0x50 & k_Looping) != 0; }
	/// Whether the clip plays by the clock even while its animal moves, rather than by the ground it covers
	[[nodiscard]] bool IsPlayedByTime() const noexcept { return (_unknown_0x50 & k_PlayedByTime) != 0; }
	/// How far one play of a moving clip carries its animal, in the mesh's units: the stride a walk or run covers
	[[nodiscard]] float GetStride() const noexcept { return _unknown_0x28; }
	/// The clip holds its last pose rather than playing round and round
	void StopLooping() noexcept { _unknown_0x50 &= ~k_Looping; }

private:
	static constexpr uint32_t k_Looping = 0x100;
	static constexpr uint32_t k_PlayedByTime = 0x200;

	std::string _name;
	uint32_t _unknown_0x20; // The play time in milliseconds
	float _unknown_0x24;    // TODO(#471)
	float _unknown_0x28;    // The stride of a moving clip
	float _unknown_0x2C;    // TODO(#471)
	float _unknown_0x30;    // TODO(#471)
	float _unknown_0x34;    // TODO(#471)
	uint32_t _unknown_0x3C; // TODO(#471): Always 1 in Body Block, a count
	uint32_t _duration;
	uint32_t _unknown_0x44; // TODO(#471): Always 1 in Body Block
	uint32_t _unknown_0x48; // TODO(#471): Always 0 in Body Block
	uint32_t _unknown_0x50; // Flags: 0x100 looping, 0x200 played by time

	std::vector<Frame> _frames;

	friend debug::gui::MeshViewer; // TODO(#471): Remove me once the unknowns are known and replace with getters
};

} // namespace openblack
