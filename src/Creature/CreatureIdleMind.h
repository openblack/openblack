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
#include "Creature/CreatureFace.h"

/// What a creature does with itself while it has nothing better to do, decided once a game turn. It works through an
/// agenda of steps: waiting while it looks about, playing an action, sitting for a while, going somewhere. When the
/// agenda runs out it picks the next: showing the player its strongest desire, at most once a minute; else sitting down
/// for a while; else hanging around, which is walking somewhere nearby and sitting there; else being idle, which is
/// waiting a second or two then a tired yawn, twice. Each step pulls a face for what it does or feels as it starts, and
/// again every few seconds while it lasts: idling, a random face for three seconds.
///
/// Before any of that, a creature in need sees to it: hungry with food at hand it picks it up, looks it over and eats
/// it, tired it sleeps on the spot, needing a poo it has one, thirsty with water in reach it goes and drinks. The
/// strongest such need is seen to first. Exhausted, starved or out of life, it faints wherever it is, and comes round a
/// little later. With no need, a curious creature picks up something nearby and looks it over, a playful one throws it
/// about, and an angry one hurls it at a home or a tree. Holding something it has no more use for, it puts it down.
///
/// Only this choice of the next agenda is the idle policy; a planner that weighs desires against the actions that
/// satisfy them can choose agendas in its place, and the steps play out the same way.
namespace openblack::creature_mind
{
/// The seconds between showing desires
constexpr float k_ShowDesireSeconds = 60.0f;
/// Each step pulls a face as it starts, and again this often for as long as it lasts: every four seconds and two turns
constexpr float k_FaceRepeatSeconds = 4.2f;
/// After each face it pulls, the creature's variety of faces moves on by up to this much less one
constexpr uint32_t k_FaceVarietyStep = 7;
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
/// A curious, playful or angry creature this strongly so does something about it with what is nearby. The game weighs
/// these desires against everything else through its planner; this stands in for that.
constexpr float k_ActOnDesire = 0.3f;
/// Looking something over, it puts it down gently most times, out of this many, and tosses it away the rest
constexpr uint32_t k_PutDownLots = 10;
constexpr uint32_t k_TossLots = 2;
/// Throwing something about, it throws it this far away and up to this much further, in a random direction
constexpr float k_ThrowAroundDistance = 30.0f;
constexpr uint32_t k_ThrowAroundExtra = 21;
/// Going up to something to act at it, it stops this close; following something, it keeps this close
constexpr float k_ApproachDistance = 12.0f;
constexpr float k_FollowDistance = 25.0f;
/// Off to hurl something, it shows its anger first one time in this many
constexpr uint32_t k_AngryBeforeHurlLots = 6;

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
	/// Doing things with what is nearby: picking something up to look it over, throwing it about, hurling it at
	/// something, and putting down what it holds
	Examine,
	PlayWithObject,
	Hurl,
	PutDown,
	/// Something else the planner chose to satisfy a desire: an emote, looking at or following something, running away,
	/// destroying something, greeting something
	Planned,
};
[[nodiscard]] std::string_view Name(Activity activity);

/// Something done with the creature's hands to a thing about it
struct ObjectOrder
{
	enum class Kind : uint8_t
	{
		PickUp,
		PutDown,
		Discard,
		Eat,
		/// Stroking, shaking, smelling or examining what it holds, by the animation
		Keep,
		/// Throwing what it holds at a point, or at a point as far from where the creature is as the point is from the
		/// origin
		Throw,
		ThrowNearby,
		Destroy,
		/// Pointing at a point
		PointAt,
	};
	Kind kind {Kind::PickUp};
	/// What it acts on, by its entity's number
	std::optional<uint32_t> object;
	glm::vec2 point {0.0f};
	size_t animation {0};
};

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
		/// Going near an object, keeping the step's distance from it; getting away from it to that distance; turning
		/// to face it and holding still a moment, the step's seconds. Each goes on until the creature's movement says
		/// it is done or has failed.
		GoNearObject,
		GetAwayFromObject,
		TurnToFaceObject,
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

