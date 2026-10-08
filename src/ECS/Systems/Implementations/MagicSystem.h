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
#include <span>
#include <unordered_map>
#include <vector>

#include <entt/entity/entity.hpp>

#include "Creature/CreatureSpells.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "Magic/AreaEffect.h"
#include "Magic/FlockMiracle.h"
#include "Magic/HandMotion.h"
#include "Magic/MagicTables.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellBehaviours.h"
#include "Magic/SpellGrid.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::components
{
struct PrayerPower;
struct Spell;
struct SpellCaster;
struct SpellSeed;
} // namespace openblack::ecs::components

namespace openblack::ecs::systems
{

/// The miracles' world in the game: the land, the creatures and villagers they heal and hurt, the fires their heat starts
/// and their water cools, the piles they put down, the rain's rings.
class GameMagicWorld final: public magic::MagicWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 xz) const override;
	[[nodiscard]] bool InBounds(glm::vec3 point) const override;
	[[nodiscard]] bool IsDryLand(glm::vec3 point) const override;
	[[nodiscard]] bool IsLand(glm::vec3 point) const override;
	[[nodiscard]] bool InInfluence(PlayerNames player, glm::vec3 point) const override;
	[[nodiscard]] std::optional<glm::vec3> PositionOf(entt::entity object) const override;
	bool ApplyEffect(entt::entity object, const magic::EffectValues& values, const magic::EffectSource& source) override;
	std::vector<entt::entity> ApplyEffectAt(glm::vec3 point, const magic::EffectValues& values,
	                                        const magic::EffectSource& source) override;
	void Water(const magic::WaterDrop& drop) override;
	[[nodiscard]] std::vector<entt::entity> HealTargets(glm::vec3 point, float radius, size_t maximum) const override;
	void CurePoison(entt::entity object) override;
	void PlayerAffected(entt::entity object, MagicType type, PlayerNames player) override;
	bool AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player) override;
	[[nodiscard]] bool CanBeDestroyedBySpell(entt::entity object) const override;

	/// Once a game turn: the flames burn down, and the objects are filed by their cells afresh when next asked for
	void ProcessTurn();
	void Reset();

private:
	/// The objects an effect may reach in a square of the map's cells, as the cells keep them
	[[nodiscard]] std::vector<magic::EffectReceiver> ReceiversIn(const magic::CellRange& cells);
	/// What an effect did to the objects it reached: their lives, the flames, the alignment of whoever it came from, and
	/// the people's reactions to what it crushed
	void Apply(const magic::EffectValues& values, std::span<const magic::EffectOutcome> outcomes,
	           const magic::EffectSource& source);

	/// A drop of the water by an object: a tree grows, a field is sown and ripens, a fire is watched going out
	void WaterObject(entt::entity object, const magic::WaterDrop& drop);
	/// The normal water by a full grown tree of a forest plants another near it, now and then
	void PlantNear(entt::entity tree, const magic::WaterDrop& drop);

	/// Each water miracle's reaction of the people watching it put out a fire
	std::unordered_map<entt::entity, uint32_t> _puttingOutFire;
};

/// A creature as a caster pays for its miracle with its body: its energy and exhaustion (see
/// Creature/CreatureSpellCasting.h)
class CreatureSpellCaster final: public magic::SpellCasterInterface
{
public:
	/// The creature and the magic type of the miracle it pays for next
	void Bind(entt::entity creature, MagicType type)
	{
		_creature = creature;
		_type = type;
	}
	float MaintainSpell(float amount) override;

private:
	entt::entity _creature {entt::null};
	MagicType _type {MagicType::None};
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
	bool CastByCreature(entt::entity creature, MagicType type, entt::entity target) override;
	void ReleaseCreatureCast(entt::entity creature) override;
	entt::entity CastOnObject(MagicType type, PlayerNames player, entt::entity target, const magic::SpellCastData& cast,
	                          const particles::ProcessInfo& info) override;
	void CloseDown(entt::entity spell) override;
	void ApplyEffectAt(glm::vec3 point, const magic::EffectValues& values, PlayerNames player) override
	{
		_world.ApplyEffectAt(point, values, {.player = player});
	}
	bool ApplyEffectToObject(entt::entity object, const magic::EffectValues& values, PlayerNames player) override
	{
		return _world.ApplyEffect(object, values, {.player = player});
	}
	bool ApplyEffectToObject(entt::entity object, const magic::EffectValues& values, const magic::EffectSource& source) override
	{
		return _world.ApplyEffect(object, values, source);
	}
	[[nodiscard]] bool CanCastAt(MagicType type, PlayerNames player, glm::vec3 point) override;
	bool SpellEvent(entt::entity spell, const particles::SpellEventInfo& event) override;
	void PayForSpell(entt::entity spell, float cost) override;

