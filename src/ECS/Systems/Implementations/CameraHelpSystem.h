/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CameraHelpSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CameraHelpSystem final: public CameraHelpSystemInterface
{
public:
	[[nodiscard]] const camera_help::CameraHelp& Get() const final { return _help; }
	[[nodiscard]] camera_help::CameraHelp& Get() final { return _help; }

private:
	camera_help::CameraHelp _help;
};

} // namespace openblack::ecs::systems