/// A miracle cast at a thing, by its magic type's number
struct CastOrder
{
	uint32_t magicType {0};
	/// What it is cast at, by its entity's number
	std::optional<uint32_t> object;
};

/// What a step does to the creature's body
enum class Effect : uint8_t
{
	None,
	/// Has eaten what it held
	Eat,
	Drink,
	Poo,
	Puke,
	/// Wakes up rested
	Slept,
	/// Comes round from a faint
	CameRound,
	/// Has looked something over, thrown something about, and hurled something
	Examined,
	ThrewAbout,
	Hurled,
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
		/// Doing something with a thing until it is done; if it can't, the rest of the agenda is given up
		Object,
		/// Casting a miracle: the start of the casting pose, then the miracle as its loop begins, held for the step's
		/// seconds counted in whole turns, then let go of as the end plays; if it can't be cast, the rest of the agenda
		/// is given up
		Cast,
		/// Drawing a gesture in the air with its hand, by the gesture's number in the step's animation, until the
		/// creature's movement says it is done or has failed
		Gesture,
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
	/// What an object step does
	ObjectOrder order {};
	/// What a cast step casts
	CastOrder cast {};
	/// The face pulled as the step starts
	creature_face::Cue face {creature_face::Cue::None};
};

struct IdleMind
{
	Activity activity {Activity::None};
	std::vector<Step> agenda;
	size_t step {0};
	bool stepStarted {false};
	float stepSeconds {0.0f};
	bool sitEnding {false};
	/// Seconds until the step's face is pulled again, and the count that varies the faces it pulls
	float faceSeconds {0.0f};
	uint32_t faceVariety {0};
	/// Seconds until it may show a desire again
	float showDesireSeconds {0.0f};
	/// The desire it showed last
	std::optional<creature_desires::Desire> shown;
	/// Told to wake up, or to end whatever static step it is in
	bool wakeWanted {false};
	/// Counts the agendas planned, so that a new one can be told from the last
	uint32_t serial {0};
	/// The agenda was given up before its end, as a step it needed couldn't be done
	bool gaveUp {false};
	/// A cast step: whether its miracle has been cast, and the turns it is held for yet
	bool castDone {false};
	uint32_t castTurns {0};
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
	/// How curious, playful and angry it is, 0 for desires it doesn't have yet
	float curiosity {0.0f};
	float play {0.0f};
	float anger {0.0f};
	/// Whether it holds something
	bool holding {false};
	/// The nearest thing it could pick up, by its entity's number, and where it is
	std::optional<uint32_t> object;
	glm::vec2 objectPoint {0.0f};
	/// The nearest home or tree it might hurl something at
	std::optional<glm::vec2> hurlTarget;
};

/// How what its hands were told to do is going
enum class HandsState : uint8_t
{
	/// Told nothing, or it was stopped
	Idle,
	Busy,
	Done,
	Failed,
};

