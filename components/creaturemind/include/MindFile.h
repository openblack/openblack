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
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/// Creature mind files: what a creature has learnt, saved by the game as the player's creature and shared by players.
/// The file is a little-endian stream of 32-bit numbers, with a few bytes and 16-bit characters. Older versions leave out
/// or add a few fields; every field of a version is kept here so that a file read and written again is the same byte
/// for byte. Its sections, in order: the creature's species row, the name of the file it was saved as and the player
/// profile it belonged to (each lightly encrypted), the creature's name, a few counters, its 40 desires with their
/// sources, the examples its decision trees learnt from, its opinions of each action, what it has seen of actions and
/// miracles, how it sees the player, the actions and miracles it knows, its alignment, its stage of growing up, its body,
/// and from version 28 on its tattoo.
namespace openblack::creaturemind
{

enum class MindResult : uint8_t
{
	Success = 0,
	ErrCantOpen,
	/// The file ends before all its sections do
	ErrTruncated,
	/// Newer than the newest version known
	ErrUnsupportedVersion,
	/// A count is far beyond anything a mind holds, so the file isn't a mind file in this layout
	ErrBadCount,
	/// The species row is not one of the game's 17, as in the computer players' version 17 templates, which predate the
	/// row and which the game itself refuses to load
	ErrNotAMindFile,
	ErrCantWrite,
};

[[nodiscard]] std::string_view ResultToStr(MindResult result);

/// The newest version, which the game writes
constexpr uint32_t k_CurrentVersion = 33;
/// The game knows 17 species rows
constexpr uint32_t k_SpeciesRows = 17;

/// The game's light encryption of the names in the header: each byte has the next number of the C library's random
/// sequence, seeded afresh for every name, added to it
void EncryptBlock(std::span<uint8_t> bytes);
void DecryptBlock(std::span<uint8_t> bytes);

/// Something one of a desire's sources measures, and the threshold past which it drives the desire
struct MindSource
{
	float value {0.0f};
	float threshold {1.0f};
	uint32_t type {0};
};

struct MindDesire
{
	/// Whether the creature's stage of growing up had brought the desire
	int32_t activated {0};
	float value {0.0f};
	float max {1.0f};
	/// The seconds the desire takes to grow by 1 with all its drive
	float increaseSeconds {1.0f};
	/// A number only versions before 7 have, and one only versions 6 to 9 have
	std::optional<float> before7;
	std::optional<float> from6To9;
	/// From version 9 on
	std::vector<MindSource> sources;
	/// A number after the sources that versions 9 to 14 have
	std::optional<int32_t> before15;
};

/// What the creature knew of something when it learnt from it: the kind of thing, where it was, and the values of its
/// attributes in the order the kind of thing lists them
struct MindBelief
{
	uint32_t type {0};
	/// Where it was, in the map's fixed point units
	int32_t x {0};
	int32_t z {0};
	std::vector<uint32_t> attributes;
};

/// One example a decision tree learns from: something the creature did a thing to or with, and the feedback for it
struct MindEpisode
{
	int32_t kind {0};
	int32_t desire {0};
	int32_t action {0};
	MindBelief belief;
	float feedback {0.0f};
};

/// The examples of one desire's decision tree: of which thing to act on (type 0) or which thing to use (type 1)
struct MindTree
{
	int32_t type {0};
	int32_t desire {0};
	/// The desire again, which the game writes twice
	int32_t desireAgain {0};
	std::vector<MindEpisode> episodes;
};

/// How often something has been seen, and when last or first, in game turns (the turn only from version 8 on)
struct MindSighting
{
	uint32_t count {0};
	std::optional<uint32_t> turn;
};

struct MindKnownAction
{
	uint32_t id {0};
	/// A number versions before 20 keep with each
	std::optional<uint32_t> before20;
};

/// The creature's body as the mind file keeps it. Names follow what each value is known to be, where it is.
struct MindPhysique
{
	uint32_t unknown0 {0};
	/// Its age in game hours
	uint32_t age {0};
	float strength {0.0f};
	float unknown1 {0.0f};
	float unknown2 {0.0f};
	/// Before version 14
	std::optional<float> before14;
	float energy {0.0f};
	/// From version 6 on
	std::optional<float> scratch;
	uint16_t flags {0};
	/// Poo, exhaustion, dehydration and three more
	std::array<float, 6> needs {};
	/// From version 22 on: a number and the creature's size
	std::optional<int32_t> unknown3;
	std::optional<float> size;
	/// Two lists the body keeps
	std::vector<uint32_t> listA;
	std::vector<uint32_t> listB;
};

/// From version 18 on, entries of a small database kept with the mind: each a pair of numbers and pairs of values, the
/// pairs numbered from 1 and only the non-zero ones kept
struct MindDatabaseEntry
{
	uint32_t id {0};
	uint32_t unknown {0};
	int32_t size {0};
	struct Pair
	{
		int32_t number {0};
		float a {0.0f};
		float b {0.0f};
	};
	std::vector<Pair> pairs;
};

/// Three counts kept for each of 45 things the creature learns, the third from version 31 on
struct MindLearningCount
{
	uint32_t a {0};
	uint32_t b {0};
	std::optional<uint32_t> c;
};

struct MindFileData
{
	uint32_t version {k_CurrentVersion};
	uint32_t speciesRow {0};
	/// The file name it was saved as, decrypted, with its terminating zero
	std::vector<uint8_t> savedAs;
	/// The player profile's name, decrypted UTF-16 with its terminating zero
	std::vector<uint8_t> profile;
	std::u16string name;
	/// Counters of the creature (the last, from version 17 on, how far it is out of its mind)
	std::vector<uint32_t> counters;
	std::vector<MindDesire> desires;
	/// Five numbers versions 6 to 9 have after the desires
	std::optional<std::array<float, 5>> from6To9;
	/// Two trees per desire, which object to act on then which object to use
	int32_t treeDesireCount {0};
	std::vector<MindTree> trees;
	/// From version 12 on, the creature's opinion of each action, -1 to 1
	std::optional<std::vector<float>> opinions;
	/// Two numbers kept for each desire
	struct DesireMemory
	{
		float value {0.0f};
		uint32_t count {0};
	};
	std::vector<DesireMemory> desireMemories;
	/// The ordinary actions and the miracles it has seen
	std::vector<MindSighting> actionsSeen;
	std::vector<MindSighting> miraclesSeen;
	uint32_t unknown0 {0};
	float attitudeToPlayer {0.0f};
	/// From version 16 on, the desires it thinks the player has, and each kind of a village's desire
	std::optional<std::vector<float>> playerDesires;
	std::optional<std::vector<float>> townDesires;
	/// The ordinary actions and the miracles it knows
	std::array<std::vector<MindKnownAction>, 2> known;
	/// From version 23 on
	std::optional<uint32_t> unknown1;
	/// Before version 11 a list of numbers, after it the creature's alignment
	std::optional<std::vector<float>> before11;
	std::optional<float> alignment;
	uint32_t developmentTimer {0};
	int32_t developmentPhase {0};
	MindPhysique physique;
	std::optional<std::vector<MindDatabaseEntry>> database;
	/// From version 24 on
	std::optional<uint32_t> unknown2;
	/// From version 27 on
	std::optional<std::array<uint32_t, 4>> unknown3;
	/// From version 28 on: eight numbers and the tattoo's bytes
	std::optional<std::array<uint32_t, 8>> tattooHeader;
	std::optional<std::vector<uint8_t>> tattoo;
	/// From version 29 on
	std::optional<std::vector<MindLearningCount>> learningCounts;
	/// From version 30 on, the scale its body is drawn at; from version 32 on another value; from version 33 on two
	/// numbers
	std::optional<float> drawnScale;
	std::optional<float> unknown4;
	std::optional<std::array<int32_t, 2>> unknown5;
	/// Anything after the last section, kept so the file writes back the same
	std::vector<uint8_t> trailing;

	/// The saved-as name and profile as text
	[[nodiscard]] std::string SavedAsText() const;
	[[nodiscard]] std::u16string ProfileText() const;
};

/// Reads a mind file into data
[[nodiscard]] MindResult Read(std::span<const uint8_t> bytes, MindFileData& data);
[[nodiscard]] MindResult ReadFile(const std::filesystem::path& path, MindFileData& data);
/// Writes the data as its version lays it out
[[nodiscard]] std::vector<uint8_t> Write(const MindFileData& data);
[[nodiscard]] MindResult WriteFile(const std::filesystem::path& path, const MindFileData& data);

} // namespace openblack::creaturemind
