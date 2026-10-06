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
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Audio/AnimEffectKeys.h"
#include "Enums.h"

/// What a creature sounds like. Almost every sound a creature makes is placed on a moment of one of its animations: a
/// footstep as a foot lands, a roar as the mouth opens, a snore as it breathes in. When the animation plays past that
/// moment, the sound is looked up in a bank by the creature's size, its species, the ground under it and the action,
/// and played from the creature.
namespace openblack::creature_audio
{
/// Where an animation's sound comes from
enum class EventKind : uint8_t
{
	/// The species' own bank: its roars, growls and cries, which other players' creatures only make when a script
	/// lets them
	Voice = 0,
	/// Not a sound: a tuft of hair shown or hidden
	HairGroup = 1,
	/// The bank all creatures share, creature.sad: footsteps, blows, snores, eating and drinking
	Generic = 2,
};

/// Something that happens at a moment of an animation
struct SoundEvent
{
	EventKind kind;
	/// Milliseconds from the start of the animation
	int32_t timeMs;
	audio::SoundAction action;
	/// 0 plays the sound; 1 would stop it and 2 let its loop run out, which no shipped animation asks for
	int32_t mode;

	constexpr bool operator==(const SoundEvent&) const noexcept = default;
};

/// An event an animation played past this frame, and how far through the frame's stretch of the animation it lies,
/// 0 to 1
struct FiredEvent
{
	float fraction;
	SoundEvent event;
};

/// A creature queues at most this many sounds at once; later ones are dropped while it is full
constexpr size_t k_MaxQueued = 16;

/// Puts an event in a queue kept in order of fraction, after those at the same fraction, unless the queue is full
void Enqueue(std::vector<FiredEvent>& queue, const FiredEvent& fired);

/// The events an animation passes playing from fromMs to toMs, a stretch of time that a looping animation may wrap
/// round its end within: those at or after the start and before the end of the stretch. A looping animation whose
/// time went back has wrapped. An animation played once whose time went back has started again and passes those from
/// its start. Nothing is passed when the time stands still. Mirroring an animation plays it left to right but at the
/// same times, so it passes the same events.
[[nodiscard]] std::vector<FiredEvent> EventsPassed(std::span<const SoundEvent> events, float fromMs, float toMs,
                                                   uint32_t durationMs, bool looping);

/// An animation a layer of the body plays, and how far through it is
struct Played
{
	size_t animation;
	float timeMs;

	constexpr bool operator==(const Played&) const noexcept = default;
};

/// What the events need to know of an animation
struct AnimationInfo
{
	std::span<const SoundEvent> events;
	uint32_t durationMs;
	bool looping;
};
using InfoOf = std::function<std::optional<AnimationInfo>(size_t animation)>;

/// The events one layer of the body passes between the last frame and this one. Playing on, it passes those between
/// the two times. When the layer changes animation, the last one, if it plays once, passes those to its end, and the
/// new one those from where it started: its beginning when startsAtBeginning, otherwise where it is now, so that an
/// animation brought in part way, such as a run in step with a walk, passes none in its first frame.
[[nodiscard]] std::vector<FiredEvent> LayerEvents(const std::optional<Played>& previous, const std::optional<Played>& current,
                                                  bool startsAtBeginning, const InfoOf& infoOf);

/// The events the face passes: those of its first time through as an expression is pulled, from its start, and none
/// once it has looped round, until another is pulled. looped is kept from frame to frame.
[[nodiscard]] std::vector<FiredEvent> FaceEvents(const std::optional<Played>& previous, const std::optional<Played>& current,
                                                 bool& looped, const InfoOf& infoOf);

/// Of the animations played together, each by its weight, the one whose events sound: the heaviest, so that a walk
/// blending into a run steps once rather than twice. Nothing when none weighs anything.
[[nodiscard]] std::optional<size_t> SoundingSlot(std::span<const float> weights);

/// The size key: 1 (large) above a size of 4/3, 3 (small) up to 2/3, otherwise 2 (medium)
[[nodiscard]] audio::SoundSize SizeKey(float size);
/// The alignment key from -1 to 1, which no creature bank tells apart
[[nodiscard]] int32_t AlignmentKey(float alignment);

/// The ground a creature stands on, as the sounds see it
struct Ground
{
	/// The cell holds water: the sea or a lake
	bool water;
	/// The sound surface of the cell's terrain material
	int32_t materialSurface;
};
/// The surface key: deep water off the map or where there is no land, shallow water on a water cell, otherwise the
/// terrain material's surface, hard for a material without one of the game's surfaces
[[nodiscard]] audio::SoundSurface SurfaceKey(const std::optional<Ground>& ground);

/// Snow this deep or deeper turns the ground to the snow material
constexpr float k_SnowMaterialDepth = 27.0f;
/// The snow material
constexpr uint16_t k_SnowMaterial = 27;
/// The terrain material of a cell, given the second of the two materials its country blends at its altitude: snow
/// where the snow lies deep enough, otherwise that material's type, with no material counting as deep water (1)
[[nodiscard]] uint16_t TerrainMaterial(std::optional<uint16_t> materialType, float snowDepth);

/// The keys an event is looked up by, in the banks' order
[[nodiscard]] audio::AnimEffectKeys Keys(float size, float alignment, int32_t soundObject, audio::SoundSurface surface,
                                         audio::SoundAction action);

/// The bank all creatures share
constexpr std::string_view k_GenericBank = "creature.sad";
/// The species' voice bank as the sounds are loaded: the bank its .cbn file names, in lower case with its extension,
/// or the one each species' file names when it names none (a lion uses the tiger's, a zebra the horse's)
[[nodiscard]] std::string VoiceBank(std::string_view rigBankName, CreatureType species);

/// Whether an event's sound is heard: none while muted, inside the temple or with the cinema bars in; a voice only
/// from the local player's creature unless a script lets every creature be heard
struct Gate
{
	bool muted;
	bool insideTemple;
	bool wideScreen;
	bool localPlayersCreature;
	bool otherVoicesEnabled;
};
[[nodiscard]] bool IsHeard(EventKind kind, const Gate& gate);

/// The action's name, for reading, or empty for one creatures don't use
[[nodiscard]] std::string_view Name(audio::SoundAction action);
} // namespace openblack::creature_audio