/// How going near, getting away from or turning to face an object is going
enum class SubMove : uint8_t
{
	Running,
	Done,
	Failed,
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
	/// How going near, getting away from or turning to face an object, or drawing a gesture, is going, by the
	/// creature's movement
	SubMove subMove {SubMove::Running};
	glm::vec2 position {0.0f};
	std::optional<creature_desires::Desire> strongest;
	/// Seconds since the player last stroked or slapped it, and which
	std::optional<float> feedbackSeconds;
	bool feedbackWasStroke {false};
	/// Its needs, and whether it has slept enough to wake
	Wants wants {};
	bool rested {false};
	HandsState hands {HandsState::Idle};
	/// What picks the faces it pulls
	creature_face::Feelings feelings {};
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
	/// A face to pull, or to relax the face pulled
	std::optional<creature_face::Request> face;
	bool relaxFace {false};
	Eyes eyes {Eyes::Unchanged};
	/// Whether the head turns to whatever is interesting this turn
	bool lookAbout {false};
	/// Where to go, or to stop going
	std::optional<Movement> move;
	bool stopMoving {false};
	/// What happens to the body this turn, and the object it eats
	Effect effect {Effect::None};
	std::optional<uint32_t> effectObject;
	/// What to do with a thing
	std::optional<ObjectOrder> object;
	/// A gesture to draw, by its number
	std::optional<uint32_t> gesture;
	/// A miracle to cast, and whether to let go of the one it holds
	std::optional<CastOrder> cast;
	bool releaseCast {false};
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
/// Picking food up, examining it and eating it
[[nodiscard]] std::vector<Step> Eat(uint32_t food);
/// Picking something up, stroking, shaking, smelling or examining it, then mostly putting it down and sometimes tossing it
/// away
[[nodiscard]] std::vector<Step> ExamineByPickingUp(uint32_t object, const Random& random);
/// Picking something up and throwing it about, 30 to 50 units away in a random direction
[[nodiscard]] std::vector<Step> ThrowAbout(uint32_t object, const Random& random);
/// A sixth of the time showing its anger, then picking something up and hurling it at a point
[[nodiscard]] std::vector<Step> Hurl(uint32_t object, glm::vec2 target, const Random& random);
/// Putting down what it holds
[[nodiscard]] std::vector<Step> PutDownHeld();
/// Walking to the water's edge, turning to the water and drinking
[[nodiscard]] std::vector<Step> Drink(glm::vec2 shore, glm::vec2 water);
/// A poo on the spot, a third of the time showing it needs one first
[[nodiscard]] std::vector<Step> Poo(const Random& random);
/// Being sick on the spot
[[nodiscard]] std::vector<Step> Puke();
/// Fainting where it stands, lying out cold a while, then getting up
[[nodiscard]] std::vector<Step> Faint();
/// Eating what it holds
[[nodiscard]] std::vector<Step> EatHeld();
/// Playing an action once on the spot
[[nodiscard]] std::vector<Step> Emote(size_t animation);
/// Turning to face a point, then playing an action
[[nodiscard]] std::vector<Step> FaceAndEmote(glm::vec2 point, size_t animation);
/// Walking up to something, then playing an action at it
[[nodiscard]] std::vector<Step> ApproachAndEmote(uint32_t object, size_t animation);
/// Turning to face something and watching it for some seconds
[[nodiscard]] std::vector<Step> LookAt(glm::vec2 point, float seconds);
/// Following something about for some seconds
[[nodiscard]] std::vector<Step> FollowFor(uint32_t object, float seconds);
/// Running away from a point
[[nodiscard]] std::vector<Step> RunFrom(glm::vec2 point);
/// Walking up to something and destroying it
[[nodiscard]] std::vector<Step> DestroyThing(uint32_t object);
/// Waiting a while, looking about
[[nodiscard]] std::vector<Step> LookAbout(float seconds);
/// The need to see to first, if any is strong enough and the means are at hand, and the agenda for it
struct NeedPlan
{
	Activity activity;
	std::vector<Step> agenda;
};
[[nodiscard]] std::optional<NeedPlan> ChooseNeed(const Wants& wants, const Random& random);
/// What to do about curiosity, play or anger, if any is strong enough and there is something at hand to do it with, or
/// with something held and nothing more to do with it, putting it down
[[nodiscard]] std::optional<NeedPlan> ChooseObjectActivity(const Wants& wants, const Random& random);
/// Whether the creature is asleep, or out cold, in its current step
[[nodiscard]] bool IsAsleep(const IdleMind& mind);
[[nodiscard]] bool IsUnconscious(const IdleMind& mind);

/// A new agenda in place of the current one
void Plan(IdleMind& mind, Activity activity, std::vector<Step> agenda);
/// The next agenda when the current runs out
void ChooseNext(IdleMind& mind, const Senses& senses, const Random& random);
/// Pulls a face for a reason, if the reason calls for one, and moves the variety of faces on
[[nodiscard]] std::optional<creature_face::Request> PullFace(IdleMind& mind, creature_face::Cue cue, const Senses& senses,
                                                             const Random& random);
/// One game turn of the mind
[[nodiscard]] Commands Think(IdleMind& mind, const Senses& senses, const Random& random);
} // namespace openblack::creature_mind
