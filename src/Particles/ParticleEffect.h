/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <any>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <ParticleFile.h>
#include <entt/entity/entity.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "ParticleMaths.h"
#include "ParticleSpellLink.h"
#include "StormMaths.h"

namespace openblack
{
class GameRandomInterface;
}

/// The particle effects of the miracles and spot visuals, as their files describe them.
///
/// An effect is made of up to 25 groups of rules. A collection is one live instance of a group: it holds atoms (the
/// particles) and runs its group's rules on them each step. Some groups are made when the effect starts; the others are
/// made under each new atom of a rule that names them, so an atom can carry its own collections (a spark trailing
/// smoke). A group flagged as a hierarchy puts the collections under its atoms in their frame, moving, turning and
/// scaling with them. Nothing moves by itself: every change is a rule's.
namespace openblack::particles
{
class Effect;
struct Atom;
struct Collection;
struct SurfaceInstance;
class Modifier;
class ParticleClassRegistry;
class ObjectEffectsInterface;
class LightSheet;
struct GestureTrail;
namespace blast
{
struct MeshFragment;
struct FragmentPiece;
} // namespace blast

/// The pieces each model breaks into, worked out once for a model and kept
class FragmentSourceInterface
{
public:
	FragmentSourceInterface() = default;
	FragmentSourceInterface(const FragmentSourceInterface&) = delete;
	FragmentSourceInterface& operator=(const FragmentSourceInterface&) = delete;
	FragmentSourceInterface(FragmentSourceInterface&&) = delete;
	FragmentSourceInterface& operator=(FragmentSourceInterface&&) = delete;
	virtual ~FragmentSourceInterface() = default;
	/// None for a model there is none of
	[[nodiscard]] virtual std::shared_ptr<const std::vector<blast::FragmentPiece>> Fragments(entt::id_type mesh) = 0;
};

constexpr size_t k_GroupCount = 25;
/// The sprinkling miracles' source keeps no higher than this above the land, however high the hand is
constexpr float k_MaximumSprinkleHeight = 58.0f;

/// A sound a particle asks for: the file's sound action and how big the sound is (1 large, 2 medium, 3 small)
struct ParticleSound
{
	psys::SoundActionValue action;
	int size {2};
	/// Heard only once it has travelled from where it started to the listener, as thunder is
	bool travelsAtSoundSpeed {false};
	/// Heard from the land under its particle rather than from the particle, as thunder comes from the ground under its
	/// cloud
	bool onLand {false};
	/// The ground under it (1 grass .. 8 loose foliage), for a sound that takes it, and its caster's alignment (1 evil, 2
	/// middling, 3 good); the world fills them in as the sound starts
	int surface {1};
	int alignment {2};

	[[nodiscard]] bool Silent() const { return action.sound.empty() || action.sound == "NO_SOUND"; }
};

/// A sound a particle started. The particle owns it: once the particle has gone, or stopped it, the world lets the
/// sound die away, fading it by the fade step each turn when it has one.
struct ParticleSoundLink
{
	ParticleSound sound;
	/// The particle, while it keeps the sound
	const Atom* atom {nullptr};
	/// Where the particle was when it started the sound
	glm::vec3 position {0.0f};
	/// How much quieter it gets each turn once let go, out of 127
	int fadeStep {0};
};

/// A shield a miracle's effect raised: a sphere that the other miracles' particles and bolts can't pass. The effect
/// that raised it owns it; the world only keeps a weak hold, so the shield goes with the effect.
struct ShieldSphere
{
	glm::vec3 centre {0.0f};
	float radius {0.0f};
	/// The miracle behind it, if any
	entt::entity spell {entt::null};
	/// The effect that raised it, for its sparks
	const Effect* owner {nullptr};
	/// Where it has been struck since its sparks last looked
	std::vector<glm::vec3> impacts;
};

/// Something a lightning bolt may strike
struct StrikeCandidate
{
	entt::entity object {entt::null};
	/// Where it stands on the land, and how tall it is
	glm::vec3 position {0.0f};
	float height {0.0f};
	/// A creature not faded away, which draws every fork of a bolt to itself
	bool drawsBolt {false};
	/// Its model doesn't move, so electric arcs can crawl over it once struck
	bool arcs {false};
};

/// A symbol of belief to rise from something that gained it: where, how much (the belief by ten thousand) and in the
/// believed player's colour, 0xRRGGBB
struct BeliefSprite
{
	glm::vec3 position {0.0f};
	int32_t amount {0};
	uint32_t colour {0xFFFFFF};
};

/// A creature's body as the creature spells' particles see it: where it stands, how big it is, where each of its bones
/// is in the world and which bone mirrors each, and the bones of its right foot and right eye
struct CreatureSpellBody
{
	glm::vec3 origin {0.0f};
	float size {1.0f};
	std::vector<glm::vec3> bones;
	std::vector<uint32_t> mirror;
	std::optional<uint32_t> rightFoot;
	std::optional<uint32_t> rightEye;
	/// How far the invisible spell is over it, 0 to 1
	float invisible {0.0f};

