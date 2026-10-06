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
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <entt/entity/entity.hpp>

#include "Creature/CreatureSpells.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellBehaviours.h"
#include "Magic/SpellGrid.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct Spell;
struct SpellSeed;
} // namespace openblack::ecs::components

namespace openblack::ecs::systems
{

/// The miracles' world in the game: the land, the creatures and villagers they heal and hurt, the piles they put down,
/// the rain's rings. There is no fire yet, so what a fireball or a bolt burns shows flames for a while and the living
/// suffer the heat at once; water puts those flames out.
class GameMagicWorld final: public magic::MagicWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 xz) const override;
	[[nodiscard]] bool InBounds(glm::vec3 point) const override;
	[[nodiscard]] bool IsDryLand(glm::vec3 point) const override;
	[[nodiscard]] bool InInfluence(PlayerNames player, glm::vec3 point) const override;
	[[nodiscard]] std::optional<glm::vec3> PositionOf(entt::entity object) const override;
	bool ApplyEffect(entt::entity object, const magic::EffectValues& values) override;
	entt::entity ApplyEffectAt(glm::vec3 point, const magic::EffectValues& values) override;
	void Heat(glm::vec3 point, float radius, float temperature) override;
	void Water(glm::vec3 drop, float reach, std::optional<float> ringGrowth) override;
	[[nodiscard]] std::vector<entt::entity> HealTargets(glm::vec3 point, float radius, size_t maximum) const override;
	bool AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool sparkles) override;

	/// Once a game turn: the flames burn down
	void ProcessTurn();
	void Reset();
	[[nodiscard]] size_t BurningCount() const { return _burning.size(); }

private:
	/// Flames shown on something set alight, and the turns they have left
	struct Burning
	{
		uint32_t effect;
		int turnsLeft;
	};
	/// Sets a thing alight, or keeps its flames going
	void SetAlight(entt::entity object);
	void PutOut(entt::entity object);

	std::unordered_map<entt::entity, Burning> _burning;
};

class MagicSystem final: public MagicSystemInterface, private magic::SpellServicesInterface
{
public:
	MagicSystem();
	~MagicSystem() override;
	MagicSystem(const MagicSystem&) = delete;
	MagicSystem& operator=(const MagicSystem&) = delete;
	MagicSystem(MagicSystem&&) = delete;
	MagicSystem& operator=(MagicSystem&&) = delete;

	entt::entity CastAtPoint(MagicType type, PlayerNames player, glm::vec3 point, const magic::SpellCastData& cast,
	                         const particles::ProcessInfo& info) override;
	entt::entity CastOnObject(MagicType type, PlayerNames player, entt::entity target, const magic::SpellCastData& cast,
	                          const particles::ProcessInfo& info) override;
	void CloseDown(entt::entity spell) override;
	[[nodiscard]] bool CanCastAt(MagicType type, PlayerNames player, glm::vec3 point) override;

	entt::entity CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians) override;
	void SetDispenserPeriod(entt::entity dispenser, float seconds) override;
	bool Remove(entt::entity entity) override;
	entt::entity CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier) override;
	entt::entity CreateOneOffSeedFor(glm::vec3 position, MagicType type) override;
	entt::entity GiveSeedToHand(PlayerNames player, SpellSeedType seed, int powerUp, float multiplier) override;
	void DiscardHeldSeed() override;

	void UpdateHand(const HandFrame& frame, float seconds) override;
	bool PressAction() override;
	void ReleaseAction() override;
	[[nodiscard]] bool IsHandBusy() const override { return _held.has_value(); }
	[[nodiscard]] std::optional<entt::entity> GetHeldSeed() const override { return _held; }
	[[nodiscard]] std::optional<entt::entity> OrbAlong(glm::vec3 origin, glm::vec3 direction) const override;
	[[nodiscard]] HandResult GetLastHandResult() const override { return _lastHandResult; }

	void ProcessTurn() override;
	void Update(float seconds) override;
	void Reset() override;

	void SetIgnoreInfluence(bool ignore) override { _ignoreInfluence = ignore; }
	[[nodiscard]] bool IsIgnoringInfluence() const override { return _ignoreInfluence; }
	[[nodiscard]] std::vector<SpellInfo> GetSpells() const override;
	[[nodiscard]] std::vector<DispenserInfo> GetDispensers() const override;

