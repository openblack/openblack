/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureAudio.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <utility>

using namespace openblack;
using namespace openblack::creature_audio;
using audio::SoundAction;
using audio::SoundSize;
using audio::SoundSurface;

namespace
{
/// The events at or after fromMs and before toMs, their fractions measured from offsetMs over spanMs
void Collect(std::span<const SoundEvent> events, float fromMs, float toMs, float offsetMs, float spanMs,
             std::vector<FiredEvent>& out)
{
	for (const auto& event : events)
	{
		const auto time = static_cast<float>(event.timeMs);
		if (time >= fromMs && time < toMs)
		{
			out.push_back({.fraction = spanMs > 0.0f ? (time - fromMs + offsetMs) / spanMs : 0.0f, .event = event});
		}
	}
}

/// A size or alignment key: 1 to 3 by how far a value lies below its top, in thirds of the range
int32_t ThirdsKey(float top, float value)
{
	return std::clamp(static_cast<int32_t>(((top - value) * 1.5f) + 1.0f), 1, 3);
}
} // namespace

void creature_audio::Enqueue(std::vector<FiredEvent>& queue, const FiredEvent& fired)
{
	if (queue.size() >= k_MaxQueued)
	{
		return;
	}
	const auto at = std::ranges::upper_bound(queue, fired.fraction, {}, &FiredEvent::fraction);
	queue.insert(at, fired);
}

std::vector<FiredEvent> creature_audio::EventsPassed(std::span<const SoundEvent> events, float fromMs, float toMs,
                                                     uint32_t durationMs, bool looping)
{
	std::vector<FiredEvent> passed;
	if (durationMs == 0 || events.empty() || fromMs == toMs)
	{
		return passed;
	}
	const auto duration = static_cast<float>(durationMs);
	if (toMs > fromMs)
	{
		// One played once stops at its end
		const auto end = looping ? toMs : std::min(toMs, duration);
		Collect(events, fromMs, end, 0.0f, toMs - fromMs, passed);
	}
	else if (looping)
	{
		// Round the end and on from the start, as one stretch of time
		const auto stretch = (duration - fromMs) + toMs;
		Collect(events, fromMs, duration, 0.0f, stretch, passed);
		Collect(events, 0.0f, toMs, duration - fromMs, stretch, passed);
	}
	else
	{
		Collect(events, 0.0f, toMs, 0.0f, toMs, passed);
	}
	std::ranges::stable_sort(passed, {}, &FiredEvent::fraction);
	return passed;
}

std::vector<FiredEvent> creature_audio::LayerEvents(const std::optional<Played>& previous, const std::optional<Played>& current,
                                                    bool startsAtBeginning, const InfoOf& infoOf)
{
	if (previous.has_value() && current.has_value() && previous->animation == current->animation)
	{
		const auto info = infoOf(current->animation);
		if (!info.has_value())
		{
			return {};
		}
		return EventsPassed(info->events, previous->timeMs, current->timeMs, info->durationMs, info->looping);
	}

	std::vector<FiredEvent> fired;
	if (previous.has_value())
	{
		// The last animation played to its end, unless it loops and was left part way
		if (const auto info = infoOf(previous->animation); info.has_value() && !info->looping)
		{
			fired = EventsPassed(info->events, previous->timeMs, static_cast<float>(info->durationMs), info->durationMs, false);
		}
	}
	if (current.has_value() && startsAtBeginning)
	{
		if (const auto info = infoOf(current->animation); info.has_value())
		{
			// From the very start, so that an event at its first moment sounds
			std::vector<FiredEvent> started;
			Collect(info->events, 0.0f,
			        info->looping ? current->timeMs : std::min(current->timeMs, static_cast<float>(info->durationMs)), 0.0f,
			        current->timeMs, started);
			std::ranges::stable_sort(started, {}, &FiredEvent::fraction);
			fired.insert(fired.end(), started.begin(), started.end());
		}
	}
	return fired;
}

std::vector<FiredEvent> creature_audio::FaceEvents(const std::optional<Played>& previous, const std::optional<Played>& current,
                                                   bool& looped, const InfoOf& infoOf)
{
	if (!current.has_value())
	{
		looped = false;
		return {};
	}
	const auto info = infoOf(current->animation);
	if (!info.has_value())
	{
		return {};
	}
	if (!previous.has_value() || previous->animation != current->animation)
	{
		looped = false;
		return LayerEvents(std::nullopt, current, true, infoOf);
	}
	if (looped)
	{
		return {};
	}
	if (current->timeMs < previous->timeMs)
	{
		// Round its end for the first time: the rest of the first time through, then no more
		looped = true;
		return EventsPassed(info->events, previous->timeMs, static_cast<float>(info->durationMs), info->durationMs, false);
	}
	return EventsPassed(info->events, previous->timeMs, current->timeMs, info->durationMs, false);
}