	[[nodiscard]] uint32_t Mirror(uint32_t bone) const { return bone < mirror.size() ? mirror[bone] : bone; }
};

/// A point of an object's surface and the way it faces there
struct SurfacePoint
{
	glm::vec3 position {0.0f};
	glm::vec3 normal {0.0f};
};

/// A lightning bolt cast from a hand, as the others see it to clash with it. The bolt's effect owns it; the world keeps
/// a weak hold.
struct BoltShare
{
	const Effect* owner {nullptr};
	/// Bolts made earlier have smaller numbers
	uint64_t order {0};
	glm::vec3 origin {0.0f};
	glm::vec3 centroid {0.0f};
	float heading {0.0f};
	bool hasTargets {false};
	/// The newer bolt this one meets, and the older one this one meets, if any
	std::weak_ptr<BoltShare> linkedTo;
	std::weak_ptr<BoltShare> linkedFrom;
	/// Where this newer bolt meets its older one
	std::optional<glm::vec3> meeting;
};

/// A game object a particle carries, such as something a tornado picked up: the object follows the particle, and is let
/// go where the particle was once the particle has gone
struct CarriedObject
{
	entt::entity object {entt::null};
	/// The particle, while it carries the object
	const Atom* atom {nullptr};
	/// Where the particle was last drawn, kept once it has gone
	glm::vec3 position {0.0f};
	glm::mat3 rotation {1.0f};
};

/// Something near a tornado's foot that it may act on
struct TornadoCandidate
{
	enum class Kind : uint8_t
	{
		/// Small enough to be picked up whole if it fits the funnel: the living, trees, things lying about, pots
		Liftable,
		/// Too big to be picked up: it is caught helpless instead
		Creature,
		/// A pile of food or wood, some of which is taken up as a pot
		Pile,
	};
	entt::entity object {entt::null};
	Kind kind {Kind::Liftable};
	glm::vec3 position {0.0f};
	/// Its radius across the ground
	float radius {0.0f};
	/// A liftable pile, as a pile the hand dropped: when it doesn't fit the funnel, some of it is taken up as a pot
	bool pile {false};
};

/// What the effects need from the world. The game backs it with the land, the players and the camera; tests use a fake.
class ParticleWorldInterface
{
public:
	ParticleWorldInterface() = default;
	ParticleWorldInterface(const ParticleWorldInterface&) = delete;
	ParticleWorldInterface& operator=(const ParticleWorldInterface&) = delete;
	ParticleWorldInterface(ParticleWorldInterface&&) = delete;
	ParticleWorldInterface& operator=(ParticleWorldInterface&&) = delete;
	virtual ~ParticleWorldInterface() = default;

