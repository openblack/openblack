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

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureDesires.h"
#include "Creature/CreatureMarks.h"
#include "Creature/CreaturePhysiology.h"
#include "Creature/CreatureTattoo.h"
#include "Enums.h"
#include "Particles/ParticleDrawPath.h"
#include "TestbedCrowd.h"

/// The testbed's scenarios: ready-made set ups of the flat testbed that show one facet of the creatures at a time, such
/// as a thirsty creature finding water or a lineup of bodies. Each is plain data: the land, time of day and weather,
/// where the camera looks, the creatures with their bodies, needs and desires, the objects about them, and a timeline of
/// commands. The debug window's runner applies them through the game's systems; everything here is pure, so the
/// registry and the maths of applying it are tested on their own.
namespace openblack::testbed_scenarios
{

/// Which side of the creatures a scenario shows, for finding it in the list
enum class Facet : uint8_t
{
	/// What it does with nothing to do: fidgeting, sitting, wandering off
	Idle,
	/// Its faces, gestures and the emotes that show its desires
	Expressions,
	/// What it looks at
	Senses,
	/// Its body's needs and how it sees to them: thirst, hunger, sleep, poo, sick, fainting, cold
	Needs,
	/// Growing up and the shapes its body takes
	Growth,
	/// Its skins, hair, tattoos, wounds and eyes
	Appearance,
	/// Its shadow and its reflection
	Light,
	/// Walking, running, turning and finding its way
	Movement,
	Footprints,
	Audio,
	/// Picking things up, looking them over, putting them down, throwing them, eating them, knocking them down,
	/// pointing, and what curiosity, play and anger have it do with them
	Objects,
	/// The player's hand on it: stroking, slapping, and its status panel
	Hand,
	/// Being led on the leashes, tied up and kept at home
	Leash,
	/// Fighting another creature, being knocked out and coming round
	Combat,
	/// What it learns: from strokes and slaps, from mind files, as it grows up, and by copying the player
	Mind,
	/// The particle effects of the miracles and spot visuals
	Particles,
	/// Set ups for trying the in-game editor on: picking, moving and placing things, and its cameras
	Editor,
	/// The miracles: dispensers, casting them and what they do, the creature spells among them
	Miracles,
	/// Crowds of hundreds to thousands of creatures or villagers, all fully simulated, for measuring how the game's
	/// costs grow with their number
	Benchmark,
	/// Creature Mode, the camera locked onto a creature, and the Creature Cave
	CreatureMode,

	_Count
};
constexpr size_t k_FacetCount = static_cast<size_t>(Facet::_Count);
[[nodiscard]] std::string_view Name(Facet facet);

/// The weather laid over the whole island, the climates breeding no storms of their own; clear is a mild day
enum class Weather : uint8_t
{
	Clear,
	Rain,
	Thunderstorm,
	Snow,
	Blizzard,
};
[[nodiscard]] std::string_view Name(Weather weather);

struct Environment
{
	/// The hour of the day, as scripts give it, and whether the clock then runs on or stands still
	float hour {12.0f};
	bool clockRuns {true};
	Weather weather {Weather::Clear};
	/// Game turns of the creatures' bodies that pass each game turn
	float bodyTimeScale {1.0f};
	/// Whether creatures faint when exhausted, starved or out of life
	bool fainting {true};
	/// Whether angry creatures pick fights with others nearby by themselves
	bool angerStartsFights {false};
	/// Smiley faces for footprints as on the first of April, or by the date when not given
	std::optional<bool> aprilFools;
	/// The grid of every miracle's dispenser the testbed lays out in front of the camera stays, or is cleared away
	bool dispenserGrid {true};
	/// The player's alignment, from -1 (evil) to 1 (good), as it was when not given
	std::optional<float> playerAlignment;
	/// Where the cursor, and so the hand, is put, as a share of the window from its top left, until the mouse moves
	std::optional<glm::vec2> cursor;
};

/// Where the camera looks as the scenario starts
enum class Shot : uint8_t
{
	/// Where the testbed puts it: above and behind the middle of the map, looking north
	Testbed,
	/// Above and to the south of everything in the scenario, all of it in view
	Overview,
	/// Behind and above one creature, keeping up with it
	Follow,
	/// In front of one creature's face, looking at its head and eyes
	Head,
};
[[nodiscard]] std::string_view Name(Shot shot);

struct Framing
{
	Shot shot {Shot::Overview};
	/// The creature followed or looked at, by its place in the scenario's creatures
	size_t creature {0};
	/// Further points kept in view by the overview, from the middle of the map, such as the lake; it keeps the creatures,
	/// the objects and wherever the commands send the creatures in view by itself
	std::vector<glm::vec2> include;
	/// Further away for more than 1, closer for less
	float distance {1.0f};
};

/// A creature's body as the scenario wants it; what is not given is as the creature's species starts
struct NeedOverrides
{
	std::optional<float> energy;
	std::optional<float> exhaustion;
	std::optional<float> dehydration;
	std::optional<float> poo;
	std::optional<float> life;
	std::optional<float> warmth;
	/// Hours of game time it has lived
	std::optional<uint32_t> age;

