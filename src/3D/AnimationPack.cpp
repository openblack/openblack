/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
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

bool AnimationPack::LoadFromFile(const fs::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Mesh Pack from file: {}", path.generic_string());
	pack::PackFile pack;

	try
	{
		pack.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open {}: {}", path.generic_string(), err.what());
		return false;
	}

	const auto& animation = pack.GetAnimations();
	_animations.resize(pack.GetAnimations().size());
	for (uint32_t i = 0; i < _animations.size(); i++)
	{
		_animations[i] = std::make_unique<L3DAnim>();
		_animations[i]->LoadFromBuffer(animation[i]);
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "{} animation with {} frames with duration {}", _animations[i]->GetName(),
		                    _animations[i]->GetFrames().size(), _animations[i]->GetDuration());
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "AnimationPack loaded {0} animations", _animations.size());

	return true;
}

} // namespace openblack