	/// The height of the land at a point of the ground
	[[nodiscard]] virtual float LandHeight(glm::vec2 xz) const = 0;
	/// A player's colour as 0xRRGGBB
	[[nodiscard]] virtual uint32_t PlayerColour(int player) const = 0;
	/// The camera's right and up, for rules that line sprites up with what the camera sees
	[[nodiscard]] virtual glm::vec3 CameraRight() const = 0;
	[[nodiscard]] virtual glm::vec3 CameraUp() const = 0;
	/// Where the camera is
	[[nodiscard]] virtual glm::vec3 CameraPosition() const { return glm::vec3(0.0f); }
	/// A sample of the game's in-game bank played to the listener rather than at a place, as a fireball rushing past the
	/// camera is heard
	virtual void PlayListenerSound(uint32_t /*inGameSample*/) {}
	/// A particle starts a sound; the world keeps a weak hold on it to play it, keep a loop going and let it die away
	virtual void StartSound(const Effect& /*effect*/, const std::shared_ptr<ParticleSoundLink>& /*sound*/) {}
	/// Whether a point of the land is under water
	[[nodiscard]] virtual bool IsWater(glm::vec3 /*point*/) const { return false; }
	/// Whether the land at a point stands above the sea, by the altitude of its cell
	[[nodiscard]] virtual bool IsDryLand(glm::vec3 /*point*/) const { return true; }
	/// The land's normal at a point of the ground
	[[nodiscard]] virtual glm::vec3 LandNormal(glm::vec2 /*xz*/) const { return {0.0f, 1.0f, 0.0f}; }
	/// The ground's kind under a point for sounds and bounces: 1 grass .. 8 loose foliage, 6 deep and 7 shallow water
	[[nodiscard]] virtual int SurfaceAt(glm::vec3 /*point*/) const { return 1; }
	/// A player's alignment as a sound takes it: 1 evil, 2 middling, 3 good
	[[nodiscard]] virtual int SoundAlignment(int /*player*/) const { return 2; }
	/// Whether it is raining or snowing at a point
	[[nodiscard]] virtual bool IsRainingAt(glm::vec3 /*point*/) const { return false; }
	/// The wind at a point, in metres a second
	[[nodiscard]] virtual glm::vec3 WindAt(glm::vec3 /*point*/) const { return glm::vec3(0.0f); }
	/// The wind at a point blended between the weather's nearest cells, in metres a second
	[[nodiscard]] virtual glm::vec3 SmoothWindAt(glm::vec3 point) const { return WindAt(point); }
	/// A storm's weather is laid over the land, until it is moved or taken away; 0 for none
	[[nodiscard]] virtual uint32_t AddRainStorm(const storm::RainStorm& /*storm*/) { return 0; }
	/// Moves a storm's weather; false once it has ended, which a script can do
	virtual bool MoveRainStorm(uint32_t /*storm*/, glm::vec3 /*centre*/) { return true; }
	/// The storm's weather goes at once
	virtual void RemoveRainStorm(uint32_t /*storm*/) {}
	/// What a lightning bolt may strike round a point on the land, as the game finds it: everything standing in the first
	/// cells of a spiral out from the point's cell, whatever its distance, in each cell what stays put and then what moves
	/// in the map's order, a building only in the cell it stands in, but for the dead and the miracles' seeds
	[[nodiscard]] virtual std::vector<StrikeCandidate> StrikeCandidates(glm::vec3 /*centre*/, size_t /*cells*/) const
	{
		return {};
	}
	/// A bolt struck an object whose model doesn't move: electric arcs crawl over it for a while, unless a hundred objects
	/// already have arcs crawling over them
	virtual void QueueArcs(entt::entity /*object*/) {}
	/// Held by an arc as long as it crawls over its object, so that the world knows how many there are
	[[nodiscard]] virtual std::shared_ptr<const void> HoldArc() const { return nullptr; }
	/// The next symbol of belief waiting to rise, the last queued first
	[[nodiscard]] virtual std::optional<BeliefSprite> TakeBeliefSprite() { return std::nullopt; }
	/// A strike no miracle is behind, as a script calls one down, burns what is within reach of where it lands by the
	/// weather's lightning
	virtual void StrikeWithoutMiracle(glm::vec3 /*point*/) {}
	/// The next object struck for arcs to crawl over, the last queued first
	[[nodiscard]] virtual std::optional<entt::entity> TakeArcs() { return std::nullopt; }
	/// Points of an object's model where it stands now, picked by the random function from a part of it: count of them,
	/// none once it has gone
	[[nodiscard]] virtual std::vector<SurfacePoint> SurfacePoints(entt::entity /*object*/, size_t /*count*/,
	                                                              const std::function<int32_t(int32_t)>& /*random*/) const
	{
		return {};
	}
	/// A bolt from a hand shows itself to the others; the world finds it while its effect lives
	virtual void AddBolt(const std::shared_ptr<BoltShare>& /*bolt*/) {}
	/// The live bolts from hands, and the number the next one takes
	[[nodiscard]] virtual std::vector<std::shared_ptr<BoltShare>> Bolts() const { return {}; }
	[[nodiscard]] virtual uint64_t NextBoltOrder() { return 0; }
	/// Whether the land rises across the straight way between two points
	[[nodiscard]] virtual bool LandBlocks(glm::vec3 /*from*/, glm::vec3 /*to*/) const { return false; }
	/// A shield is raised; the effect keeps the sphere, the world finds it while it lives
	virtual void AddShield(const std::shared_ptr<ShieldSphere>& /*shield*/) {}
	/// The live shield holding a point, the sphere grown by a margin, if any
	[[nodiscard]] virtual std::shared_ptr<ShieldSphere> FindShield(glm::vec3 /*point*/, float /*margin*/) const
	{
		return nullptr;
	}
	/// The live shield an effect raised, if any
	[[nodiscard]] virtual std::shared_ptr<ShieldSphere> ShieldOf(const Effect& /*effect*/) const { return nullptr; }

	/// Where an object a rule acts on is and how big it is
	struct TargetInfo
	{
		/// Where it stands, raised by half its height when asked for its centre
		glm::vec3 position;
		/// Its radius across the ground and its height
		float radius;
		float height;
		/// Its scale
		float scale {1.0f};
	};
	/// An object a miracle gave its effect, none once it has gone
	[[nodiscard]] virtual std::optional<TargetInfo> Target(entt::entity /*target*/, bool /*centre*/) const
	{
		return std::nullopt;
	}
	/// The object is in a hand, out of the miracle's reach
	[[nodiscard]] virtual bool IsTargetHeld(entt::entity /*target*/) const { return false; }
	/// A glow of a colour on the object, black for none, as the heal chakra lights the healed
	virtual void SetTargetGlow(entt::entity /*target*/, glm::u8vec3 /*rgb*/) {}
	/// Whether some effect already acts on the object, so that two heals don't both take one person; an effect claims an
	/// object while it acts on it
	[[nodiscard]] virtual bool IsTargetClaimed(entt::entity /*target*/) const { return false; }
	virtual void ClaimTarget(entt::entity /*target*/, bool /*claimed*/) {}

