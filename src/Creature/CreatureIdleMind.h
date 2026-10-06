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

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "Creature/CreatureDesires.h"

/// What a creature does with itself while it has nothing better to do, decided once a game turn. It works through an
/// agenda of steps: waiting while it looks about, playing an action, sitting for a while. When the agenda runs out it
/// picks the next: showing the player its strongest desire, at most once a minute; else sitting down for a while; else
/// being idle, which is waiting a second or two then a tired yawn, twice. At the start of each step it pulls a face
/// for three seconds.
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
/// One choice in this many is to sit down rather than be idle. The game chooses between its idle activities by the
/// strength of the desire to rest and what the creature has learnt; this stands in for that.
constexpr uint32_t k_SitOdds = 3;

/// What the creature is doing
enum class Activity : uint8_t
{
	None,
	BeIdle,
	Sit,
	ShowDesire,
	/// Something it was told to do
	Told,
};
[[nodiscard]] std::string_view Name(Activity activity);

struct Step
{
	enum class Kind : uint8_t
	{
		/// Waiting while looking about
		Wait,
		/// An action played once
		Action,
		/// Sitting down, sitting for a while while looking about, and getting up
		Sit,
	};
	Kind kind {Kind::Wait};
	float seconds {0.0f};
	size_t animation {0};
	/// Played with drooping, slowly blinking eyes, as if just woken up
	bool sleepyEyes {false};
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
};

/// What the mind knows this turn
struct Senses
{
	float seconds {0.0f};
	/// Whether the body plays an action, and whether it is in the loop of one, such as sitting
	bool bodyBusy {false};
	bool bodyLooping {false};
	std::optional<creature_desires::Desire> strongest;
	/// Seconds since the player last stroked or slapped it, and which
	std::optional<float> feedbackSeconds;
	bool feedbackWasStroke {false};
};

/// How the eyes should look
enum class Eyes : uint8_t
{
	Unchanged,
	/// Drooping and slowly blinking, as if just woken up
	Sleepy,
	Normal,
};

/// What the mind tells the body
struct Commands
{
	std::optional<size_t> playOnce;
	bool mirrored {false};
	bool startSit {false};
	bool endSit {false};
	/// A face to pull, or none to relax it
	std::optional<std::optional<size_t>> face;
	Eyes eyes {Eyes::Unchanged};
	/// Whether the head turns to whatever is interesting this turn
	bool lookAbout {false};
};

/// random(n) is a whole number from 0 to n - 1
using Random = std::function<uint32_t(uint32_t)>;

/// Being idle: wait a second or two looking about, then a tired yawn with sleepy eyes, twice
[[nodiscard]] std::vector<Step> BeIdle(const Random& random);
/// Sitting for ten to fourteen seconds
[[nodiscard]] Step SitDown(const Random& random);
/// A new agenda in place of the current one
void Plan(IdleMind& mind, Activity activity, std::vector<Step> agenda);
/// The next agenda when the current runs out
void ChooseNext(IdleMind& mind, const Senses& senses, const Random& random);
/// One game turn of the mind
[[nodiscard]] Commands Think(IdleMind& mind, const Senses& senses, const Random& random);
} // namespace openblack::creature_mind
