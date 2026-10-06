/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureDesires.h"

#include <cmath>

#include <algorithm>

#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::creature_desires;
namespace animations = openblack::creature_layers::animations;

namespace
{
/// The game's sigmoid, from how far a value is past its threshold, -1 to 1, in 41 steps
constexpr std::array<float, 41> k_Sigmoid {
    0.0f,        3.6e-9f,     1e-8f,        2.8e-8f,      7.78e-8f,     2.163e-7f,    6.018e-7f,    1.674e-6f,   4.6568e-6f,
    1.29542e-5f, 3.60351e-5f, 1.002359e-4f, 2.787865e-4f, 7.751431e-4f, 0.002153319f, 0.005967208f, 0.01642494f, 0.04439174f,
    0.1144373f,  0.2644244f,  0.5f,         0.7355757f,   0.8855627f,   0.9556082f,   0.9835750f,   0.9940328f,  0.9978467f,
    0.9992248f,  0.9997212f,  0.9998997f,   0.9999639f,   0.9999871f,   0.9999954f,   0.9999983f,   0.9999994f,  0.9999998f,
    0.9999999f,  1.0f,        1.0f,         1.0f,         1.0f,
};
constexpr float k_SigmoidSteps = 20.5f;

constexpr std::array<std::string_view, k_DesireCount> k_Names {
    "Impress",
    "Compassion",
    "Anger",
    "Play",
    "Hunger",
    "Fear",
    "Curiosity",
    "Poo",
    "Tiredness",
    "Idle with player",
    "Wanderlust",
    "Puke",
    "Build home",
    "Bring home",
    "Water",
    "Restore health",
    "Be friends",
    "Attract attention",
    "Manifest state",
    "Get warmer",
    "Get colder",
    "Scratch",
    "Run from player",
    "Rest",
    "Obey player",
    "Illness",
    "Obey creature",
    "Sadness",
    "Stay near home",
    "Tell player",
    "Play with player",
    "Tell creature",
    "Educate friend",
    "Follow player",
    "Get high",
    "Hang around home",
    "Mental illness",
    "Miss friend",
    "Look around",
    "Steal",
};

/// The action that shows each desire the creature can show
struct Emote
{
	Desire desire;
	size_t animation;
};
constexpr std::array<Emote, 20> k_Emotes {{
    {Desire::Impress, animations::k_Summon},
    {Desire::Compassion, animations::k_FeelingNice},
    {Desire::Anger, animations::k_Taunt},
    {Desire::Play, animations::k_FeelPlayful},
    {Desire::Hunger, animations::k_Hungry},
    {Desire::Fear, animations::k_Frightened},
    {Desire::Curiosity, animations::k_Confused},
    {Desire::Poo, animations::k_NeedAPoo},
    {Desire::Tiredness, animations::k_Tired},
    {Desire::IdleWithPlayer, animations::k_FeelPlayful},
    {Desire::Water, animations::k_Hot},
    {Desire::BeFriends, animations::k_Happy},
    {Desire::AttractAttention, animations::k_FriendlyWave},
    {Desire::GetWarmer, animations::k_Cold},
    {Desire::GetColder, animations::k_Hot},
    {Desire::Rest, animations::k_Tired},
    {Desire::Sadness, animations::k_Sad},
    {Desire::PlayWithPlayer, animations::k_FeelPlayful},
    {Desire::MissFriend, animations::k_Sad},
    {Desire::Steal, animations::k_FeelPlayful},
}};
} // namespace

std::string_view creature_desires::Name(Desire desire)
{
	const auto index = static_cast<size_t>(desire);
	return index < k_Names.size() ? k_Names.at(index) : "Unknown";
}

float creature_desires::Sigmoid(float threshold, float value)
{
	if (value <= 0.0f || threshold == 1.0f)
	{
		return 0.0f;
	}
	return SigmoidStep(threshold, value);
}

float creature_desires::SigmoidStep(float threshold, float value)
{
	const auto past = std::clamp(std::clamp(value, -1.0f, 1.0f) - threshold, -1.0f, 1.0f);
	const auto step = std::min(static_cast<size_t>((past + 1.0f) * k_SigmoidSteps), k_Sigmoid.size() - 1);
	return k_Sigmoid.at(step);
}

