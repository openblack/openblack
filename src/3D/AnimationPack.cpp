/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AnimationPack.h"
#include "Common/FileSystem.h"
#include "Game.h"
#include "L3DAnim.h"

#include <PackFile.h>
#include <spdlog/spdlog.h>

namespace openblack
{

AnimationPack::AnimationPack() = default;

AnimationPack::~AnimationPack() = default;

void AnimationPack::LoadFromFile(const std::string& filename)
{
	spdlog::debug("Loading Mesh Pack from file: {}", filename);
	pack::PackFile pack;

	try
	{
		pack.Open(Game::instance()->GetFileSystem().FindPath(filename).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", filename, err.what());
		return;
	}

	const auto& animation = pack.GetAnimations();
	_animations.resize(pack.GetAnimations().size());
	for (uint32_t i = 0; i < _animations.size(); i++)
	{
		_animations[i] = std::make_unique<L3DAnim>();
		_animations[i]->LoadFromBuffer(animation[i]);
		spdlog::debug("{} animation with {} frames with duration {}", _animations[i]->GetName(),
		              _animations[i]->GetFrames().size(), _animations[i]->GetDuration());
	}

	spdlog::debug("AnimationPack loaded {0} animations", _animations.size());
}

} // namespace openblack