std::vector<FiredEvent> creature_audio::FrameEvents(Layers& last, const Layers& current, float elapsedMs, const InfoOf& infoOf)
{
	std::vector<FiredEvent> queue;
	if (elapsedMs <= 0.0f)
	{
		return queue;
	}
	const auto enqueue = [&queue](const std::vector<FiredEvent>& fired) {
		for (const auto& event : fired)
		{
			Enqueue(queue, event);
		}
	};
	enqueue(LayerEvents(last.body, current.body, true, infoOf));
	if (current.soundingSlot.has_value() && *current.soundingSlot < current.slots.size())
	{
		// The heaviest slot sounds from where it was last frame; one just brought in sounds from the next
		const auto& slot = current.slots[*current.soundingSlot];
		const auto previous = std::ranges::find(last.slots, slot.animation, &Played::animation);
		enqueue(LayerEvents(previous != last.slots.end() ? std::optional(*previous) : std::nullopt, slot, false, infoOf));
	}
	enqueue(LayerEvents(last.gesture, current.gesture, true, infoOf));
	// A face sounds as it is pulled, not again each time the expression loops round
	bool faceLooped = last.faceLooped;
	enqueue(FaceEvents(last.face, current.face, faceLooped, infoOf));

	last = current;
	last.faceLooped = faceLooped;
	return queue;
}

std::optional<size_t> creature_audio::SoundingSlot(std::span<const float> weights)
{
	std::optional<size_t> heaviest;
	for (size_t i = 0; i < weights.size(); ++i)
	{
		if (weights[i] > 0.0f && (!heaviest.has_value() || weights[i] > weights[*heaviest]))
		{
			heaviest = i;
		}
	}
	return heaviest;
}

SoundSize creature_audio::SizeKey(float size)
{
	return static_cast<SoundSize>(ThirdsKey(2.0f, size));
}

int32_t creature_audio::AlignmentKey(float alignment)
{
	return ThirdsKey(1.0f, alignment);
}

SoundSurface creature_audio::SurfaceKey(const std::optional<Ground>& ground)
{
	if (!ground.has_value())
	{
		return SoundSurface::DeepWater;
	}
	if (ground->water)
	{
		return SoundSurface::ShallowWater;
	}
	if (ground->materialSurface < static_cast<int32_t>(SoundSurface::Grass) ||
	    ground->materialSurface > static_cast<int32_t>(SoundSurface::LooseFoliage))
	{
		return SoundSurface::Hard;
	}
	return static_cast<SoundSurface>(ground->materialSurface);
}

uint16_t creature_audio::TerrainMaterial(std::optional<uint16_t> materialType, float snowDepth)
{
	if (snowDepth >= k_SnowMaterialDepth)
	{
		return k_SnowMaterial;
	}
	// No material is deep water
	return materialType.value_or(0) != 0 ? *materialType : uint16_t {1};
}

audio::AnimEffectKeys creature_audio::Keys(float size, float alignment, int32_t soundObject, SoundSurface surface,
                                           SoundAction action)
{
	return {
	    .size = SizeKey(size),
	    .alignment = AlignmentKey(alignment),
	    .object = static_cast<audio::SoundObject>(soundObject),
	    .surface = surface,
	    .action = action,
	};
}

