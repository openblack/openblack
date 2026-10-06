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

	_Count
};
constexpr size_t k_FacetCount = static_cast<size_t>(Facet::_Count);
[[nodiscard]] std::string_view Name(Facet facet);

/// The testbed's land: the plain, or the plain just above the sea with a pool of it in front of the camera
enum class Land : uint8_t
{
	Plain,
	Pool,
};

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
	Land land {Land::Plain};
	/// The hour of the day, as scripts give it, and whether the clock then runs on or stands still
	float hour {12.0f};
	bool clockRuns {true};
	Weather weather {Weather::Clear};
	/// Game turns of the creatures' bodies that pass each game turn
	float bodyTimeScale {1.0f};
	/// Whether creatures faint when exhausted, starved or out of life
	bool fainting {true};
	/// Smiley faces for footprints as on the first of April, or by the date when not given
	std::optional<bool> aprilFools;
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
	/// Further points kept in view by the overview, from the middle of the map, such as the pool; it keeps the creatures,
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
};

/// Something put on the land for the creatures: food, a tree, or a feature such as a pillar of rock
struct ObjectSetup
{
	std::variant<MobileObjectInfo, TreeInfo, FeatureInfo> type;
	glm::vec2 offset {0.0f};
	float scale {1.0f};
	float yawDegrees {0.0f};
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
	};
	Kind kind {Kind::Stop};
	/// Which creature, by its place in the scenario's creatures
	size_t creature {0};
	/// Seconds after the last command before this one, counted once its creature is free when it waits for that
	float delaySeconds {0.0f};
	/// Waits until its creature has stopped moving and its body plays nothing, such as having arrived
	bool waitUntilFree {false};
	/// The point gone to, fled from or faced, from the middle of the map
	glm::vec2 point {0.0f};
	/// The animation played, or the creature followed
	size_t value {0};
	/// The hour jumped to
	float hour {12.0f};
};
[[nodiscard]] std::string_view Name(Command::Kind kind);

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
	std::vector<Command> commands;
	/// After the last command, the commands go round again from this one
	std::optional<size_t> repeatFrom;
};

/// Every scenario, in the order the window lists them
[[nodiscard]] std::span<const Scenario> All();
[[nodiscard]] const Scenario* Find(std::string_view id);
/// What is wrong with a scenario's data, if anything, one line each
[[nodiscard]] std::vector<std::string> Problems(const Scenario& scenario);

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
