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

#include <filesystem>
#include <memory>
#include <vector>

namespace openblack
{

class L3DAnim;

class AnimationPack
{
public:
	AnimationPack();
	virtual ~AnimationPack();

	bool LoadFromFile(const std::filesystem::path& path);

	using AnimationVec = std::vector<std::unique_ptr<L3DAnim>>;
	[[nodiscard]] const AnimationVec& GetAnimations() const { return _animations; }
	[[nodiscard]] const L3DAnim& GetAnimation(uint32_t id) const { return *_animations.at(id); }

private:
	AnimationVec _animations;
};

} // namespace openblack