	/// The game turn, for the rules that act only every few turns
	[[nodiscard]] virtual uint32_t GameTurn() const { return 0; }
	/// The colour of the land at a point of the ground, as its material shows it
	[[nodiscard]] virtual glm::u8vec3 LandColour(glm::vec2 /*xz*/) const { return {255, 255, 255}; }
	/// The things a tornado's foot reaches, out to the reach past each one's own radius, in the order it looks for them:
	/// cell by cell outwards from the foot
	[[nodiscard]] virtual std::vector<TornadoCandidate> TornadoCandidates(glm::vec3 /*foot*/, float /*reach*/) const
	{
		return {};
	}
	/// A creature caught by a tornado stops where it is, helpless, and is let off the leash
	virtual void CatchCreature(entt::entity /*creature*/) {}
	/// Takes up to an amount of a pile's resource into a new pot of it at the pile, its size times the share; the pot,
	/// none when nothing could be taken
	virtual entt::entity TakeFromPile(entt::entity /*pile*/, uint32_t /*amount*/, float /*sizeShare*/) { return entt::null; }
	/// A particle starts carrying an object, which leaves the ground and follows it; the object's rotation for the
	/// particle to start with, none when it can't be carried
	virtual std::optional<glm::mat3> Carry(const std::shared_ptr<CarriedObject>& /*carried*/) { return std::nullopt; }
	/// The extra points of an object's model, placed in the world as the object stands; none for a model without any
	[[nodiscard]] virtual std::vector<glm::vec3> TargetExtraPoints(entt::entity /*target*/) const { return {}; }
	/// Where an object a rule follows stands, none once it is no longer there
	[[nodiscard]] virtual std::optional<glm::vec3> ObjectPosition(entt::entity /*object*/) const { return std::nullopt; }
	/// A point picked on an object's model, placed in the world as the object stands: a submesh, one of its primitives
	/// and one of its triangles, each by the effect's random numbers, then a point evenly within the triangle
	struct SurfacePoint
	{
		enum class Kind : uint8_t
		{
			/// The object is no longer there
			Gone,
			/// It has no model to pick from: nothing is picked
			NoModel,
			Point,
		};
		Kind kind {Kind::Gone};
		glm::vec3 position {0.0f};
	};
	[[nodiscard]] virtual SurfacePoint RandomSurfacePoint(entt::entity /*object*/, Effect& /*effect*/) const { return {}; }
	/// A player's alignment, -1 (evil) to 1 (good)
	[[nodiscard]] virtual float PlayerAlignment(int /*player*/) const { return 0.0f; }
	/// How many points an object's model has for glints to sparkle on, 0 for none: a miracle's seed shown in a globe has
	/// every point of every part of its model
	[[nodiscard]] virtual uint32_t TargetPointCount(entt::entity /*object*/) const { return 0; }
	/// One of them, placed in the world as the object was last drawn; none past the end or once it has gone
	[[nodiscard]] virtual std::optional<glm::vec3> TargetPoint(entt::entity /*object*/, uint32_t /*index*/) const
	{
		return std::nullopt;
	}
	/// The scale the object is drawn at, which glints on it take
	[[nodiscard]] virtual float TargetScale(entt::entity /*object*/) const { return 1.0f; }
	/// A creature's body for its spells' particles, none once it has gone
	[[nodiscard]] virtual std::optional<CreatureSpellBody> CreatureBody(entt::entity /*creature*/) const
	{
		return std::nullopt;
	}
	/// The camera follows a camera path file, its points placed by a matrix: it glides onto the path's start over the
	/// pause and then follows it, its time sped up by a factor, for as long as the animation file it goes with lasts and
	/// the miracle behind it is there
	virtual void FollowCameraPath(std::string_view /*file*/, std::string_view /*animation*/, const glm::mat4& /*placement*/,
	                              float /*pauseSeconds*/, float /*speedUp*/, entt::entity /*spell*/)
	{
	}
	/// The next recognised gesture's trail waiting to be shown, the last given first
	[[nodiscard]] virtual std::shared_ptr<GestureTrail> TakeGestureTrail() { return nullptr; }
	/// A trail's sheet of light stands; the effect keeps it, and while it lives the world moves it on every frame and
	/// draws it
	virtual void AddLightSheet(const std::shared_ptr<LightSheet>& /*sheet*/) {}
	/// The colour added to the light of this computer's hand, 0xRRGGBB, black for none
	virtual void SetHandGlow(uint32_t /*rgb*/) {}
	/// What the destructive miracles' rules do to the world's objects, none where there are none
	[[nodiscard]] virtual ObjectEffectsInterface* ObjectEffects() { return nullptr; }
	/// The pieces models break into
	[[nodiscard]] virtual FragmentSourceInterface* FragmentSource() { return nullptr; }
};

/// What every effect works with: the particle classes, the world, the random numbers and the noise
struct EffectServices
{
	const ParticleClassRegistry& classes;
	ParticleWorldInterface& world;
	GameRandomInterface& random;
	const maths::ValueNoise& noise;
};

/// What a new atom looks like and how it is drawn, from one of the file's particle creator classes
struct Creator
{
	enum class Kind : uint8_t
	{
		/// Not drawn: an atom that only carries collections, or marks a point
		Point,
		/// A sprite from a sprite sheet, facing the screen or lying flat
		Sprite,
		/// A model, turned and scaled with its atom
		Mesh,
		/// One joint of a ribbon drawn through every joint of its collection
		Chain,
		/// A puff of mist, the mist mesh facing the camera
		Mist,
		/// A light stamped on the land's colour under it
		LightMap,
		/// The casting player's symbol between two glows
		Symbol,
		/// A surface turned round the vertical, such as the swirl under a dispenser (see SurfaceCreator)
		Surface,
		/// A piece of a model broken apart: its own triangles, turned and scaled with its atom
		Fragment,
		/// A class the game does not draw yet
		Other,
	};