std::string creature_audio::VoiceBank(std::string_view rigBankName, CreatureType species)
{
	std::string name(rigBankName);
	if (name.empty())
	{
		// The bank each species' file names
		switch (species)
		{
		case CreatureType::Cow:
			name = "bcow";
			break;
		case CreatureType::Tiger:
		case CreatureType::Leopard:
		case CreatureType::Lion:
			name = "btiger";
			break;
		case CreatureType::Wolf:
			name = "bwolf";
			break;
		case CreatureType::Horse:
		case CreatureType::Zebra:
			name = "bhorse";
			break;
		case CreatureType::Tortoise:
			name = "btort";
			break;
		case CreatureType::BrownBear:
		case CreatureType::PolarBear:
			name = "bbear";
			break;
		case CreatureType::Sheep:
			name = "bsheep";
			break;
		case CreatureType::Chimp:
		case CreatureType::Mandrill:
		case CreatureType::Gorilla:
		case CreatureType::GiantApe:
			name = "bape";
			break;
		case CreatureType::Ogre:
			name = "bgreek";
			break;
		case CreatureType::Rhino:
			name = "brhino";
			break;
		default:
			return {};
		}
	}
	std::ranges::transform(name, name.begin(),
	                       [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
	return name + ".sad";
}

bool creature_audio::IsHeard(EventKind kind, const Gate& gate)
{
	if (kind == EventKind::HairGroup || gate.muted || gate.insideTemple || gate.wideScreen)
	{
		return false;
	}
	return kind != EventKind::Voice || gate.localPlayersCreature || gate.otherVoicesEnabled;
}

std::string_view creature_audio::Name(SoundAction action)
{
	constexpr std::array<std::pair<SoundAction, std::string_view>, 52> k_Names {{
	    {SoundAction::BreatheIn, "breathe in"},
	    {SoundAction::BreatheOut, "breathe out"},
	    {SoundAction::FootstepLight, "light footstep"},
	    {SoundAction::FootstepNormal, "footstep"},
	    {SoundAction::FootStamp, "foot stamp"},
	    {SoundAction::Scream, "scream"},
	    {SoundAction::RoarShort, "short roar"},
	    {SoundAction::SnoreIn, "snore in"},
	    {SoundAction::SnoreOut, "snore out"},
	    {SoundAction::SneezeIn, "sneeze in"},
	    {SoundAction::SneezeOut, "sneeze out"},
	    {SoundAction::GrowlShort, "short growl"},
	    {SoundAction::Scratch, "scratch"},
	    {SoundAction::Vomit, "vomit"},
	    {SoundAction::Pooh, "poo"},
	    {SoundAction::Sniff, "sniff"},
	    {SoundAction::Bite, "bite"},
	    {SoundAction::Chew, "chew"},
	    {SoundAction::Yawn, "yawn"},
	    {SoundAction::Grunt, "grunt"},
	    {SoundAction::Pant, "pant"},
	    {SoundAction::FacePunch, "face punch"},
	    {SoundAction::SlapFace, "face slap"},
	    {SoundAction::SlapSide, "side slap"},
	    {SoundAction::ReactToHit, "react to hit"},
	    {SoundAction::HitGround, "hit ground"},
	    {SoundAction::RewardedShort, "short reward"},
	    {SoundAction::Swipe, "swipe"},
	    {SoundAction::Acknowledge, "acknowledge"},
	    {SoundAction::Refusal, "refusal"},
	    {SoundAction::Whistle, "whistle"},
	    {SoundAction::TauntGrunt, "taunt grunt"},
	    {SoundAction::HappyGrunt, "happy grunt"},
	    {SoundAction::Scared, "scared"},
	    {SoundAction::PlayfulGrunt, "playful grunt"},
	    {SoundAction::PickMeScream, "pick me scream"},
	    {SoundAction::AngryScream, "angry scream"},
	    {SoundAction::FightGrunt, "fight grunt"},
	    {SoundAction::BodyPunch, "body punch"},
	    {SoundAction::RewardedMedium, "medium reward"},
	    {SoundAction::RewardedLong, "long reward"},
	    {SoundAction::GrowlMedium, "medium growl"},
	    {SoundAction::GrowlLong, "long growl"},
	    {SoundAction::RoarMedium, "medium roar"},
	    {SoundAction::RoarLong, "long roar"},
	    {SoundAction::Death, "death"},
	    {SoundAction::Drink, "drink"},
	    {SoundAction::DrinkFinish, "drink finish"},
	    {SoundAction::KissShort, "short kiss"},
	    {SoundAction::KissLong, "long kiss"},
	    {SoundAction::Huff, "huff"},
	    {SoundAction::Collide, "collide"},
	}};
	const auto value = static_cast<int32_t>(action);
	if (value >= static_cast<int32_t>(SoundAction::CreedGlowLeft0) &&
	    value <= static_cast<int32_t>(SoundAction::CreedGlowCentre100))
	{
		return "creed glow";
	}
	const auto found = std::ranges::find(k_Names, action, &std::pair<SoundAction, std::string_view>::first);
	return found != k_Names.end() ? found->second : std::string_view {};
}