Desires creature_desires::Create(const std::array<DesireSetup, k_DesireCount>& setup,
                                 const std::function<float(float, float)>& uniform)
{
	Desires desires;
	for (size_t i = 0; i < k_DesireCount; ++i)
	{
		const auto& from = setup.at(i);
		auto& desire = desires.desires.at(i);
		desire.max = from.max;
		desire.decay = uniform(from.decayMin, from.decayMax);
		desire.increaseSeconds = from.increaseSeconds;
		desire.weight = from.weight;
		for (const auto& source : from.sources)
		{
			if (source.type < k_NoSource && desire.sources.size() < k_MaxSources)
			{
				desire.sources.push_back(source);
			}
		}
	}
	return desires;
}

void creature_desires::ActivateForPhase(Desires& desires, std::span<const PhaseDesires> phases, size_t phase)
{
	for (auto& desire : desires.desires)
	{
		desire.suppressedTurns = 0;
		desire.activated = false;
	}
	for (size_t i = 0; i <= phase && i < phases.size(); ++i)
	{
		for (const auto desire : phases[i].add)
		{
			desires[desire].activated = true;
		}
		for (const auto desire : phases[i].remove)
		{
			desires[desire].activated = false;
		}
	}
}

void creature_desires::UpdateSources(Desires& desires, const SourceReader& read)
{
	for (auto& desire : desires.desires)
	{
		for (auto& source : desire.sources)
		{
			if (const auto value = read ? read(source.type, desires) : std::nullopt)
			{
				source.value = *value;
			}
			source.value *= source.multiplier;
		}
	}
}

void creature_desires::UpdateDesires(Desires& desires, float turnsPerSecond)
{
	desires.sum = 0.0f;
	for (auto& desire : desires.desires)
	{
		if (!desire.activated)
		{
			continue;
		}
		float drive = 0.0f;
		for (auto& source : desire.sources)
		{
			const auto sourceDrive = Sigmoid(source.threshold, source.value);
			source.drive += sourceDrive;
			drive += sourceDrive;
		}
		const auto increase =
		    desire.increaseSeconds > 0.0f && turnsPerSecond > 0.0f ? drive / (turnsPerSecond * desire.increaseSeconds) : 0.0f;
		if (desire.suppressedTurns > 0)
		{
			--desire.suppressedTurns;
		}
		desire.value = increase > 0.0f && desire.suppressedTurns == 0 ? desire.value + increase : desire.value * desire.decay;
		desire.value = std::clamp(desire.value, 0.0f, std::max(desire.max, 0.0f));
		desires.sum += desire.value;
	}
}

void creature_desires::Suppress(Desires& desires, Desire desire, float seconds, float turnsPerSecond)
{
	auto& state = desires[desire];
	const auto turns = static_cast<uint32_t>(std::max(seconds * turnsPerSecond, 0.0f));
	state.suppressedTurns = std::max(state.suppressedTurns, turns);
}

void creature_desires::ChangeSource(Desires& desires, uint32_t type, float amount)
{
	for (auto& desire : desires.desires)
	{
		for (auto& source : desire.sources)
		{
			if (source.type == type)
			{
				source.value = std::clamp(source.value + amount, 0.0f, 1.0f);
			}
		}
	}
}

std::optional<float> creature_desires::SourceValue(const DesireState& desire, uint32_t type)
{
	const auto found = std::ranges::find(desire.sources, type, &Source::type);
	if (found == desire.sources.end())
	{
		return std::nullopt;
	}
	return found->value;
}

std::optional<size_t> creature_desires::EmoteFor(Desire desire)
{
	const auto found = std::ranges::find(k_Emotes, desire, &Emote::desire);
	if (found == k_Emotes.end())
	{
		return std::nullopt;
	}
	return found->animation;
}

std::optional<Desire> creature_desires::StrongestShowable(const Desires& desires, float minimum)
{
	std::optional<Desire> strongest;
	float strongestValue = minimum;
	for (const auto& emote : k_Emotes)
	{
		const auto& state = desires[emote.desire];
		if (state.activated && state.value > strongestValue)
		{
			strongest = emote.desire;
			strongestValue = state.value;
		}
	}
	return strongest;
}
