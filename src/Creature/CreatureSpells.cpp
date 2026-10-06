/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpells.h"

#include <cmath>

#include <algorithm>

#include "CreatureDesires.h"

using namespace openblack;
using namespace openblack::creature_spells;

namespace
{
/// The game's sound actions of a creature taking each spell, from freeze on; the unfinished spells have none
constexpr std::array<int32_t, k_SpellCount> k_SoundActions {0x76, 0x77, 0x78, 0x79, 0x7A, 0, 0, 0x7B,
                                                            0x7C, 0x7D, 0,    0,    0,    0, 0, 0x7E};

constexpr std::array<Kind, k_SpellCount> k_Kinds {
    Kind::Look, Kind::Size, Kind::Size,  Kind::Strength, Kind::Strength, Kind::Other, Kind::Other, Kind::Look,
    Kind::Mood, Kind::Mood, Kind::Other, Kind::Other,    Kind::Other,    Kind::Other, Kind::Other, Kind::Mood,
};

/// Turns up a turn count, keeping "for ever" (negative) as it is
int32_t Add(int32_t turns, int32_t more)
{
	return turns < 0 || more < 0 ? -1 : turns + more;
}

/// The spell is cut short: it holds no longer
void CutShort(Slot& slot)
{
	slot.holdTurns = 0;
	if (slot.phase == Phase::Holding)
	{
		slot.turnsLeft = 0;
	}
}

void Begin(Slot& slot, int32_t holdTurns, entt::entity miracle)
{
	slot = {.phase = Phase::Waiting, .turnsLeft = 0, .holdTurns = holdTurns, .miracle = miracle, .before = 0.0f};
}
} // namespace

std::optional<Spell> creature_spells::SpellOf(MagicType type)
{
	const auto index = static_cast<int>(type) - static_cast<int>(MagicType::CreatureSpellFreeze);
	if (index < 0 || index >= static_cast<int>(k_SpellCount))
	{
		return std::nullopt;
	}
	return static_cast<Spell>(index);
}

std::optional<Spell> creature_spells::SpellOf(CreatureReceiveSpellType type)
{
	const auto index = static_cast<size_t>(type);
	return index < k_SpellCount ? std::optional(static_cast<Spell>(index)) : std::nullopt;
}

Kind creature_spells::KindOf(Spell spell)
{
	return k_Kinds.at(static_cast<size_t>(spell));
}

Effect creature_spells::EffectOf(Spell spell)
{
	using creature_desires::Desire;
	std::optional<uint8_t> desire;
	switch (spell)
	{
	case Spell::Nice:
		desire = static_cast<uint8_t>(Desire::Compassion);
		break;
	case Spell::Nasty:
		desire = static_cast<uint8_t>(Desire::Anger);
		break;
	case Spell::Hungry:
		desire = static_cast<uint8_t>(Desire::Hunger);
		break;
	case Spell::Frightened:
		desire = static_cast<uint8_t>(Desire::Fear);
		break;
	case Spell::Tired:
		desire = static_cast<uint8_t>(Desire::Tiredness);
		break;
	case Spell::Ill:
		desire = static_cast<uint8_t>(Desire::Illness);
		break;
	case Spell::Thirsty:
		desire = static_cast<uint8_t>(Desire::Water);
		break;
	case Spell::Itchy:
		desire = static_cast<uint8_t>(Desire::Scratch);
		break;
	default:
		break;
	}
	return {.desire = desire, .soundAction = k_SoundActions.at(static_cast<size_t>(spell))};
}

bool Spells::IsKindActive(Kind kind) const
{
	for (size_t i = 0; i < k_SpellCount; ++i)
	{
		if (slots.at(i).phase != Phase::Off && KindOf(static_cast<Spell>(i)) == kind)
		{
			return true;
		}
	}
	return false;
}

int32_t creature_spells::TurnsOf(float seconds, float turnsPerSecond)
{
	return seconds > 0.0f ? static_cast<int32_t>(seconds * turnsPerSecond) : -1;
}

ReceiveResult creature_spells::Receive(Spells& spells, Spell spell, int32_t holdTurns, entt::entity miracle)
{
	auto& slot = spells[spell];
	switch (slot.phase)
	{
	case Phase::Waiting:
	case Phase::Starting:
	{
		slot.holdTurns = Add(slot.holdTurns, holdTurns);
		const auto replaced = slot.miracle;
		slot.miracle = miracle;
		return {Received::Extended, replaced};
	}
	case Phase::Holding:
	{
		slot.turnsLeft = Add(slot.turnsLeft, holdTurns);
		const auto replaced = slot.miracle;
		slot.miracle = miracle;
		return {Received::Extended, replaced};
	}
	case Phase::Finishing:
		spells.waiting.push_back({spell, holdTurns, miracle});
		return {Received::Queued, entt::null};
	case Phase::Off:
		break;
	}
	const auto kind = KindOf(spell);
	if (spells.IsKindActive(kind))
	{
		for (size_t i = 0; i < k_SpellCount; ++i)
		{
			if (KindOf(static_cast<Spell>(i)) == kind && spells.slots.at(i).phase != Phase::Off)
			{
				CutShort(spells.slots.at(i));
			}
		}
		spells.waiting.push_back({spell, holdTurns, miracle});
		return {Received::Queued, entt::null};
	}
	Begin(slot, holdTurns, miracle);
	return {Received::Started, entt::null};
}

