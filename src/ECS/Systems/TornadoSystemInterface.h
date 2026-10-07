/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <memory>
#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace openblack::particles
{
struct CarriedObject;
struct TornadoCandidate;
} // namespace openblack::particles

namespace openblack::ecs::systems
{

/// What a tornado does to the things of the world: finds what its foot reaches, picks things up and carries them round
/// its funnel, takes pots from piles, catches creatures, and lets what it carried go where it was flung: the living die
/// where they land, anything else is gone
class TornadoSystemInterface
{
public:
	virtual ~TornadoSystemInterface() = default;

	/// The things a foot reaches, out to the reach past each one's own radius, cell by cell outwards from the foot
	[[nodiscard]] virtual std::vector<particles::TornadoCandidate> Candidates(glm::vec3 foot, float reach) const = 0;
	/// Starts carrying an object with a particle; the object's rotation to start the particle with, none when it can't be
	/// carried
	virtual std::optional<glm::mat3> Carry(const std::shared_ptr<particles::CarriedObject>& carried) = 0;
	/// A creature stops where it is, helpless, and is let off the leash, once
	virtual void CatchCreature(entt::entity creature) = 0;
	/// Takes up to an amount of a pile's food or wood into a new pot of it at the pile, its size times the share; the
	/// pot, none when nothing could be taken. An emptied pile is gone.
	virtual entt::entity TakeFromPile(entt::entity pile, uint32_t amount, float sizeShare) = 0;

	/// Once a game turn, after the particles: what was carried by a particle now gone is let go
	virtual void ProcessTurn() = 0;
	/// Every frame: what is carried is drawn where its particle is, between its last two steps
	virtual void Update(float turnFraction) = 0;
	[[nodiscard]] virtual size_t CarriedCount() const = 0;
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
