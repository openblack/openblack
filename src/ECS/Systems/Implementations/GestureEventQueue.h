/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/GestureEventsInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

/// The gesture events without a recogniser: only what debug tools and the testbed put in reaches the miracles
class GestureEventQueue final: public GestureEventsInterface
{
public:
	[[nodiscard]] std::vector<GestureEvent> TakeEvents() override;
	void Inject(const GestureEvent& event) override;
	void Reset() override;

private:
	std::vector<GestureEvent> _events;
};

} // namespace openblack::ecs::systems