private:
	// The services the miracles' rules work with
	[[nodiscard]] const InfoConstants& Info() const override;
	[[nodiscard]] magic::MagicWorldInterface& World() override { return _world; }
	[[nodiscard]] magic::SpellCasterInterface* CasterOf(const components::Spell& spell) override;
	[[nodiscard]] float TribalPower(const components::Spell& spell) const override;
	[[nodiscard]] float SeedPower(const components::Spell& spell) const override;
	void AddEffectTarget(const components::Spell& spell, entt::entity target) override;
	[[nodiscard]] std::optional<entt::entity> ShieldAt(glm::vec3 point, float margin) override;
	void StrikeShield(entt::entity shieldSpell, glm::vec3 point) override;
	[[nodiscard]] components::Spell* FindSpell(entt::entity spell) override;
	[[nodiscard]] float GameRandom(float max) override;

	class SpellLink;
	class HandEffectLink;

	/// An event a miracle's effect sent it; whether it acted
	bool OnSpellEvent(entt::entity spell, const particles::SpellEventInfo& event);
	/// The miracle's upkeep and age for a turn, and what it hands its effect
	void Maintain(entt::entity entity, components::Spell& spell);
	/// The miracle's turn: its caster tops it up, its effect steps, its kind does what more it does; false once it is
	/// over
	bool Process(entt::entity entity, components::Spell& spell);
	void Delete(entt::entity spell);
	void ProcessDispensers();
	/// Whether a magic type may be cast on an object: a creature spell only on a creature
	[[nodiscard]] bool CanCastOn(MagicType type, entt::entity target) const;
	/// A creature spell's miracle hands its spell to the creature, which holds it for the miracle's time
	void ReceiveCreatureSpell(entt::entity creature, entt::entity miracle, components::Spell& spell);
	/// A turn of every creature's spells, and what each does to it
	void ProcessCreatureSpells();
	void ApplyCreatureSpell(entt::entity creature, const creature_spells::TurnEvent& event);
	entt::entity MakeOrb(entt::entity dispenser);

	/// What the hand hands a miracle it casts: where it is, the camera, how it moves
	[[nodiscard]] particles::ProcessInfo HandInfo() const;
	/// The held seed's miracle cast at the hand's point; the miracle, or none
	entt::entity CastHeldSeed();
	void FailCast();
	/// The seed has gone from the hand, kept or not
	void LetGoOfSeed(bool destroy);
	/// A held miracle's button came up: the seed keeps what is left for another go, if enough
	void StopHeldMiracle();
	/// The seed's in-hand effect, started, stepped and moved with the hand
	void StartHandEffect(entt::entity seed);
	void StopHandEffect(components::SpellSeed& seed);
	/// A bubble taken by the hand: its miracle in the hand, the bubble popped
	bool TapOrb(entt::entity orb);
	void DestroyOrb(entt::entity orb);
	/// The bubble's model, loaded once
	[[nodiscard]] entt::id_type BubbleMesh() const;
	/// The tribal power of a player for a magic type
	[[nodiscard]] float PlayerTribalPower(PlayerNames player, MagicType type) const;

	GameMagicWorld _world;
	magic::SpellGrid _grid;
	/// The miracles, newest first, as they are processed
	std::deque<entt::entity> _spells;
	std::unordered_map<entt::entity, std::unique_ptr<SpellLink>> _links;
	std::unique_ptr<HandEffectLink> _handEffectLink;
	std::array<std::unique_ptr<magic::PlayerSpellCaster>, static_cast<size_t>(PlayerNames::_COUNT)> _players;
	magic::ObjectSpellCaster _objectCaster;

	// The hand
	HandFrame _hand;
	/// The hand's speed, smoothed over a few frames, and where it was last frame
	glm::vec3 _handVelocity {0.0f};
	std::optional<glm::vec3> _lastHandPosition;
	/// Where the hand was when the held miracle's in-hand effect last stepped
	glm::vec3 _handAtStep {0.0f};
	std::optional<entt::entity> _held;
	/// A thrown miracle readied, waiting for the button to come up
	bool _readied {false};
	/// A held miracle running while the button is down
	bool _holding {false};
	HandResult _lastHandResult {HandResult::None};
	bool _ignoreInfluence {false};
};

} // namespace openblack::ecs::systems
