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

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

#include "Creature/CreatureDesires.h"

/// What a creature does with itself while it has nothing better to do, decided once a game turn. It works through an
/// agenda of steps: waiting while it looks about, playing an action, sitting for a while, going somewhere. When the
/// agenda runs out it picks the next: showing the player its strongest desire, at most once a minute; else sitting down
/// for a while; else hanging around, which is walking somewhere nearby and sitting there; else being idle, which is
/// waiting a second or two then a tired yawn, twice. At the start of each step it pulls a face for three seconds.
///
/// Before any of that, a creature in need sees to it: hungry with food at hand it eats, tired it sleeps on the spot,
/// needing a poo it has one, thirsty with water in reach it goes and drinks. The strongest such need is seen to first.
/// Exhausted, starved or out of life, it faints wherever it is, and comes round a little later.
///
/// Only this choice of the next agenda is the idle policy; a planner that weighs desires against the actions that
/// satisfy them can choose agendas in its place, and the steps play out the same way.
namespace openblack::creature_mind
{
/// The seconds between showing desires
constexpr float k_ShowDesireSeconds = 60.0f;
/// A face lasts this long
constexpr float k_FaceSeconds = 3.0f;
/// Within this many seconds of being stroked or slapped, the creature shows its pleasure or sorrow at it
constexpr float k_FeedbackSeconds = 10.0f;
/// Idle waits are one second and up to one more
constexpr float k_IdleWaitSeconds = 1.0f;
/// Being idle is waiting then yawning, this many times
constexpr int k_IdleRepeats = 2;
/// Sitting lasts ten seconds and up to four more
constexpr uint32_t k_SitSeconds = 10;
constexpr uint32_t k_SitExtraSeconds = 5;
/// A desire weaker than this isn't worth showing. The game shows desires when its planner picks the desire to show how
/// it is over all others; this stands in for that.
constexpr float k_MinDesireShown = 0.2f;
/// The idle activities are chosen at random among this many lots: the first is sitting down, the last hanging
/// around, the rest being idle. The game chooses between its idle activities by the strength of the desire to rest and
/// what the creature has learnt; this stands in for that.
constexpr uint32_t k_ActivityLots = 4;
/// Hanging around, the creature walks this far away and up to this much further, in a random direction
constexpr float k_HangAroundDistance = 20.0f;
constexpr uint32_t k_HangAroundExtra = 21;
/// A need this strong is seen to. The game weighs needs against everything else the creature might do through its
/// planner; this stands in for that.
constexpr float k_ActOnNeed = 0.3f;
/// Off to sleep, the creature yawns first one time in this many; off for a poo, it shows it needs one first one time in
/// this many
constexpr uint32_t k_YawnBeforeSleepLots = 2;
constexpr uint32_t k_ShowPooLots = 3;
/// A poo and being sick each last this many seconds
constexpr float k_PooSeconds = 4.0f;
constexpr float k_PukeSeconds = 4.0f;
/// Fainted, it lies out cold this long before it gets up. The game's time is not known; this stands in for it.
constexpr float k_FaintSeconds = 10.0f;
/// It drinks from anywhere this close to the water's edge
constexpr float k_DrinkReach = 5.0f;

/// What the creature is doing
enum class Activity : uint8_t
{
	None,
	BeIdle,
	Sit,
	ShowDesire,
	/// Walking somewhere nearby and sitting there
	HangAround,
	/// Something it was told to do
	Told,
	/// Seeing to its body: eating, drinking, sleeping, having a poo, being sick, and lying out cold
	Eat,
	Drink,
	Sleep,
	Poo,
	Puke,
	Faint,
};
[[nodiscard]] std::string_view Name(Activity activity);

/// Going somewhere, or turning to face something
struct Movement
{
	enum class Kind : uint8_t
	{
		/// To a point, or to a point as far from where the creature is as the point is from the origin
		ToPoint,
		Nearby,
		/// Up to an object, or following it until the step's time is up
		ToObject,
		Follow,
		/// Running away from a point
		FleeFrom,
		TurnToFace,
	};
	Kind kind {Kind::ToPoint};
	glm::vec2 point {0.0f};
	/// The object walked up to or followed, by its entity's number
	std::optional<uint32_t> object;
	bool run {false};
	/// Arriving anywhere from min to max from the point, or keeping within max of what it follows
	float minDistance {0.0f};
	float maxDistance {0.0f};
};

/// What a step does to the creature's body
enum class Effect : uint8_t
{
	None,
	/// Eats its object
	Eat,
	Drink,
	Poo,
	Puke,
	/// Wakes up rested
	Slept,
	/// Comes round from a faint
	CameRound,
};

struct Step
{
	enum class Kind : uint8_t
	{
		/// Waiting while looking about
		Wait,
		/// An action played once
		Action,
		/// A start, a loop for a while and an end, such as sitting down, sitting while looking about, and getting up
		Static,
		/// Going somewhere until it arrives or gives up, or for the step's seconds when they are more than 0
		Move,
	};
	Kind kind {Kind::Wait};
	float seconds {0.0f};
	size_t animation {0};
	/// Played with drooping, slowly blinking eyes, as if just woken up
	bool sleepyEyes {false};
	Movement movement {};
	/// A static step's start, loop and end
	std::array<size_t, 3> sequence {};
	/// A static step's loop lasts until the creature is rested, rather than for the step's seconds
	bool untilRested {false};
	/// A static step's loop holds the last frame of its start, as lying out cold
	bool holdLoop {false};
	/// Played with the eyes closed
	bool closedEyes {false};
	/// What the step does to the body: when an action is done (eating, as it starts), or a static step's loop ends
	Effect effect {Effect::None};
	/// The object eaten, by its entity's number
	std::optional<uint32_t> object;
};

struct IdleMind
{
	Activity activity {Activity::None};
	std::vector<Step> agenda;
	size_t step {0};
	bool stepStarted {false};
	float stepSeconds {0.0f};
	bool sitEnding {false};
	/// Seconds the face is held for yet
	float faceSeconds {0.0f};
	/// Seconds until it may show a desire again
	float showDesireSeconds {0.0f};
	/// The desire it showed last
	std::optional<creature_desires::Desire> shown;
	/// Told to wake up, or to end whatever static step it is in
	bool wakeWanted {false};
};

/// The needs it might see to, and the means at hand
struct Wants
{
	/// How strong each need's desire is, 0 for desires it doesn't have yet
	float hunger {0.0f};
	float tiredness {0.0f};
	float poo {0.0f};
	float water {0.0f};
	/// The food nearest to hand, by its entity's number, and where it is
	std::optional<uint32_t> food;
	glm::vec2 foodPoint {0.0f};
	/// The nearest water, and where it can stand at its edge to drink
	struct WaterSpot
	{
		glm::vec2 shore;
		glm::vec2 water;
	};
	std::optional<WaterSpot> waterSpot;
};

/// What the mind knows this turn
struct Senses
{
	float seconds {0.0f};
	/// Whether the body plays an action or is on the move, and whether it is in the loop of an action, such as sitting
	bool bodyBusy {false};
	bool bodyLooping {false};
	/// Whether it is on its way somewhere or turning, and where it stands
	bool moving {false};
	glm::vec2 position {0.0f};
	std::optional<creature_desires::Desire> strongest;
	/// Seconds since the player last stroked or slapped it, and which
	std::optional<float> feedbackSeconds;
	bool feedbackWasStroke {false};
	/// Its needs, and whether it has slept enough to wake
	Wants wants {};
	bool rested {false};
};

/// How the eyes should look
enum class Eyes : uint8_t
{
	Unchanged,
	/// Drooping and slowly blinking, as if just woken up
	Sleepy,
	Normal,
	Closed,
};

/// What the mind tells the body
struct Commands
{
	std::optional<size_t> playOnce;
	bool mirrored {false};
	/// A start, loop and end to play, the loop held on its last frame or not, and the end of the loop playing now
	std::optional<std::array<size_t, 3>> startSequence;
	bool holdLoop {false};
	bool endSit {false};
	/// A face to pull, or none to relax it
	std::optional<std::optional<size_t>> face;
	Eyes eyes {Eyes::Unchanged};
	/// Whether the head turns to whatever is interesting this turn
	bool lookAbout {false};
	/// Where to go, or to stop going
	std::optional<Movement> move;
	bool stopMoving {false};
	/// What happens to the body this turn, and the object it eats
	Effect effect {Effect::None};
	std::optional<uint32_t> effectObject;
};

/// random(n) is a whole number from 0 to n - 1
using Random = std::function<uint32_t(uint32_t)>;

/// Being idle: wait a second or two looking about, then a tired yawn with sleepy eyes, twice
[[nodiscard]] std::vector<Step> BeIdle(const Random& random);
/// Sitting for ten to fourteen seconds
[[nodiscard]] Step SitDown(const Random& random);
/// Hanging around: walking 20 to 40 units away in a random direction, then sitting there
[[nodiscard]] std::vector<Step> HangAround(const Random& random);
/// Sleeping on the spot: half the time a tired yawn first, then sleeping with its eyes closed until rested, then a dazed
/// look about with sleepy eyes
[[nodiscard]] std::vector<Step> Sleep(const Random& random);
/// Going up to food and eating it
[[nodiscard]] std::vector<Step> Eat(uint32_t food);
/// Walking to the water's edge, turning to the water and drinking
[[nodiscard]] std::vector<Step> Drink(glm::vec2 shore, glm::vec2 water);
/// A poo on the spot, a third of the time showing it needs one first
[[nodiscard]] std::vector<Step> Poo(const Random& random);
/// Being sick on the spot
[[nodiscard]] std::vector<Step> Puke();
/// Fainting where it stands, lying out cold a while, then getting up
[[nodiscard]] std::vector<Step> Faint();
/// The need to see to first, if any is strong enough and the means are at hand, and the agenda for it
struct NeedPlan
{
	Activity activity;
	std::vector<Step> agenda;
};
[[nodiscard]] std::optional<NeedPlan> ChooseNeed(const Wants& wants, const Random& random);
/// Whether the creature is asleep, or out cold, in its current step
[[nodiscard]] bool IsAsleep(const IdleMind& mind);
[[nodiscard]] bool IsUnconscious(const IdleMind& mind);

/// A new agenda in place of the current one
void Plan(IdleMind& mind, Activity activity, std::vector<Step> agenda);
/// The next agenda when the current runs out
void ChooseNext(IdleMind& mind, const Senses& senses, const Random& random);
/// One game turn of the mind
[[nodiscard]] Commands Think(IdleMind& mind, const Senses& senses, const Random& random);
} // namespace openblack::creature_mind
