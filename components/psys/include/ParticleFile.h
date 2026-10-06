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
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/// The particle effect files of the miracles and spot visuals (Data/Spells/ZSpellFiles). Each is the text the effect
/// editor saved: a header of properties, then one block per class instance:
///
///     BEGINPROPERTIES
///     PROPERTY MaxSpellAge FLOAT -1
///     ENDPROPERTIES
///     BEGINCLASS DiskEmitter DiskEmitter0
///     BEGINPROPERTIES
///     PROPERTY EmissionFreq FLOAT 3
///     PROPERTY PCreator PERSIS_PNTR ParticleSpriteCreator0
///     ENDPROPERTIES
///     ENDCLASS
///
/// The game ships them compressed as <name>_txt.zzz: the length of the text as a little endian u32, then the text
/// deflated with zlib. A loose <name>.txt beside them is read in preference, which lets the effects be modded.
namespace openblack::psys
{

/// A SOUND_ACTION property: the sound's name in the game's sound action list (NO_SOUND for none) and its four switches
struct SoundActionValue
{
	std::string sound;
	bool looping {false};
	/// Read by the game and then ignored
	bool onlyOne {false};
	/// A stopped loop plays to the end of its pass rather than being cut
	bool softRelease {false};
	/// Played with the sound of the ground under the particle
	bool useSurface {false};
};

/// One PROPERTY line
struct Property
{
	enum class Type : uint8_t
	{
		Bool,
		Integer,
		Float,
		/// STRING (a path) and ENUM (a mesh or animation name)
		String,
		/// PERSIS_PNTR: the name of another class instance in the file, empty for none
		Reference,
		Array,
		SoundAction,
	};

	Type type {Type::Integer};
	int integer {0};
	float number {0.0f};
	/// String, Reference and the sound's name
	std::string text;
	/// The elements of an array as written: most are whole numbers, some (spline key points) are not
	std::vector<float> numbers;
	SoundActionValue sound;
};

/// BEGINCLASS <class> <name>: one instance of a particle class, its properties by name
struct ParticleObject
{
	std::string className;
	std::string name;
	std::map<std::string, Property, std::less<>> properties;

	[[nodiscard]] bool Has(std::string_view key) const { return properties.contains(key); }
	/// The property read as the type asked for, a float truncated to an int and an int widened to a float, or the fallback
	/// when the object has none of that name
	[[nodiscard]] bool Bool(std::string_view key, bool fallback) const;
	[[nodiscard]] int Int(std::string_view key, int fallback) const;
	[[nodiscard]] float Float(std::string_view key, float fallback) const;
	/// The text of a string or reference, empty when absent
	[[nodiscard]] std::string String(std::string_view key) const;
	/// An array's elements truncated to ints, empty when absent
	[[nodiscard]] std::vector<int> IntArray(std::string_view key) const;
	/// An array's elements as written, empty when absent
	[[nodiscard]] std::span<const float> FloatArray(std::string_view key) const;
	/// A sound action, NO_SOUND with every switch off when absent
	[[nodiscard]] SoundActionValue Sound(std::string_view key) const;
};

/// A whole particle file
struct ParticleFile
{
	/// DeleteOnCloseDown, Hierarchies (25 flags), InitiallyCreated (25 flags) and MaxSpellAge
	ParticleObject header;
	/// In file order
	std::vector<ParticleObject> objects;

	/// The first object of that name (a few files repeat names), nullptr for an empty name or none
	[[nodiscard]] const ParticleObject* Find(std::string_view objectName) const;

	/// Parses the text of a particle file; nullopt when it is not one
	[[nodiscard]] static std::optional<ParticleFile> Parse(std::string_view text);
};

/// The parts of a compressed particle file: the length of the text and the zlib stream to inflate to it
struct CompressedParticleFile
{
	uint32_t textSize;
	std::span<const uint8_t> deflated;
};
/// nullopt when the data is too short to be one
[[nodiscard]] std::optional<CompressedParticleFile> SplitCompressed(std::span<const uint8_t> data);

} // namespace openblack::psys