	[[nodiscard]] bool Empty() const;
};

/// A desire set to a fraction of its maximum, and activated if the creature's stage of growing up hadn't brought it
struct DesireOverride
{
	creature_desires::Desire desire;
	float fraction {1.0f};
};

struct CreatureSetup
{
	/// What the readout calls it, such as "fat"; the species' name when empty
	std::string_view label;
	CreatureType species {CreatureType::Tiger};
	/// Where it stands, from the middle of the map, x east and y north in units of the land
	glm::vec2 offset {0.0f};
	/// Its facing as the spawner gives it: 0 faces south, towards the testbed's camera, and 180 faces north
	float facingDegrees {0.0f};
	PlayerNames owner {PlayerNames::PLAYER_ONE};
	/// Whether it is the one creature its owner can lead on the leash. When not given, a player's first creature is
	/// and the others aren't.
	std::optional<bool> leashable;
	/// Its alignment, fatness and strength, the species' start when not given, and its size, 1 when not given: a
	/// creature well grown, about 15 units tall, rather than as small as a new one starts
	std::optional<float> alignment;
	std::optional<float> fatness;
	std::optional<float> strength;
	std::optional<float> size;
	/// How far it has grown up, from 0; fully grown up when not given
	std::optional<uint32_t> phase;
	NeedOverrides needs;
	std::vector<DesireOverride> desires;
	/// The needs and desires are set again every frame, so they stay as given however time goes
	bool hold {false};
	/// The mind leaves the body alone, to be posed by the commands
	bool pauseMind {false};
	std::vector<creature_tattoo::Slot> tattoos;
	/// Wounds and burns, and drops of blood, on its skin
	std::vector<creature_marks::Mark> wounds;
	std::vector<creature_marks::Mark> blood;
	/// A mind file it takes up as it starts: "game:NAME" for one of the game's Scripts/CreatureMind files, or
	/// "chosen:NAME" for the one last opened with the spawner's file dialog, falling back to the game's file NAME
	std::string_view mindFile;
};

/// Something put on the land for the creatures: an object, a tree, a feature such as a pillar of rock, a villager, or a
/// pot or pile of food or wood
struct ObjectSetup
{
	std::variant<MobileObjectInfo, TreeInfo, FeatureInfo, VillagerInfo, PotInfo> type;
	glm::vec2 offset {0.0f};
	float scale {1.0f};
	float yawDegrees {0.0f};
	/// How much a pot holds, which for food is what it is worth to eat: a meal for a grown up creature by default
	int32_t amount {800};
};

/// A particle effect played on the land
struct ParticleSetup
{
	ParticleType type {ParticleType::Smoke};
	/// An effect file played in place of the type's, by its name, for the effects no particle type names
	std::string_view file;
	particles::draw::DrawPath path {particles::draw::DrawPath::Sorted};
	/// The scenario's creatures are given to it to act on, as the heal miracle is given the people it heals
	bool targetsCreatures {false};
	glm::vec2 offset {0.0f};
	/// Above the land
	float height {0.0f};
	float magnitude {1.0f};
	/// The player whose colour it takes where it takes one
	int player {0};
	/// Seconds after which it closes down and starts again, for effects that end; none to run until the scenario stops
	float restartSeconds {0.0f};
};

/// A miracle dispenser, or a one-shot bubble on its own, put down for the scenario
struct DispenserSetup
{
	MagicType type {MagicType::Fireball};
	glm::vec2 offset {0.0f};
	/// Only the bubble, floating this high above the land, without its dispenser
	std::optional<float> bubbleHeight;
};

/// A miracle cast in the scenario, as if from a hand above the land
struct MiracleCast
{
	enum class Target : uint8_t
	{
		/// At the point
		Point,
		/// On one of the scenario's creatures, by its place
		Creature,
	};
	MagicType type {MagicType::Fireball};
	Target target {Target::Point};
	/// The point cast at, from the middle of the map
	glm::vec2 point {0.0f};
	size_t creature {0};
	/// Where the hand casting it is, from the middle of the map, and how high above the land
	glm::vec2 handOffset {0.0f};
	float handHeight {15.0f};
	/// The hand's movement as it lets go, which throws a fireball
	glm::vec3 throwVelocity {0.0f};
	/// Seconds after the scenario starts that it is cast
	float delaySeconds {1.0f};
	/// A miracle held in the hand (lightning, water, food, wood) runs this long, then stops; none to run its course
	std::optional<float> holdSeconds;
	/// Seconds after which it is cast again, none for once
	std::optional<float> repeatSeconds;
};

/// Something a creature is told to do, in turn with the scenario's other commands
struct Command
{
	enum class Kind : uint8_t
	{
		/// Going to the point, by walking or running; following the other creature; running away from the point;
		/// turning to face the point, or the camera; stopping
		WalkTo,
		RunTo,
		Follow,
		FleeFrom,
		TurnToFace,
		FaceCamera,
		Stop,
		/// Playing the animation: an action, a gesture on top of the body, or a face
		PlayAction,
		PlayGesture,
		PullFace,
		/// Pulling the face its mind would for a feeling or what it does (value, by the face cues)
		ShowFeeling,
		SitDown,
		StandUp,
		/// Seeing to a need now: sleeping, waking, eating the nearest food, drinking at the nearest water, having a
		/// poo, being sick, fainting
		Sleep,
		Wake,
		Eat,
		Drink,
		Poo,
		Puke,
		Faint,
		/// As if the player's hand stroked or slapped it
		Stroke,
		Slap,
		/// The hour of the day jumps, for every creature
		SetHour,
		/// Walking up to the scenario's object and picking it up; then, with what it holds, putting it down, tossing it
		/// away, lobbing it, eating it, or looking it over with one of its four ways of doing so (stroking, shaking,
		/// smelling, examining); throwing it at the point
		PickUp,
		PutDown,
		Discard,
		Lob,
		EatHeld,
		Examine,
		ThrowAt,
		/// Walking up to the scenario's object and knocking it down; pointing at the point
		KnockDown,
		PointAt,
		/// The player's hand stroking a part of its body, or slapping it at a height and hard or gently, held to it
		/// until the hand lets go, which tells its mind how it was treated
		HandStroke,
		HandSlap,
		HandLetGo,
		/// Putting a leash on it, which it is first taught; tying the leash to the scenario's object, untying it to the
		/// hand, taking it off; keeping it within a radius of where it stands
		PutOnLeash,
		TieLeash,
		UntieLeash,
		TakeOffLeash,
		ConfineToHome,
		/// Making it the one creature its owner can lead, which the owner's other creature stops being; the player's
		/// hand tapping it to put the picked leash on, as the Action button does; the player pressing a leash shortcut
		/// (value: 0 the leash key L, 1 the previous leash V, 2 the next leash B), which acts on the player's creature
		MakeLeashable,
		HandTapLeash,
		LeashKey,
		/// The player shaking the hand: the cursor swept quickly back and forth through the same tracking the mouse goes
		/// through, which takes off a leash held in the hand
		LeashShake,
		/// Fighting the other creature; then, in the fight, a blow high, in the middle or low charged for a while, a
		/// block, a step forward, back, right or left, the special move, and fighting by itself or not, as the player's
		/// clicks and the debug tools give them
		StartFight,
		FightBlow,
		FightBlock,
		FightStep,
		FightSpecial,
		FightAuto,
		/// Being knocked out, as a fight's loser is, and brought round again
		KnockOut,
		BringRound,
		/// Tying the leash it wears to the other creature, which has it fight that creature
		TieLeashToCreature,
		/// A desire (by value) is set to a fraction (amount) of its maximum; the creature grows up to a stage (by value)
		SetDesire,
		SetPhase,
		/// From now on, each thing it does to something is judged once done, by the mind's trainer: stroked if the
		/// thing is of a kind (value, the game's belief type, such as 6 for a villager), else slapped
		RewardIf,
		/// It watches a skill (value, by its row of the game's table) or a miracle near it; the player does one of the
		/// deeds creatures copy (value) at the point
		SeeSkill,
		SeeMiracle,
		PlayerDid,
		/// C pressed: Creature Mode locks onto the player's creature, or lets go of it; a double click on the creature;
		/// the cursor keys (value: left, right, up or down) held with Shift, or with Ctrl, for some seconds (amount);
		/// Ctrl and Shift pressed together for a clear view; the camera given back
		CreatureKey,
		DoubleClick,
		CameraKeys,
		ClearCameraView,
		LeaveCreatureMode,
		/// F5 pressed: the Creature Cave opens, on a page (value); a symbol (value) tattooed on a place (body part), or
		/// the place's symbols taken off
		OpenCreatureCave,
		ApplyTattoo,
		RemoveTattoo,
		/// The player's mouse, through the same input the real one goes through: the pointer put at a point on the
		/// screen (point, as fractions of its width and height from the top left); a button (value: 1 left, 2 middle,
		/// 3 right) pressed or let go; the mouse moved by point, as fractions of the screen, over some seconds
		/// (amount); the wheel turned some notches (value) away from the player, or towards with ctrl
		PointerTo,
		PointerPress,
		PointerRelease,
		PointerSweep,
		WheelTurn,
		/// The player's alignment jumps, which the hand shows
		SetAlignment,
	};
	Kind kind {Kind::Stop};
	/// Which creature, by its place in the scenario's creatures
	size_t creature {0};
	/// The player whose hand or keys do it
	PlayerNames player {PlayerNames::PLAYER_ONE};
	/// Seconds after the last command before this one, counted once its creature is free when it waits for that
	float delaySeconds {0.0f};
	/// Waits until its creature has stopped moving and its body plays nothing, such as having arrived
	bool waitUntilFree {false};
	/// The point gone to, fled from or faced, from the middle of the map
	glm::vec2 point {0.0f};
	/// The animation played, the creature followed or fought, the band of a blow (high, middle, low), the step
	/// (forward, back, right, left), or whether it fights by itself (1) or not
	size_t value {0};
	/// The hour jumped to
	float hour {12.0f};
	/// The object picked up, knocked down or tied to, by its place in the scenario's objects
	size_t object {0};
	/// The leash put on
	LeashType leash {LeashType::Rope};
	/// The part of the body stroked, by its place in the hand's parts (the head first)
	size_t bodyPart {0};
	/// How high a slap lands, as a share of the creature's height, whether it is gentle, and which way it sweeps
	float slapHeight {0.85f};
	bool gentle {false};
	bool sweepsRight {false};
	/// How far from where it stands it is kept
	float radius {0.0f};
	/// How long a blow's click is held, charging it
	float chargeMs {0.0f};
	/// A desire's fraction of its maximum, or the seconds the camera's keys are held
	float amount {1.0f};
	/// The camera's keys are held with Ctrl rather than Shift
	bool ctrl {false};
	/// The player's alignment jumped to, from -1 (evil) to 1 (good)
	float alignment {0.0f};
};
[[nodiscard]] std::string_view Name(Command::Kind kind);
/// Whether a command is the player's mouse, which needs no creature
[[nodiscard]] bool IsPointerCommand(Command::Kind kind);

struct Scenario
{
	/// Unique and never changed, for picking it from the command line or a test
	std::string_view id;
	std::string_view name;
	Facet facet {Facet::Idle};
	/// What it sets up and what to look for
	std::string_view description;
	std::string_view expected;
	Environment environment;
	Framing framing;
	std::vector<CreatureSetup> creatures;
	std::vector<ObjectSetup> objects;
	std::vector<ParticleSetup> particles;
	std::vector<DispenserSetup> dispensers;
	std::vector<MiracleCast> miracles;
	std::vector<Command> commands;
	/// After the last command, the commands go round again from this one
	std::optional<size_t> repeatFrom;
	/// A crowd spawned a batch a frame, after the creatures and objects above; the runner measures the frames once it
	/// is all there
	std::optional<Crowd> crowd;
};

/// The miracles' scenarios, added to every scenario by the registry
void AddMiracleScenarios(std::vector<Scenario>& all);
/// Creature Mode's and the Creature Cave's scenarios
void AddCreatureModeScenarios(std::vector<Scenario>& all);
/// The player's hand moving over the land, dragging it and turning and zooming the camera
void AddHandNavigationScenarios(std::vector<Scenario>& all);
/// The scenarios of how the hand looks for its player's alignment
void AddHandLookScenarios(std::vector<Scenario>& all);

/// Every scenario, in the order the window lists them
[[nodiscard]] std::span<const Scenario> All();
[[nodiscard]] const Scenario* Find(std::string_view id);
/// What is wrong with a scenario's data, if anything, one line each
[[nodiscard]] std::vector<std::string> Problems(const Scenario& scenario);
/// Whether the testbed's grid of dispensers stays for a scenario: unless it asks for none, or one of its creatures or
/// things would stand on it
[[nodiscard]] bool KeepsDispenserGrid(const Scenario& scenario);

/// A point given from the middle of the map, on the land
[[nodiscard]] glm::vec2 MapPoint(glm::vec2 middle, glm::vec2 offset);

/// About how tall a creature of a size stands
[[nodiscard]] float CreatureHeight(float size);

/// Where the camera is and what it looks at
struct CameraPlacement
{
	glm::vec3 origin;
	glm::vec3 focus;
};
/// The camera above and to the south of a box on the land, by its middle and half its size east to west and south to
/// north, looking down at its middle from far enough back for all of it to be seen through the fields of view, across
/// and up and down, in radians
[[nodiscard]] CameraPlacement Overview(glm::vec3 centre, glm::vec2 halfSize, glm::vec2 fieldOfView, float distanceFactor);
/// The camera behind and above a creature, by its height, looking at its middle from the south
[[nodiscard]] CameraPlacement Follow(glm::vec3 position, float height, float distanceFactor);
/// The camera in front of a creature's face, by its height, its position and the way it faces on the land
[[nodiscard]] CameraPlacement Head(glm::vec3 position, glm::vec2 ahead, float height, float distanceFactor);
/// The box round points on the land, at least so big: its middle and half its size
struct Bounds
{
	glm::vec2 centre;
	glm::vec2 halfSize;
};
[[nodiscard]] Bounds BoundsOf(std::span<const glm::vec2> points, glm::vec2 minimumHalfSize);

/// Sets a body's needs to the overrides; the energy is at most the creature's size, as a meal can fill it
void Apply(const NeedOverrides& overrides, creature_physiology::Needs& needs, float size);
/// Sets desires to their fractions of their maxima, activating them
void Apply(std::span<const DesireOverride> overrides, creature_desires::Desires& desires);

/// Where the timeline of a scenario's commands is
struct Timeline
{
	/// The next command, and the seconds counted towards its delay: since the last was given, or since its creature
	/// was free when it waits for that
	size_t next {0};
	float seconds {0.0f};
	/// Seconds since the last command was given
	float sinceGiven {0.0f};
	/// The next command's creature has been free since the last command, so its delay counts on whatever the
	/// creature's idle mind has it do meanwhile
	bool freed {false};
	/// Every command has been given and none go round again
	bool done {false};
};
/// The timeline some seconds on: the commands due now, in order. A command waiting for its creature to be free asks
/// isFree; its delay only counts from when the creature first is.
[[nodiscard]] std::vector<size_t> Advance(Timeline& timeline, std::span<const Command> commands,
                                          std::optional<size_t> repeatFrom, float seconds,
                                          const std::function<bool(size_t creature)>& isFree);

} // namespace openblack::testbed_scenarios
