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

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Creature fights: two creatures duel inside a circular arena. Each plays a small set of fight states (starting,
/// standing in its stance, acting, blocking, casting, reeling, fainting) fed by a queue of up to twelve moves. The
/// player fills the queue by clicking the opponent's body high, in the middle or low, their own creature to block, or
/// the ground to step, holding the click to charge a blow. Left alone, the creature fights by itself, more or less
/// aggressively by what it has learnt of fighting. Each blow takes from a fight health of its own, which starts at
/// (life + 1) / 2; at nothing the creature faints, and a quarter of what it lost comes off its real life afterwards.
/// Creatures never die of it: the loser lies out cold a while, is taken home, rests and gets up again.
namespace openblack::creature_fight
{
/// The places of the fight animations in the creature spec file
namespace animations
{
constexpr size_t k_Faint = 106;
constexpr size_t k_GetUp = 107;
/// Played while a charged blow waits to be let go
constexpr size_t k_PowerUp = 108;
constexpr size_t k_StartCast = 110;
constexpr size_t k_Cast = 111;
constexpr size_t k_EndCast = 112;
constexpr size_t k_Start = 122;
constexpr size_t k_Stance = 123;
constexpr size_t k_Finish = 124;
/// The blows: high, eye, belly, pelvis, knee and special, then a second of each
constexpr size_t k_FirstAttack = 125;
constexpr size_t k_AttackCount = 12;
constexpr size_t k_AttackHigh = 125;
constexpr size_t k_AttackEye = 126;
constexpr size_t k_AttackBelly = 127;
constexpr size_t k_AttackPelvis = 128;
constexpr size_t k_AttackKnee = 129;
constexpr size_t k_AttackSpecial = 130;
constexpr size_t k_AttackSpecial2 = 136;
constexpr size_t k_StepForward = 137;
constexpr size_t k_StepBack = 138;
constexpr size_t k_StepRight = 139;
constexpr size_t k_StepLeft = 140;
constexpr size_t k_StartBlock = 142;
constexpr size_t k_Block = 143;
constexpr size_t k_EndBlock = 144;
/// Reeling from a blow high, in the middle and low, each followed by its right, left, top and bottom versions
constexpr size_t k_RecoilHigh = 179;
constexpr size_t k_RecoilMid = 184;
constexpr size_t k_RecoilLow = 189;
/// The wobbles a blow sends through the body, high and low, side to side and front to back, played on top of it
constexpr size_t k_WobbleHighSide = 194;
constexpr size_t k_WobbleHighFrontBack = 195;
constexpr size_t k_WobbleLowSide = 196;
constexpr size_t k_WobbleLowFrontBack = 197;
/// Reeling back while blocking
constexpr size_t k_RecoilBlock = 198;
/// The actions played around a fight: the taunt before it, and being happy, sad or showing off after it
constexpr size_t k_Taunt = 70;
constexpr size_t k_Happy = 55;
constexpr size_t k_Sad = 56;
constexpr size_t k_Impress = 65;
} // namespace animations

[[nodiscard]] bool IsAttack(size_t animation);
[[nodiscard]] bool IsSpecialAttack(size_t animation);
[[nodiscard]] bool IsStep(size_t animation);

/// random(n) is a whole number from 0 to n - 1
using Random = std::function<uint32_t(uint32_t)>;

// The arena

/// The arena's radius is 39 units for each of the bigger creature's size, at most 60
constexpr float k_ArenaRadiusPerSize = 15.0f * 2.6f;
constexpr float k_MaxArenaRadius = 60.0f;
/// The two creatures take their places this many units of their size either side of its middle
constexpr float k_SpotPerSize = 15.0f;
/// Within this many units of their size of their place counts as arrived
constexpr float k_ArrivalPerSize = 1.5f * 15.0f;
/// Moves are only made, and creatures only step back or sideways, within this share of the radius
constexpr float k_RangeShare = 0.8f;

struct Arena
{
	glm::vec2 centre {0.0f};
	float radius {0.0f};
};
[[nodiscard]] float ArenaRadius(float sizeA, float sizeB);
/// The arena between two creatures: centred between them
[[nodiscard]] Arena MakeArena(glm::vec2 a, glm::vec2 b, float sizeA, float sizeB);
/// Where a creature takes its place: the one that made the arena to the east of the middle, the other to the west
[[nodiscard]] glm::vec2 ArenaSpot(const Arena& arena, float size, bool madeIt);
[[nodiscard]] float ArrivalDistance(float size);
/// Whether a point is within the range moves are made in
[[nodiscard]] bool WithinRange(const Arena& arena, glm::vec2 point);
/// Fighters keep at least half their own radius and all of their opponent's apart
[[nodiscard]] float ClosestApart(float radius, float opponentRadius);
/// Where a fighter moving from one point to another ends up without coming closer to its opponent than a distance:
/// moving in that far, it stops on the circle round the opponent
[[nodiscard]] glm::vec2 KeepApart(glm::vec2 from, glm::vec2 to, glm::vec2 opponent, float closest);
/// Fighters only make moves facing their opponents to within this many radians, turning to them first
constexpr float k_FacingTolerance = 0.35f;
/// A point pulled back within the range, as a block's recoil never pushes a creature past it
[[nodiscard]] glm::vec2 ClampToRange(const Arena& arena, glm::vec2 point);

// Bands of the body

/// High, middle and low on the body, by shares of its height
enum class Band : uint8_t
{
	High,
	Mid,
	Low,
};
constexpr float k_LowBelow = 0.33f;
constexpr float k_HighAbove = 0.66f;
/// The band a height up the body falls in, as a share of the body's height
[[nodiscard]] Band BandOf(float shareOfHeight);

// Moves

enum class Step : uint8_t
{
	Forward,
	Back,
	Right,
	Left,
};
[[nodiscard]] size_t StepAnimation(Step step);
/// The step towards a point, given in the creature's frame (x to its right, y ahead): whichever axis is longer picks
/// stepping forward or back, or right or left
[[nodiscard]] Step StepTowards(glm::vec2 local);
/// Whether a creature at a point may take a step: forward always, back or sideways only within range
[[nodiscard]] bool CanStep(const Arena& arena, glm::vec2 position, Step step);

struct Move
{
	enum class Kind : uint8_t
	{
		/// Blows at a band of the opponent's body
		High,
		Mid,
		Low,
		Block,
		Special,
		/// Playing an animation, as stepping is
		Animation,
		Spell,
	};
	Kind kind {Kind::Mid};
	/// The animation played, or the spell cast
	uint32_t value {0};

	bool operator==(const Move&) const = default;
};
[[nodiscard]] Move AttackMove(Band band);
[[nodiscard]] Move StepMove(Step step);
[[nodiscard]] Move BlockMove();
[[nodiscard]] bool IsBlow(Move::Kind kind);

/// A blow is charged for as long as the click is held, up to 1.2 seconds
constexpr float k_MaxChargeMs = 1200.0f;
/// What a move starts charged at: a blow waits to be let go (none), blocks and animations need no charge, and special
/// moves and spells are fully charged
[[nodiscard]] std::optional<float> InitialCharge(Move::Kind kind);
[[nodiscard]] float ReleasedCharge(float heldMs);
/// How fast a blow plays by its charge, 0.5 to 1.5
[[nodiscard]] float BlowSpeed(float chargeMs);
/// The fight's computer opponent hits at this speed
constexpr float k_AiBlowSpeed = 0.5f;

struct QueuedMove
{
	Move move {};
	std::optional<float> chargeMs;
};

/// The moves a creature has been told to make, in turn, at most twelve
class MoveQueue
{
public:
	static constexpr size_t k_Capacity = 12;

	/// A move added at the back, charged as its kind starts, or in place of every other at the front. False when full.
	bool Push(const Move& move, bool replace);
	[[nodiscard]] std::optional<QueuedMove> Front() const;
	void Pop();
	void Clear();
	/// The first blow still waiting for its charge gets it. False when none waits.
	bool Release(float heldMs);
	/// Getting hit takes away a charged blow still waiting
	void CancelWaiting();
	[[nodiscard]] bool HasWaiting() const;
	[[nodiscard]] size_t Size() const { return _count; }
	[[nodiscard]] bool Empty() const { return _count == 0; }
	[[nodiscard]] std::span<const QueuedMove> Moves() const { return {_moves.data(), _count}; }

private:
	std::array<QueuedMove, k_Capacity> _moves {};
	size_t _count {0};
};

// Learning to fight

/// How the creature leans in fights, from -1 (defensive) to 1 (aggressive). Each move the player makes for it nudges
/// it: a block towards defence, anything else towards attack.
[[nodiscard]] float LearnTendency(float tendency, Move::Kind kind);
/// A creature's first fight starts it leaning against its alignment: evil creatures attack, good ones defend
[[nodiscard]] float FirstTendency(float alignment);

// Health, stamina and fainting

[[nodiscard]] float FightHealthAtStart(float life);
[[nodiscard]] float StaminaAtStart(float energy, float exhaustion);
/// Stamina comes back a little every game turn, up to 1. Nothing is known to use it up.
constexpr float k_StaminaPerTurn = 1.0f / 600.0f;
[[nodiscard]] float RegainStamina(float stamina);
/// A quarter of the fight health lost comes off the real life at the end
constexpr float k_LifeLostShare = 0.25f;
[[nodiscard]] float LifeAfterFight(float life, float fightHealth);
/// A creature this healthy or less won't start a fight
constexpr float k_MinLifeToFight = 0.1f;
[[nodiscard]] bool HealthyEnoughToFight(float life);
/// Knocked out, a creature lies 8 seconds and 4 more for each of its size
[[nodiscard]] float FaintSeconds(float size);
/// Taken home, it fades out and in this long, then waits this long before resting
constexpr float k_FizzSeconds = 2.0f;
constexpr float k_WaitAfterHomeSeconds = 3.0f;
/// It rests until at least this healthy and no more exhausted than this, then gets up
constexpr float k_GetUpLife = 0.4f;
constexpr float k_RestedExhaustion = 0.3f;
[[nodiscard]] bool NeedsRest(float life, float exhaustion);

// Blows

constexpr float k_HighDamage = 0.05f;
constexpr float k_LowDamage = 0.03f;
/// A blocked blow does a tenth of the damage
constexpr float k_BlockedShare = 0.1f;
constexpr float k_MinDamageFactor = 0.04f;
constexpr float k_MaxDamageFactor = 2.0f;

/// One blow landing
struct Blow
{
	float attackerSize {1.0f};
	float defenderSize {1.0f};
	/// From 0 (weak) to 1 (strong)
	float attackerStrength {0.5f};
	float defenderStrength {0.5f};
	/// How fast the blow played, by its charge
	float speed {1.0f};
	bool special {false};
	/// Where it landed, as a share of the defender's height
	float heightShare {0.5f};
	bool blocked {false};
};
/// How hard a blow lands: the attacker's size and strength against the defender's, times its speed, kept from 0.04
/// to 2, and twice that for a special move
[[nodiscard]] float DamageFactor(const Blow& blow);

enum class RecoilDirection : uint8_t
{
	Plain,
	Right,
	Left,
	Top,
	Bottom,
};
struct Hit
{
	Band band;
	float damage;
	/// The reeling animation played by a body that isn't blocking, and its plain version for a species without the
	/// directed one
	size_t recoil;
	size_t plainRecoil;
};
[[nodiscard]] Hit ResolveBlow(const Blow& blow, RecoilDirection direction);
/// A blow this near the middle of the body, in shares of its half width and half height, throws it straight back
constexpr float k_PlainRecoilWithin = 0.25f;
/// Which way a blow throws the body, by where it lands from the middle of the victim's body: x to its right as a share
/// of its half width, y up as a share of its half height. Off to a side it reels right or left, high or low it reels
/// by its top or bottom version; which of those the game means isn't known, so high is taken as top.
[[nodiscard]] RecoilDirection RecoilDirectionOf(glm::vec2 offset);
/// The wobble played on top of the body after a blow: high or low, side to side or front to back
[[nodiscard]] size_t WobbleAnimation(Band band, bool sideways);
/// Which step an animation is, if it is one
[[nodiscard]] std::optional<Step> StepOf(size_t animation);
/// The kind of wound a blow leaves: half the time the first kind, else by the blow's own wound type (0, 1 or 2)
[[nodiscard]] uint8_t WoundKind(uint8_t blowWoundType, const Random& random);
/// The wound type each blow leaves; the game keeps one for each, which isn't found, so this guesses it by where the
/// blow lands: blows at the head cut, those to the body bruise, low blows graze
[[nodiscard]] uint8_t BlowWoundType(size_t animation);
/// Wounds of these kinds also bleed
[[nodiscard]] bool Bleeds(uint8_t woundKind);

// Playing the fight

/// The fight animations play at this speed, steps at 0.9 of it and everything else at 0.6, and an action's own speed
/// on top
constexpr float k_FightSpeed = 1.1f;
constexpr float k_StepSpeedShare = 0.9f;
constexpr float k_OtherSpeedShare = 0.6f;
[[nodiscard]] float PlaybackSpeed(size_t animation, float actionSpeed);

enum class State : uint8_t
{
	/// Not fighting
	Idle,
	Start,
	Stance,
	/// A blow, a step or reeling from a blow
	Action,
	Finish,
	BlockStart,
	Block,
	BlockRecoil,
	BlockEnd,
	Faint,
	GetUp,
	CastStart,
	Cast,
	CastEnd,
	/// Lying where it fainted
	Lying,
};
[[nodiscard]] std::string_view Name(State state);
/// The animation a state plays, other than an action's own
[[nodiscard]] size_t AnimationOf(State state);
/// Whether a state's animation plays round and round until something else happens, or holds its last frame
[[nodiscard]] bool Loops(State state);
/// What comes after a state's animation has played through
[[nodiscard]] State AfterAnimation(State state, bool hasOpponent);
/// Whether the creature is blocking: while blocking, once half way into starting to block, and until half way out of
/// it again
[[nodiscard]] bool IsBlocking(State state, float timeMs, float startBlockDurationMs);
/// Whether a creature in a state can be told to make a move
[[nodiscard]] bool TakesMoves(State state);
/// Whether a creature in a state turns to face its opponent
[[nodiscard]] bool FacesOpponent(State state);

/// Who chooses the creature's moves
enum class Control : uint8_t
{
	Player,
	Computer,
};
/// The computer leaves a player's creature alone for 3 seconds as a fight starts, and for 15 after each of the
/// player's moves
constexpr float k_ComputerWaitsAtStartMs = 3000.0f;
constexpr float k_ComputerWaitsMs = 15000.0f;

/// A creature's side of a fight
struct Fighter
{
	State state {State::Idle};
	/// The animation playing, how far through it is and the speed of the action
	size_t animation {0};
	float timeMs {0.0f};
	float speed {1.0f};
	bool mirrored {false};
	/// A blow that is the special move, and whether it has landed yet
	bool special {false};
	bool landed {false};
	/// The spell being cast
	std::optional<uint32_t> spell;
	MoveQueue queue;
	float health {1.0f};
	float stamina {1.0f};
	Control control {Control::Computer};
	/// The computer always chooses, whatever the player does
	bool autoFight {false};
	float computerWaitMs {k_ComputerWaitsAtStartMs};
	float tendency {0.0f};
};
/// Plays a state, with an action's animation and speed
void Enter(Fighter& fighter, State state, std::optional<size_t> animation = std::nullopt, float speed = 1.0f);
/// The player adds a move: the computer gives the creature back and waits 15 seconds, and the creature learns from it
bool PlayerMove(Fighter& fighter, const Move& move, bool replace);

/// What to do with the move at the front of the queue
struct Order
{
	enum class Kind : uint8_t
	{
		EndBlock,
		Animation,
		Cast,
		Block,
		/// A blow at a band, or the special move
		Blow,
		Special,
	};
	Kind kind {Kind::Blow};
	Band band {Band::Mid};
	uint32_t value {0};
	float speed {1.0f};
};
/// The move at the front of the queue taken off it, if one can be made now: only in range of the opponent, in the
/// stance or blocking (when anything ends the block), and a blow only once its charge is known
[[nodiscard]] std::optional<Order> TakeOrder(Fighter& fighter, bool inRange);

// The computer's choices

/// What the opponent is doing, as the computer sees it
enum class Opponent : uint8_t
{
	Stance,
	EndingCast,
	BlockRecoil,
	Attacking,
	Casting,
	SteppingForward,
	OtherAction,
	Blocking,
	Other,
};
[[nodiscard]] Opponent OpponentOf(State state, size_t animation);
/// How aggressive the computer is, 0 to 2, by the tendency. The game meant 1 for the middle, but only ever gives 0 below
/// 0.33 and 2 from it, which is kept.
[[nodiscard]] uint32_t TierOf(float tendency);
struct Choice
{
	enum class Kind : uint8_t
	{
		None,
		Blow,
		Block,
		Step,
	};
	Kind kind {Kind::None};
	Band band {Band::Mid};
	Step step {Step::Back};
};
/// The computer's move this game turn, given its tier and what the opponent is doing
[[nodiscard]] Choice ChooseMove(uint32_t tier, Opponent opponent, const Random& random);
/// While blocking, the computer stops one turn in eight
[[nodiscard]] bool ComputerEndsBlock(const Random& random);

// Choosing the blow

/// One of the creature's blows as it lands: how far ahead of the creature it reaches and how high, in the world
struct Reach
{
	size_t animation;
	float reach;
	float height;
};
/// The blow to strike, or the step to take first
struct AttackChoice
{
	std::optional<size_t> animation;
	std::optional<Step> step;
};
/// Steps are tried this many each way
constexpr int k_MaxSteps = 10;
/// A blow lands well when it reaches this far into the opponent, at most, for each of the attacker's size
constexpr float k_DepthPerSize = 7.5f;
/// The blow at a band that lands from the fewest steps forward or back, of those that reach high enough. A blow lands
/// when it reaches into the opponent, but not by more than 7.5 units of the attacker's size; blows at the band asked for
/// beat all others, then the fewest steps, then the deepest. If the best needs steps, the creature steps once instead.
/// gap is from the attacker to the opponent's near side.
[[nodiscard]] AttackChoice ChooseAttack(std::span<const Reach> reaches, Band band, float gap, float stepLength,
                                        float attackerSize, float opponentHeight);

// The fight's end

/// The winner of a knock out shows off, unless evil, when it has a poo on the loser
[[nodiscard]] bool WinnerPoos(float alignment);
/// After a fight that ended otherwise, the creature with more life is happy and the other sad
[[nodiscard]] size_t Response(float life, float otherLife);

// Starting fights

/// An angry creature this angry picks a fight. The game weighs fighting against everything else through its planner;
/// this stands in for that.
constexpr float k_AngerToFight = 0.6f;
/// It picks on another creature within this many units of its size
constexpr float k_PickFightPerSize = 60.0f;
/// It waits this long after a fight before picking another
constexpr float k_SecondsBetweenFights = 120.0f;
[[nodiscard]] bool WantsToFight(float anger, float life, float distance, float size, float secondsSinceFight);

/// The camera watches a fight from the arena's radius away to a side and half of it up, looking at the middle. The game
/// always looks from the east, along the line the fighters start on; here it looks from across the line between them,
/// so both are seen side on.
[[nodiscard]] glm::vec3 CameraOrigin(const Arena& arena, float ground, glm::vec2 side);
/// The side to watch two fighters from: across the line between them
[[nodiscard]] glm::vec2 CameraSide(glm::vec2 a, glm::vec2 b);
/// The game's camera stays on the arena; here it follows the fighters once they move this share of the radius away
/// from where it looks
constexpr float k_CameraFollowShare = 0.3f;
} // namespace openblack::creature_fight