	Creator() = default;
	Creator(const Creator&) = default;
	Creator(Creator&&) = default;
	Creator& operator=(const Creator&) = default;
	Creator& operator=(Creator&&) = default;
	virtual ~Creator() = default;

	Kind kind {Kind::Point};
	std::string className;
	std::array<uint8_t, 4> rgba {255, 255, 255, 255};
	/// Tinted by the casting player's colour, moved towards white by the blend below 1
	bool usePlayerColour {false};
	float playerColourBlend {1.0f};
	float initialScale {1.0f};
	/// A sprite's size is drawn between 0.3 and 1 of the initial scale
	bool randomiseScale {false};
	bool loopAnim {true};

	// Sprites
	/// The sprite sheet's file name without its folder or extension, as the file spells it (such as "S_SpriteSheet3");
	/// its alpha is the same name with an "a"
	std::string texture;
	int fileOffset {0};
	int spritesPerRow {8};
	int numFrames {1};
	int initFrame {0};
	bool randomiseInitFrame {false};
	/// Half of the atoms play their animation backwards
	bool randomiseFrameDirection {false};
	/// Frames a second
	float frameRate {1.0f};
	bool playAnim {false};
	/// Adds to what is behind rather than blending over it
	bool additive {true};
	bool writeDepth {false};
	/// The drawn alpha is multiplied by this over 255
	int scaleAlpha {255};
	/// The height over the width
	float stretch {1.0f};
	/// Lies flat on the ground rather than facing the screen
	bool horizontal {false};
	/// Drawn with its bottom at the atom rather than its middle
	bool centreAtBase {false};
	/// Drawn unrolled whatever the atom's rotation
	bool ignoreRotation {false};
	/// Where in the sprite the atom is, as fractions of the width and height
	glm::vec2 origin {0.0f};
	/// Tinted by the colour of the land under it, as dust kicked up from the ground is
	bool useLandscapeColour {false};

	/// What a creator class adds to a new atom
	virtual void InitAtom(Effect& /*effect*/, Atom& /*atom*/) const {}
};

/// Where an atom was at the end of a step, for drawing between steps
struct DrawState
{
	glm::vec3 position {0.0f};
	glm::mat3 rotation {1.0f};
	float scale {1.0f};
	float stretch {1.0f};
	float alpha {255.0f};
	float frame {0.0f};
};

/// What a rule keeps for one atom, such as when a condition first held
struct AtomRuleData
{
	bool started {false};
	/// An object the rule follows with the atom
	entt::entity object {entt::null};
	/// Something the rule keeps for as long as the atom lives
	std::shared_ptr<const void> held;
	glm::vec4 a {0.0f};
	glm::vec4 b {0.0f};
};

/// A particle
struct Atom
{
	Atom() = default;
	Atom(const Atom&) = delete;
	Atom(Atom&&) = delete;
	Atom& operator=(const Atom&) = delete;
	Atom& operator=(Atom&&) = delete;
	/// Lets go of its sounds and of what it carries
	~Atom();

	Collection* collection {nullptr};
	const Creator* creator {nullptr};
	/// In its collection's frame: the world, or its parent's in a hierarchy
	glm::vec3 position {0.0f};
	glm::vec3 velocity {0.0f};
	glm::mat3 rotation {1.0f};
	/// The creator's scale, and the scale the rules set on top of it
	float baseScale {1.0f};
	float ruleScale {1.0f};
	float stretch {1.0f};
	std::array<uint8_t, 4> rgba {255, 255, 255, 255};
	/// Light added to its colour, as a flash of lightning lights a cloud
	std::array<uint8_t, 3> specular {0, 0, 0};
	/// When it was made, in seconds of its effect's age
	float birth {0.0f};
	bool visible {true};
	float frame {0.0f};
	float frameRate {0.0f};
	bool playAnim {false};
	/// How strongly gravity pulls it
	float gravity {1.0f};
	/// Values its creator gives it for drawing, such as a mist's shape and where its animation starts
	glm::vec2 creatorValue {0.0f};
	/// A number of its own in 0..255, drawn when it is made
	uint32_t random {0};
	/// Turned away by a shield, or cooled, so that it no longer acts on what it meets
	bool deflected {false};
	/// How much of its effect's draw offset it takes, as the start of a bolt from the hand keeps up with the hand between
	/// steps while its forks stay where they were laid
	float drawWeight {1.0f};
	/// The sounds it keeps going, the newest first
	std::vector<std::shared_ptr<ParticleSoundLink>> sounds;
	/// A game object it carries, if any
	std::shared_ptr<CarriedObject> carried;
	/// The triangles of a piece of a broken model it is drawn as
	std::shared_ptr<const blast::MeshFragment> fragment;
	DrawState previous;
	DrawState current;
	/// It has been through a step's end, so it has somewhere to be drawn
	bool drawn {false};
	/// The steps it has been through, up to two. The game draws an atom from its second step on, or from its first when
	/// the rule that made it asks.
	uint8_t steps {0};
	bool drawOnFirstUpdate {false};
	/// Drawn this far from where it is, the shift running out evenly over two seconds from when it was made, at the
	/// effect's age then: a wisp let out of this computer's hand starts at the hand
	std::optional<glm::vec3> drawOffset;
	float drawOffsetFrom {0.0f};
	/// The surface it is drawn as, its own, when a rule draws it as a surface of revolution
	std::shared_ptr<SurfaceInstance> surface;
	std::vector<std::unique_ptr<Collection>> subCollections;
	std::unordered_map<const Modifier*, AtomRuleData> data;
};

/// One live instance of a group
struct Collection
{
	/// What one of the group's rules keeps for this collection
	struct Slot
	{
		const Modifier* modifier {nullptr};
		/// A rule that has finished with the collection is let go
		bool attached {true};
		maths::EmitterClock emitter;
		/// Further state of the rule: a trail's last point, how much it has emitted
		glm::vec4 state {0.0f};
		glm::vec4 extra {0.0f};
		bool first {true};
		/// Any further state a rule keeps for the collection, such as a lightning bolt's forks
		std::any data;
	};

