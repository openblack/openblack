/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <map>
#include <vector>

#include "ECS/Systems/TimeSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class TimeSystem final: public TimeSystemInterface
{
public:
	void Start() override;
	void Update() override;
	[[nodiscard]] std::chrono::milliseconds GetElapsedTime() const override;

private:
	std::chrono::time_point<std::chrono::steady_clock> _start;
	std::chrono::milliseconds _elapsedTime;
};
} // namespace openblack::ecs::systems
