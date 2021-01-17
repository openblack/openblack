/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/L3DMesh.h"
#include "Common/MeshLookup.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "Sound/Sound.h"

#include <memory>

namespace openblack::entities::components
{

struct Hand
{
	enum class RenderType
	{
		Model,
		Symbol
	};

	static constexpr MeshId meshId = 999;
	static const std::vector<audio::SoundId>& GrabLandSoundIds() { return Hand::_grabLandSounds; };

	void Init();

	RenderType renderType;

private:
	static const std::vector<audio::SoundId> _grabLandSounds;
};
} // namespace openblack::entities::components