	int group {0};
	Atom* parent {nullptr};
	float birth {0.0f};
	/// The whole collection's alpha, 0..255
	float alpha {255.0f};
	/// Its atoms are drawn between the last two steps; the lightning's forks are drawn as last stepped
	bool interpolated {true};
	/// In the frame of an ancestor atom whose group is a hierarchy
	bool hierarchy {false};
	/// How many times its ribbon repeats its frame, -1 for as its creator has it
	int textureRepeats {-1};
	/// How fast its ribbon's texture slides along it, in sheet heights a second
	float textureSpeed {0.0f};
	std::vector<std::unique_ptr<Atom>> atoms;
	std::vector<Slot> modifiers;
};

/// A rule of a group: it creates atoms, moves them, changes how they look, or removes them
class Modifier
{
public:
	Modifier() = default;
	Modifier(const Modifier&) = delete;
	Modifier& operator=(const Modifier&) = delete;
	Modifier(Modifier&&) = delete;
	Modifier& operator=(Modifier&&) = delete;
	virtual ~Modifier() = default;

	int group {-1};
	/// Let go when the effect closes down
	bool removeOnCloseDown {false};
	/// The name of the condition object that must hold for it to run, empty for always
	std::string condition;

	/// It makes atoms, so an effect is not over while it is attached
	[[nodiscard]] virtual bool Creates() const { return false; }
	/// A class the game does not run yet: it does nothing, but a miracle's effect lives on until the miracle closes it,
	/// since the class might have made atoms
	[[nodiscard]] virtual bool Unported() const { return false; }
	/// It keeps an effect with no atoms alive until the effect closes down, waiting for something to act on
	[[nodiscard]] virtual bool KeepsAlive() const { return false; }
	/// Runs on the collection; false lets the rule go from it. By default it runs on each atom whose condition holds.
	virtual bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const;
	/// Runs on one atom; false removes the atom
	virtual bool ModifyAtom(Effect& /*effect*/, Atom& /*atom*/, Collection::Slot& /*slot*/) const { return true; }
};

/// A running effect
class Effect
{
public:
	/// synced: its random numbers are drawn from the seed every machine of the game shares, rather than this one's
	Effect(std::shared_ptr<const psys::ParticleFile> file, EffectServices services, glm::vec3 origin, float magnitude,
	       bool synced);
	~Effect();
	Effect(const Effect&) = delete;
	Effect& operator=(const Effect&) = delete;
	Effect(Effect&&) = delete;
	Effect& operator=(Effect&&) = delete;

	/// One step of dt seconds, or two the first time when told to
	void Step(float dt);
	/// Its first Step steps twice, as every effect the game runs does: a new effect has run two steps by the end of the
	/// turn it started in
	void StepTwiceFirstTime() { _firstStepTwice = true; }
	/// Closing down sets the close-down conditions, lets the rules marked for it go, and may end the effect at once
	void CloseDown();
	/// Nothing is left of it: no atoms and no rule that could make more, or it has outlived its file's age
	[[nodiscard]] bool Finished() const;
	[[nodiscard]] bool Closing() const { return _closing; }
	/// Closing down ends it at once
	[[nodiscard]] bool DeleteOnCloseDown() const { return _deleteOnCloseDown; }