	entt::entity CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians) override;
	void SetDispenserPeriod(entt::entity dispenser, float seconds) override;
	void ChargeDispenser(entt::entity dispenser) override;
	bool Remove(entt::entity entity) override;
	entt::entity CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier) override;
	entt::entity CreateOneOffSeedFor(glm::vec3 position, MagicType type) override;
	entt::entity GiveSeedToHand(PlayerNames player, SpellSeedType seed, int powerUp, float multiplier) override;
	entt::entity SummonSeed(PlayerNames player, SpellSeedType seed, int powerUp) override;
	void DiscardHeldSeed() override;

	void UpdateHand(const HandFrame& frame, float seconds) override;
	bool TapAction() override;
	bool PressAction() override;
	void ReleaseAction() override;
	[[nodiscard]] bool IsHandBusy() const override { return _held.has_value(); }
	[[nodiscard]] magic::PourPose GetHandPour(float fraction) const override { return magic::PourPoseAt(_pour, fraction); }
	[[nodiscard]] HandCastState GetHandCastState() const override;
	[[nodiscard]] std::optional<entt::entity> GetHeldSeed() const override { return _held; }
	[[nodiscard]] std::optional<entt::entity> OrbAlong(glm::vec3 origin, glm::vec3 direction) const override;
	/// The nearest fireball the ray passes through that the hand may hold now, if any
	[[nodiscard]] std::optional<entt::entity> FireBallAlong(glm::vec3 origin, glm::vec3 direction) const;
	/// A fireball of another player the hand takes hold of, as a tap or a grab starts, is caught into a fireball seed in
	/// the hand; whether it was
	bool CatchFireBall(entt::entity ball);
	/// A ready fire seed in the hand put to a fireball takes it in, the seed a twentieth stronger; whether it did
	bool AbsorbFireBall(entt::entity ball);
	[[nodiscard]] HandResult GetLastHandResult() const override { return _lastHandResult; }
	void PlaceHandEffect(glm::vec3 point, float handScale) override;
	[[nodiscard]] bool IsHandInInfluence() const override;

	bool SendSpellEvent(entt::entity spell, const particles::SpellEventInfo& event) override;
	float ForcePayForSpell(entt::entity spell, float cost) override;
	[[nodiscard]] float SpellStrength(entt::entity spell) override;

	void ProcessTurn() override;
	void Update(float seconds) override;
	void Reset() override;

	void SetIgnoreInfluence(bool ignore) override { _ignoreInfluence = ignore; }
	void SetTribalPower(PlayerNames player, Tribe tribe, float power) override;
	void DriveHand(std::optional<HandFrame> frame) override { _driven = frame; }
	[[nodiscard]] std::optional<HandFrame> GetDrivenHand() const override { return _driven; }
	[[nodiscard]] bool IsIgnoringInfluence() const override { return _ignoreInfluence; }
	void RainOnFire(const glm::vec3& point) override;
	[[nodiscard]] std::vector<SpellInfo> GetSpells() const override;
	[[nodiscard]] std::optional<entt::entity> SpellAt(MagicType type, glm::vec3 point, float radius) const override;
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
	void ReactToSpell(components::Spell& spell, bool onCast) override;
	void StartCastEffect(components::Spell& spell, ParticleType type) override;
	[[nodiscard]] magic::FlockMiracleInterface* Flocks() override { return &_flocks; }
	entt::entity CreateTeleportStone(const components::Spell& spell) override;
	[[nodiscard]] bool CanPlaceTeleportStone(glm::vec3 point) const override;
	void ShieldStruck(const components::Spell& shield, bool destroyed) override;
	[[nodiscard]] bool HasWorldObjects(const components::Spell& spell) const override;
	bool PlantForest(components::Spell& spell) override;
	[[nodiscard]] bool ForestCanGrowAt(glm::vec3 point) const override;
	[[nodiscard]] entt::entity SpellEntity(const components::Spell& spell) const override;
	/// The caster player's creature, if it can see where a miracle is cast, takes it that the player wants the desires
	/// the miracle's table says it answers
	void Empathise(MagicType type, PlayerNames player, glm::vec3 point);
	/// The player's last cast and how many of each magic type they have cast
	void RecordCast(MagicType type, PlayerNames player, glm::vec3 point);

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
	/// A miracle cast by a caster at a point, or on an object
	entt::entity Cast(MagicType type, const components::SpellCaster& caster, glm::vec3 point, const magic::SpellCastData& cast,
	                  const particles::ProcessInfo& info);
	entt::entity CastOn(MagicType type, const components::SpellCaster& caster, entt::entity target,
	                    const magic::SpellCastData& cast, const particles::ProcessInfo& info);
	/// Whether a miracle may be cast at an object by a creature, which no influence holds back
	[[nodiscard]] bool CanCreatureCastAt(MagicType type, entt::entity target);
	/// Where a creature casting a miracle holds its hands, and the way to what it casts at: each turn its miracle follows
	/// them
	void UpdateCreatureCast(components::Spell& spell);
	/// A creature spell's miracle hands its spell to the creature, which holds it for the miracle's time
	void ReceiveCreatureSpell(entt::entity creature, entt::entity miracle, components::Spell& spell);
	/// A turn of every creature's spells, and what each does to it
	void ProcessCreatureSpells();
	void ApplyCreatureSpell(entt::entity creature, const creature_spells::TurnEvent& event);
	/// The hand pours the magic of a creature spell it cast onto the creature for a few seconds, rising and tipping, and
	/// stops when the spell closes down
	void StartHandGrain(const components::Spell& spell, entt::entity creature);
	void StopHandGrain(const components::Spell& spell);
	/// The creature spells' own sounds stop once their creature has gone or the camera is beyond their reach
	void KeepCreatureSpellSounds();
	/// The beams of a creature casting from above stay on its hands until they go
	void KeepCreatureBeams();
	struct CreatureSpellSound
	{
		entt::entity emitter;
		entt::entity creature;
		int32_t sample;
	};
	std::vector<CreatureSpellSound> _creatureSpellSounds;
	entt::entity MakeOrb(entt::entity dispenser);

	/// What the hand hands a miracle it casts: where it is, the camera, how it moves
	[[nodiscard]] particles::ProcessInfo HandInfo() const;
	/// The held seed's magic type and records
	[[nodiscard]] MagicType HeldMagicType() const;
	/// The object under the hand the held seed may be cast on: a creature for a creature spell
	[[nodiscard]] std::optional<entt::entity> HeldSeedTarget() const;
	/// Whether the held seed may be cast at the hand's point: on the map, by its cast rule, in the player's influence
	[[nodiscard]] bool HandPointValid() const;
	/// A miracle held locked in the local hand that the hand moves outside its cast rule is let go, as the button would
	void LetGoIfOutsideCastRule(const components::Spell& spell);
	/// Carries out what the hand's casting state machine says
	void Do(const magic::CastActions& actions);
	/// The held seed's miracle cast at the hand's point, at the circle drawn, or on the object under the hand; or applied
	/// again when it runs. The miracle, or none.
	entt::entity CastHeldSeed(magic::CastTarget target);
	/// What becomes of the seed once its miracle is cast: it stays, goes, or leaves the hand with its miracle
	void AfterCast(entt::entity spell);
	void FailCast();
	/// The seed has gone from the hand, kept or not
	void LetGoOfSeed(bool destroy);
	/// A locked miracle was let go: the seed keeps what is left for another go, if enough
	void UnlockHeldMiracle();
	/// The gesture events since the last frame: the circle remembered, a power-up, the seed dropped
	void TakeGestures();
	void PowerUpHeldSeed(int level);
	/// The hum of an armed seed, following the hand
	void StartHoldLoop();
	void StopHoldLoop();
	/// The player's prayer power, if they have a store
	[[nodiscard]] components::PrayerPower* PrayerOf(PlayerNames player) const;
	/// The seed's in-hand effect, started, stepped and moved with the hand
	void StartHandEffect(entt::entity seed);
	void StopHandEffect(components::SpellSeed& seed);
	/// The held seed as the hand shows it, after it came to the hand, became ready or was powered up: how the hand holds
	/// it, its model once it is ready (for the seeds that show one), and its in-hand effect drawn once it is ready
	void ShowHeldSeed();
	/// Whether a player has an icon for a seed at one of their worship sites, at which a seed from a globe is then made
	[[nodiscard]] bool PlayerHasSpellIcon(PlayerNames player, SpellSeedType seed) const;
	/// What the held seed's in-hand effect is handed: where it sits, with the seed's power as its strength
	[[nodiscard]] particles::ProcessInfo HandEffectInfo(const components::SpellSeed& seed) const;
	/// The tribe whose power raises a player's miracle of a magic type, if any
	[[nodiscard]] std::optional<Tribe> TribalPowerTribe(PlayerNames player, MagicType type) const;
	/// A seed in the hand cast a new miracle: a tribe's power behind it is announced
	void AfterSeedCast(const components::SpellSeed& seed, MagicType type);
	/// A bubble taken by the hand: its miracle in the hand, the bubble popped
	bool TapOrb(entt::entity orb);
	void DestroyOrb(entt::entity orb);
	/// Each tribe's power multiplier for a player
	[[nodiscard]] std::array<float, magic::k_TribeCount> PlayerTribalMultipliers(PlayerNames player) const;
	/// The tribal power of a player for a magic type
	[[nodiscard]] float PlayerTribalPower(PlayerNames player, MagicType type) const;

	GameMagicWorld _world;
	magic::FlockMiracle _flocks;
	magic::SpellGrid _grid;
	/// The miracles, newest first, as they are processed
	std::deque<entt::entity> _spells;
	std::unordered_map<entt::entity, std::unique_ptr<SpellLink>> _links;
	std::unique_ptr<HandEffectLink> _handEffectLink;
	std::array<std::unique_ptr<magic::PlayerSpellCaster>, static_cast<size_t>(PlayerNames::_COUNT)> _players;
	magic::ObjectSpellCaster _objectCaster;
	magic::GlobeSpellCaster _globeCaster;
	CreatureSpellCaster _creatureCaster;
	/// The miracle each creature cast and holds
	std::unordered_map<entt::entity, entt::entity> _creatureCasts;
	/// The two beams from a creature's hands as it casts from above, left then right
	std::unordered_map<entt::entity, std::array<uint32_t, 2>> _creatureBeams;

	// The hand
	HandFrame _hand;
	/// The hand's speed, smoothed, and where it was last frame
	glm::vec3 _handVelocity {0.0f};
	std::optional<glm::vec3> _lastHandPosition;
	/// The spin the hand gives the miracle it holds, measured from when that miracle came to it
	magic::HandSpin _handSpin;
	entt::entity _spinningSeed {entt::null};
	/// Where the held miracle's in-hand effect sits, in the fingers of the hand's pose, and how large the hand is drawn
	std::optional<glm::vec3> _handEffectPoint;
	float _handScale {1.0f};
	/// Where the in-hand effect sat when it last stepped
	glm::vec3 _handAtStep {0.0f};
	std::optional<entt::entity> _held;
	magic::CastInput _input;
	/// The action button is down
	bool _actionDown {false};
	/// The hum of an armed seed
	entt::entity _holdLoop {entt::null};
	/// The pour of food or wood lifting and tipping the hand
	magic::PourState _pour;
	/// Each storm miracle's reaction of the people watching its rain put out a fire
	std::unordered_map<entt::entity, uint32_t> _rainWatchers;
	/// The circle drawn for a storm or shield, and the turn it was drawn
	struct Circle
	{
		glm::vec3 centre;
		float radius;
		uint32_t turn;
	};
	std::optional<Circle> _circle;
	/// Game turns since the land started
	uint32_t _turn {0};
	HandResult _lastHandResult {HandResult::None};
	bool _ignoreInfluence {false};
	/// Each player's tribal power multipliers, all 1 until worship raises them (set for now by the testbed)
	std::array<std::array<float, magic::k_TribeCount>, static_cast<size_t>(PlayerNames::_COUNT)> _tribalPowers {};
	/// The hand as a testbed scenario puts it, in place of the mouse
	std::optional<HandFrame> _driven;
};

} // namespace openblack::ecs::systems
