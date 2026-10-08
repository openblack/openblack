/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Particles/ParticleBlast.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleObjectEffects.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

/// What the fireball's and the blast's rules do to the game's objects, through the fire, explosion and particle systems
class GameObjectEffects final: public particles::ObjectEffectsInterface
{
public:
	std::optional<entt::entity> AttachFireBall(glm::vec3 position, float strength, float radius, bool affectedByRain,
	                                           std::optional<PlayerNames> player) override;
	FireBallState FollowFireBall(entt::entity ball, glm::vec3 position, float radius, float strength,
	                             std::optional<glm::vec3> handTarget) override;

	uint32_t StartSpotVisual(SpotVisualType type, glm::vec3 position, float magnitude, int turns) override;
	void CloseSpotVisual(uint32_t id) override;
	[[nodiscard]] std::vector<particles::WaveTarget> FixedObjectsNear(glm::vec3 centre, float range) const override;
	[[nodiscard]] std::optional<particles::WaveTarget> Available(entt::entity object) const override;
	[[nodiscard]] bool CanBeDestroyedBySpell(entt::entity object) const override;
	[[nodiscard]] bool IsCreature(entt::entity object) const override;
	void ExplodeObject(entt::entity object, glm::vec3 origin, float speed) override;
	void ExplodeMesh(entt::id_type mesh, const glm::mat4& transform, glm::vec3 origin, float speed) override;
	[[nodiscard]] std::vector<particles::ExplodeRecord> TakeExplodeRecords(int queue) override;
	void DestroyByBeam(entt::entity object) override;
	void AddRubbleMark(glm::vec3 centre) override;
	void AddWaterRing(glm::vec3 centre, float size) override;
	void ShakeCamera(glm::vec3 position, float radius, float strength, float seconds) override;
	[[nodiscard]] bool IsDryLand(glm::vec3 point) const override;

	/// A new land: nothing waits to break
	void Reset();

private:
	std::array<std::vector<particles::ExplodeRecord>, 2> _queues;
};

/// The pieces each model breaks into, from its triangles, worked out the first time it breaks
class GameFragmentSource final: public particles::FragmentSourceInterface
{
public:
	[[nodiscard]] std::shared_ptr<const std::vector<particles::blast::FragmentPiece>> Fragments(entt::id_type mesh) override;

private:
	std::unordered_map<entt::id_type, std::shared_ptr<const std::vector<particles::blast::FragmentPiece>>> _pieces;
};

} // namespace openblack::ecs::systems
