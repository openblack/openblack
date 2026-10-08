/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

namespace openblack::particles::draw
{
struct Frame;
}

namespace openblack::ecs::systems
{

/// What explosions leave after them besides their particles: the heap of rubble on the land with its puff of dust, and the
/// camera shaking as they land near it
class ExplosionSystemInterface
{
public:
	virtual ~ExplosionSystemInterface() = default;

	/// A heap of rubble at a point, turned and sized as given, with its puff of dust: what a blast leaves, and the hole of
	/// roots a tree pulled out of the ground leaves
	virtual void AddRubble(const glm::vec3& centre, float yaw, float scale) = 0;
	/// A puff of smoke of a size and colour (0xRRGGBB) at a point, as a tree taking root again throws up
	virtual void AddSmoke(const glm::vec3& centre, float size, uint32_t colour) = 0;
	/// The camera shakes while within a radius of a point, from a strength down to none over the seconds
	virtual void AddShake(const glm::vec3& position, float radius, float strength, float seconds, bool verticalOnly) = 0;
	/// Whether any shake is going on, near the camera or not
	[[nodiscard]] virtual bool IsShaking() const = 0;
	/// Once a frame of the game's time: the rubble lies and fades, the dust flies, and the camera shakes
	virtual void Update(float milliseconds) = 0;
	/// The dust drawn this frame, added to the particles' frame
	virtual void CollectDrawFrame(particles::draw::Frame& frame) const = 0;
	/// A new land: no rubble, no dust, no shaking
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