	void SetOrigin(glm::vec3 origin) { _origin = origin; }
	void SetMagnitude(float magnitude) { _magnitude = magnitude; }
	[[nodiscard]] glm::vec3 GetOrigin() const { return _origin; }
	[[nodiscard]] float GetAge() const { return _age; }
	[[nodiscard]] float GetCloseAge() const { return _closeAge; }
	[[nodiscard]] float GetMagnitude() const { return _magnitude; }
	[[nodiscard]] float GetDt() const { return _dt; }
	[[nodiscard]] const psys::ParticleFile& GetFile() const { return *_file; }
	[[nodiscard]] EffectServices& Services() { return _services; }
	[[nodiscard]] const EffectServices& Services() const { return _services; }
	/// The classes its file names that the game does not run yet
	[[nodiscard]] const std::vector<std::string>& UnportedClasses() const { return _unported; }

	/// The miracle behind it; it is told the effect has started
	void SetSink(SpellSink* sink);
	[[nodiscard]] SpellSink* GetSink() const { return _sink; }
	void SetProcessInfo(const ProcessInfo& info) { _info = info; }
	[[nodiscard]] const ProcessInfo& GetProcessInfo() const { return _info; }
	/// Whether the miracle acted on it; false without a miracle
	bool SendSpellEvent(const SpellEventInfo& event) const;
	[[nodiscard]] int PowerUpLevel() const { return _sink != nullptr ? _sink->PowerUpLevel() : -1; }
	/// The objects the miracle wants its effect to act on, such as the people to heal; the rules take them, the last
	/// given first
	void AddTarget(entt::entity target) { _targets.push_back(target); }
	[[nodiscard]] std::optional<entt::entity> TakeTarget();
	[[nodiscard]] size_t TargetCount() const { return _targets.size(); }
	/// Points of the world the effect is to act on, such as where a beam ends; the rules take them, the last given first
	void AddTargetPosition(glm::vec3 position) { _targetPositions.push_back(position); }
	[[nodiscard]] std::optional<glm::vec3> TakeTargetPosition();
	[[nodiscard]] size_t TargetPositionCount() const { return _targetPositions.size(); }
	/// An effect without a miracle counts as this computer's
	[[nodiscard]] bool IsMyInterfaceCasting() const { return _sink == nullptr || _sink->IsMyInterfaceCasting(); }
	[[nodiscard]] bool IsHumanPlayerCasting() const { return _sink != nullptr && _sink->IsHumanPlayerCasting(); }
	/// An effect without a miracle isn't a script's
	[[nodiscard]] bool IsScriptCasting() const { return _sink != nullptr && _sink->IsScriptCasting(); }
	void SetDirection(glm::vec3 direction) { _direction = direction; }
	[[nodiscard]] glm::vec3 GetDirection() const { return _direction; }
	/// The casting player, for the creators that take its colour; -1 for none
	void SetPlayer(int player) { _player = player; }
	[[nodiscard]] int GetPlayer() const { return _player; }
	/// The whole effect's drawn alpha, 0..255
	void SetGlobalAlpha(float alpha) { _globalAlpha = alpha; }
	[[nodiscard]] float GetGlobalAlpha() const { return _globalAlpha; }
	/// A hidden effect steps as usual but draws nothing, as the miracle in the hand does until its seed is ready
	void SetHidden(bool hidden) { _hidden = hidden; }
	[[nodiscard]] bool IsHidden() const { return _hidden; }

	// For the rules
	/// Random numbers on the effect's stream; 0 outside a step, as the game draws them
	[[nodiscard]] float Random(float max);
	[[nodiscard]] float Random(float min, float max);
	[[nodiscard]] int32_t Rand(int32_t n);
	/// A point in the unit ball
	[[nodiscard]] glm::vec3 RandomInBall();
	/// The value of a float provider object this step, or the fallback for none
	[[nodiscard]] float FloatProvider(const std::string& name, float fallback) const;
	[[nodiscard]] bool ConditionForCollection(const std::string& name, const Collection& collection) const;
	[[nodiscard]] bool ConditionForAtom(const std::string& name, const Atom& atom) const;
	[[nodiscard]] const Creator* FindCreator(const std::string& name) const;
	/// A new atom in the collection where its parent is, coloured and scaled by its creator, with its next groups'
	/// collections made under it
	Atom& NewAtom(Collection& collection, const Creator* creator, std::span<const int> nextGroups);
	/// Moves an atom out of its collection into the first collection of a group made at the start, which is made when
	/// there is none, where it carries on from the point of the world it was at. Not while its collection's atoms are
	/// being walked.
	void MoveToGroup(Atom& atom, int group);
	/// The groups' collections made under an atom that already exists
	void AddSubCollections(Atom& atom, std::span<const int> groups);
	/// A new atom in the first collection of a group made at the start, which is made when there is none; nullptr for no
	/// such group
	Atom* NewAtomInGroup(int group, const Creator* creator);
	/// Where a collection's new atoms start: the effect's origin for a group made at the start; the parent atom's
	/// point, or nothing at all in a hierarchy where the parent's frame already puts them there
	[[nodiscard]] glm::vec3 SpawnPosition(const Collection& collection) const;
	[[nodiscard]] glm::vec3 GlobalPosition(const Atom& atom) const;
	/// A point of a collection's frame in the world and back. In a hierarchy the frame is that of every ancestor atom
	/// whose group is a hierarchy, each with its position, rotation and scale; elsewhere the world.
	[[nodiscard]] glm::vec3 LocalToGlobal(const Collection& collection, const glm::vec3& local) const;
	[[nodiscard]] glm::vec3 GlobalToLocal(const Collection& collection, const glm::vec3& global) const;
	/// The scale of an atom's frame: its two scales, the vertical also stretched
	[[nodiscard]] static glm::vec3 FrameScale(const Atom& atom);
	[[nodiscard]] float AtomAge(const Atom& atom) const { return _age - atom.birth; }
	[[nodiscard]] float CollectionAge(const Collection& collection) const { return _age - collection.birth; }
	[[nodiscard]] bool IsHierarchy(int group) const;