float creature_spells::Ratio(const Slot& slot, const Timing& timing, float turnsPerSecond)
{
	switch (slot.phase)
	{
	case Phase::Starting:
	{
		const float turns = turnsPerSecond * timing.startSeconds;
		return turns > 0.0f ? std::clamp(1.0f - static_cast<float>(slot.turnsLeft) / turns, 0.0f, 1.0f) : 1.0f;
	}
	case Phase::Holding:
		return 1.0f;
	case Phase::Finishing:
	{
		const float turns = turnsPerSecond * timing.finishSeconds;
		return turns > 0.0f ? std::clamp(static_cast<float>(slot.turnsLeft) / turns, 0.0f, 1.0f) : 0.0f;
	}
	case Phase::Off:
	case Phase::Waiting:
		break;
	}
	return 0.0f;
}

TurnResult creature_spells::Step(Spells& spells, std::span<const Timing, k_SpellCount> timings, float turnsPerSecond)
{
	TurnResult result;
	bool anyEnded = false;
	for (size_t i = 0; i < k_SpellCount; ++i)
	{
		const auto spell = static_cast<Spell>(i);
		auto& slot = spells.slots.at(i);
		const auto& timing = timings[i];
		if (slot.phase == Phase::Off)
		{
			continue;
		}
		if (slot.turnsLeft != 0)
		{
			// Holding for ever counts down from nothing
			if (slot.turnsLeft > 0)
			{
				--slot.turnsLeft;
			}
			if (slot.phase == Phase::Starting || slot.phase == Phase::Finishing)
			{
				result.events.push_back({spell, Event::Ease, Ratio(slot, timing, turnsPerSecond)});
			}
			result.events.push_back({spell, Event::Hold, Ratio(slot, timing, turnsPerSecond)});
			continue;
		}
		switch (slot.phase)
		{
		case Phase::Waiting:
			slot.phase = Phase::Starting;
			slot.turnsLeft = std::max(TurnsOf(timing.startSeconds, turnsPerSecond), 0);
			result.events.push_back({spell, Event::Start, 0.0f});
			break;
		case Phase::Starting:
			slot.phase = Phase::Holding;
			slot.turnsLeft = slot.holdTurns;
			break;
		case Phase::Holding:
			slot.phase = Phase::Finishing;
			slot.turnsLeft = std::max(TurnsOf(timing.finishSeconds, turnsPerSecond), 0);
			result.events.push_back({spell, Event::BeginFinish, 1.0f});
			break;
		case Phase::Finishing:
			slot.phase = Phase::Off;
			result.events.push_back({spell, Event::Finish, 0.0f});
			if (slot.miracle != entt::null)
			{
				result.ended.push_back(slot.miracle);
			}
			slot.miracle = entt::null;
			anyEnded = true;
			break;
		case Phase::Off:
			break;
		}
	}
	// A spell that ended makes way for those of its kind waiting
	if (anyEnded)
	{
		for (auto it = spells.waiting.begin(); it != spells.waiting.end();)
		{
			if (spells.IsKindActive(KindOf(it->spell)))
			{
				++it;
				continue;
			}
			Begin(spells[it->spell], it->holdTurns, it->miracle);
			it = spells.waiting.erase(it);
		}
	}
	return result;
}

float creature_spells::SizeTarget(Spell spell, float before, float smallest, float largest)
{
	if (spell == Spell::Big)
	{
		return std::max(std::min(before * k_BigFactor, largest), before);
	}
	if (spell == Spell::Small)
	{
		return std::min(std::max(before * k_SmallFactor, smallest), before);
	}
	return before;
}

std::optional<float> creature_spells::Target(Spell spell)
{
	switch (spell)
	{
	case Spell::Freeze:
		return 1.0f;
	case Spell::Weak:
		return k_WeakStrength;
	case Spell::Strong:
		return k_StrongStrength;
	case Spell::Fat:
		return k_FatFatness;
	case Spell::Thin:
		return k_ThinFatness;
	case Spell::Invisible:
		return k_InvisibleFizz;
	case Spell::Nice:
		return k_NiceAlignment;
	case Spell::Nasty:
		return k_NastyAlignment;
	default:
		return std::nullopt;
	}
}

float creature_spells::Ease(float before, float target, float ratio)
{
	return before + (target - before) * ratio;
}

uint32_t creature_spells::FrozenTint(float freeze)
{
	freeze = std::clamp(freeze, 0.0f, 1.0f);
	uint32_t tint = 0;
	for (const int shift : {16, 8, 0})
	{
		const auto icy = static_cast<float>((k_FrozenColour >> shift) & 0xFFu);
		const auto channel = static_cast<uint32_t>(std::lround(255.0f + (icy - 255.0f) * freeze));
		tint |= channel << shift;
	}
	return tint;
}
