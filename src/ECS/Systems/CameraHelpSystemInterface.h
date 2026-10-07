/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Camera/CameraHelp.h"

namespace openblack::ecs::systems
{

/// What the world's camera lets the player do, as the land's scripts allow it, kept across camera models and reset as
/// a new land opens
class CameraHelpSystemInterface
{
public:
	virtual ~CameraHelpSystemInterface() = default;

	[[nodiscard]] virtual const camera_help::CameraHelp& Get() const = 0;
	[[nodiscard]] virtual camera_help::CameraHelp& Get() = 0;
};

} // namespace openblack::ecs::systems