	/// An atom drawn between the last two steps
	struct DrawAtom
	{
		const Creator* creator;
		glm::vec3 position;
		glm::mat3 rotation;
		float scale;
		float stretch;
		float alpha;
		float frame;
		std::array<uint8_t, 3> rgb;
		/// Seconds it has lived, at the drawn time
		float age {0.0f};
		/// Its creator's values (Atom::creatorValue)
		glm::vec2 creatorValue {0.0f};
		/// How far it is drawn from the last step to the current one, 0..1
		float fraction {1.0f};
		/// The surface it is drawn as (Atom::surface)
		const SurfaceInstance* surface {nullptr};
		/// How much of its effect's draw offset it takes (Atom::drawWeight)
		float offsetWeight {1.0f};
		/// The triangles of a piece of a broken model
		std::shared_ptr<const blast::MeshFragment> fragment;
	};
	/// The ribbon through the joints of one collection
	struct DrawChain
	{
		const Creator* creator;
		/// Its joints in DrawWalk::joints, the first made first
		uint32_t firstJoint;
		uint32_t jointCount;
		/// How many times it repeats its frame, -1 for as its creator has it
		int textureRepeats {-1};
		/// How far its texture has slid along it, in sheet heights
		float textureScroll {0.0f};
	};
	/// Everything an effect draws, in the order the game draws it all at once: each collection's atoms, the newest first,
	/// then its ribbon, then the collections under each of its atoms in turn, walked the same way. The newest collections
	/// come first too.
	struct DrawWalk
	{
		/// A drawn atom, or with `chain` set a ribbon, by its index in atoms or chains
		struct Step
		{
			bool chain;
			uint32_t index;
		};
		std::vector<DrawAtom> atoms;
		std::vector<DrawAtom> joints;
		std::vector<DrawChain> chains;
		std::vector<Step> steps;

		void Clear()
		{
			atoms.clear();
			joints.clear();
			chains.clear();
			steps.clear();
		}
	};
	/// Every visible atom of a kind, t of the way from the last step to the current one, in the walk's order. Atoms faded
	/// below one step of alpha are left out.
	void Collect(float t, std::vector<DrawAtom>& out, Creator::Kind kind = Creator::Kind::Sprite) const;
	/// Everything drawn, t of the way from the last step to the current one, appended to the walk. A ribbon needs two
	/// joints; its joints are drawn whatever their alpha.
	void Walk(float t, DrawWalk& out) const;
	[[nodiscard]] size_t AtomCount() const { return _atomCount; }
	[[nodiscard]] size_t CollectionCount() const;

private:
	void StepOnce(float dt);
	void CreateCollection(int group, Atom* parent, std::vector<std::unique_ptr<Collection>>& into);
	void UpdateCollection(Collection& collection);
	void PostUpdate(Collection& collection, const glm::vec3& parentPosition, const glm::mat3& parentRotation,
	                const glm::vec3& parentScale);
	void WalkCollection(const Collection& collection, float t, DrawWalk& out) const;
	[[nodiscard]] std::optional<DrawAtom> Interpolate(const Atom& atom, float t, bool interpolated) const;
	[[nodiscard]] bool AnyCreatorLeft(const Collection& collection) const;
	void UpdateFloatProviders();

	std::shared_ptr<const psys::ParticleFile> _file;
	EffectServices _services;
	std::vector<std::unique_ptr<Modifier>> _modifiers;
	std::array<std::vector<const Modifier*>, k_GroupCount> _groups;
	std::array<bool, k_GroupCount> _hierarchies {};
	std::unordered_map<std::string, std::unique_ptr<Creator>> _creators;
	std::unordered_map<std::string, float> _floatValues;
	std::vector<std::unique_ptr<Collection>> _roots;
	std::vector<std::string> _unported;
	glm::vec3 _origin;
	float _magnitude;
	float _age {0.0f};
	float _closeAge {0.0f};
	float _dt {0.1f};
	bool _firstStepTwice {false};
	bool _closing {false};
	bool _deleteOnCloseDown {true};
	float _maxSpellAge {-1.0f};
	size_t _atomCount {0};
	bool _synced;
	SpellSink* _sink {nullptr};
	ProcessInfo _info {};
	int _player {-1};
	float _globalAlpha {255.0f};
	bool _hidden {false};
	glm::vec3 _direction {0.0f};
	std::vector<entt::entity> _targets;
	std::vector<glm::vec3> _targetPositions;
};

} // namespace openblack::particles
