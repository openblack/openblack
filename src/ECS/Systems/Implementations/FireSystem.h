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
#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/FireSystemInterface.h"
#include "Fire/FireModel.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct Fire;
}

namespace openblack::ecs::systems
{

class FireSystem final: public FireSystemInterface
{
public:
	FireSystem();
	~FireSystem() override;

	void ProcessTurn() override;
	void MarkBurnPoint() override;
	void Update(float seconds) override;
	void Reset() override;

	void ApplyBurn(entt::entity object, float burn, std::optional<PlayerNames> player) override;
	void SetTemperature(entt::entity object, float temperature, entt::entity source) override;
	void SetOnFire(entt::entity object, float speed) override;
	void PutOut(entt::entity object) override;
	void Forget(entt::entity object) override;
	void HeatHeldObject(entt::entity object) override;
	void StartedMoving(entt::entity object, bool inHand) override;
	void SetCanBeSetOnFire(entt::entity object, bool can) override;
	void SetHurtByFire(entt::entity object, bool hurt) override;

	[[nodiscard]] float GetTemperature(entt::entity object) const override;
	[[nodiscard]] bool IsOnFire(entt::entity object) const override;
	[[nodiscard]] bool IsFireNear(const glm::vec3& point, float radius) const override;
	[[nodiscard]] float GetCharring(entt::entity object) const override;

	[[nodiscard]] std::optional<Blaze> GetBlaze(entt::entity object) const override;
	[[nodiscard]] std::optional<entt::entity> NearestHotMember(entt::entity root, const glm::vec3& point) const override;
	[[nodiscard]] std::optional<Reach> GetReach(entt::entity object) const override;
	[[nodiscard]] std::vector<entt::entity> GetBlazeMembers(entt::entity object) const override;
	[[nodiscard]] std::optional<entt::entity> NearestSafeFire(entt::entity object, const glm::vec3& point) const override;
	[[nodiscard]] bool InSameBlaze(entt::entity a, entt::entity b) const override;
	void MergeBlazes(entt::entity object, entt::entity other) override;
	void MoveFire(entt::entity from, entt::entity to) override;
	void CopyFire(entt::entity from, entt::entity to) override;
	void AddFireman(entt::entity object, entt::entity villager) override;
	void RemoveFireman(entt::entity object, entt::entity villager) override;
	[[nodiscard]] bool IsFiremanOf(entt::entity object, entt::entity villager) const override;

	void CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const override;
	[[nodiscard]] std::optional<uint32_t> GetCharredColour(entt::entity object) const override;
	[[nodiscard]] std::optional<TreeLook> GetBurningTreeLook(entt::entity tree) const override;
	[[nodiscard]] std::optional<uint32_t> GetGlowColour(entt::entity object) const override;
	[[nodiscard]] std::vector<FireInfo> GetFires() const override;

private:
	/// The fire on an object, made when it can burn and has none yet; nullptr when it can't
	components::Fire* FindOrCreate(entt::entity object, std::optional<PlayerNames> player, entt::entity source);
	void Process(entt::entity object);
	void Spread(entt::entity object, float radius);
	void HeatTransfer(entt::entity fire, entt::entity target);
	/// The fire goes out and is taken off the object, which the living think of again as they did before it burned
	void Delete(entt::entity object);
	/// The fire is taken off the object and out of its blaze, with its reaction and sound; whether it had one
	bool Detach(entt::entity object);
	/// Objects that left the world without their fires being forgotten are taken out of the blazes they were in
	void PruneBlazes();
	void JoinBlaze(entt::entity fire, entt::entity other);
	void LeaveBlaze(entt::entity object);
	void HandOverBlaze(entt::entity root, entt::entity newRoot);
	void StopFiremen(entt::entity root, std::optional<entt::entity> onlyFightingThis);
	void ConsiderSound(entt::entity object, float fraction);
	void PlaySounds();
	void StopSound(entt::entity object);

	/// The fires, newest first
	std::vector<entt::entity> _fires;
	/// The turn whose burn point has passed, none before the first
	std::optional<uint32_t> _burnPointTurn;
	/// The slots for the fires that crackle near the camera, and the sounds they make: only the first is ever filled
	struct SoundSlot
	{
		entt::entity fire {entt::null};
		float distance {0.0f};
		entt::entity emitter {entt::null};
	};
	std::array<SoundSlot, 2> _soundSlots {};
	float _farthestSlot {0.0f};
	float _frameTime {0.0f};
};

} // namespace openblack::ecs::systems
