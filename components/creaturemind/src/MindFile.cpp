/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MindFile.h"

#include <cstring>

#include <fstream>
#include <iterator>
#include <type_traits>

using namespace openblack::creaturemind;

namespace
{
/// The seed of the names' encryption
constexpr uint32_t k_CipherSeed = 0x913;
/// No count in a mind comes near this; beyond it the file is not laid out as a mind file
constexpr uint32_t k_MaxCount = 1u << 20;

/// The C library's random sequence the encryption adds
class CipherStream
{
public:
	[[nodiscard]] uint8_t Next()
	{
		_state = _state * 214013u + 2531011u;
		return static_cast<uint8_t>((_state >> 16) & 0x7fff);
	}

private:
	uint32_t _state {k_CipherSeed};
};

/// Walks a mind file's layout, reading values in or writing them out, so the two ways can't drift apart
class Stream
{
public:
	explicit Stream(std::span<const uint8_t> input)
	    : _input(input)
	    , _reading(true)
	{
	}
	explicit Stream(std::vector<uint8_t>& output)
	    : _output(&output)
	    , _reading(false)
	{
	}

	[[nodiscard]] bool Reading() const { return _reading; }
	[[nodiscard]] MindResult Result() const { return _result; }
	[[nodiscard]] bool Ok() const { return _result == MindResult::Success; }
	void Fail(MindResult result)
	{
		if (_result == MindResult::Success)
		{
			_result = result;
		}
	}
	[[nodiscard]] size_t Remaining() const { return _input.size() - _position; }

	template <typename T>
	    requires std::is_arithmetic_v<T>
	void Value(T& value)
	{
		if (!Ok())
		{
			return;
		}
		if (_reading)
		{
			if (Remaining() < sizeof(T))
			{
				Fail(MindResult::ErrTruncated);
				value = {};
				return;
			}
			std::memcpy(&value, _input.data() + _position, sizeof(T));
			_position += sizeof(T);
		}
		else
		{
			const auto at = _output->size();
			_output->resize(at + sizeof(T));
			std::memcpy(_output->data() + at, &value, sizeof(T));
		}
	}

	/// A count followed by that many entries: read, the vector takes the count; written, the count is its size
	template <typename Count, typename T, typename Each>
	void Counted(std::vector<T>& values, Each&& each)
	{
		auto count = static_cast<Count>(values.size());
		Value(count);
		if (!Ok())
		{
			return;
		}
		if (_reading)
		{
			if (static_cast<uint64_t>(count) > k_MaxCount)
			{
				Fail(MindResult::ErrBadCount);
				return;
			}
			values.assign(static_cast<size_t>(count), T {});
		}
		for (auto& value : values)
		{
			each(value);
			if (!Ok())
			{
				return;
			}
		}
	}

	/// A field only some versions have
	template <typename T, typename Each>
	void Optional(bool present, std::optional<T>& value, Each&& each)
	{
		if (!present)
		{
			if (_reading)
			{
				value.reset();
			}
			return;
		}
		if (_reading)
		{
			value.emplace();
		}
		else if (!value.has_value())
		{
			// Written at a version that has it, a missing field is written as nothing set
			T empty {};
			each(empty);
			return;
		}
		each(*value);
	}

	template <typename T>
	void Optional(bool present, std::optional<T>& value)
	{
		Optional(present, value, [this](T& v) { Value(v); });
	}

	void Rest(std::vector<uint8_t>& bytes)
	{
		if (_reading)
		{
			bytes.assign(_input.begin() + static_cast<std::ptrdiff_t>(_position), _input.end());
			_position = _input.size();
		}
		else
		{
			_output->insert(_output->end(), bytes.begin(), bytes.end());
		}
	}

private:
	std::span<const uint8_t> _input;
	std::vector<uint8_t>* _output {nullptr};
	size_t _position {0};
	bool _reading;
	MindResult _result {MindResult::Success};
};

/// A name kept as a byte count and its bytes, encrypted in the file and plain in the data
void EncryptedName(Stream& stream, std::vector<uint8_t>& plain)
{
	stream.Counted<uint8_t>(plain, [&stream](uint8_t& byte) { stream.Value(byte); });
	if (!stream.Ok())
	{
		return;
	}
	if (stream.Reading())
	{
		DecryptBlock(plain);
	}
}

void Belief(Stream& stream, MindBelief& belief)
{
	stream.Value(belief.type);
	stream.Value(belief.x);
	stream.Value(belief.z);
	stream.Counted<uint32_t>(belief.attributes, [&stream](uint32_t& value) { stream.Value(value); });
}

void Desire(Stream& stream, MindDesire& desire, uint32_t version)
{
	stream.Value(desire.activated);
	stream.Value(desire.value);
	stream.Value(desire.max);
	stream.Value(desire.increaseSeconds);
	stream.Optional(version < 7, desire.before7);
	stream.Optional(version > 5 && version < 10, desire.from6To9);
	if (version > 8)
	{
		stream.Counted<uint32_t>(desire.sources, [&stream](MindSource& source) {
			stream.Value(source.value);
			stream.Value(source.threshold);
			stream.Value(source.type);
		});
	}
	stream.Optional(version > 8 && version < 15, desire.before15);
}

void Sightings(Stream& stream, std::vector<MindSighting>& sightings, uint32_t version)
{
	stream.Counted<uint32_t>(sightings, [&stream, version](MindSighting& sighting) {
		stream.Value(sighting.count);
		stream.Optional(version > 7, sighting.turn);
	});
}

void Physique(Stream& stream, MindPhysique& body, uint32_t version)
{
	stream.Value(body.unknown0);
	stream.Value(body.age);
	stream.Value(body.strength);
	stream.Value(body.unknown1);
	stream.Value(body.unknown2);
	stream.Optional(version < 14, body.before14);
	stream.Value(body.energy);
	stream.Optional(version > 5, body.scratch);
	stream.Value(body.flags);
	for (auto& need : body.needs)
	{
		stream.Value(need);
	}
	stream.Optional(version > 21, body.unknown3);
	stream.Optional(version > 21, body.size);
	stream.Counted<uint32_t>(body.listA, [&stream](uint32_t& value) { stream.Value(value); });
	stream.Counted<uint32_t>(body.listB, [&stream](uint32_t& value) { stream.Value(value); });
}

/// The database is a list ended by an id of 0, each entry's pairs a list ended by a number of 0
void Database(Stream& stream, std::vector<MindDatabaseEntry>& entries)
{
	if (stream.Reading())
	{
		entries.clear();
		while (stream.Ok())
		{
			uint32_t id = 0;
			stream.Value(id);
			if (id == 0 || !stream.Ok())
			{
				return;
			}
			auto& entry = entries.emplace_back();
			entry.id = id;
			stream.Value(entry.unknown);
			stream.Value(entry.size);
			while (stream.Ok())
			{
				int32_t number = 0;
				stream.Value(number);
				if (number == 0)
				{
					break;
				}
				auto& pair = entry.pairs.emplace_back();
				pair.number = number;
				stream.Value(pair.a);
				stream.Value(pair.b);
			}
			if (entries.size() > k_MaxCount)
			{
				stream.Fail(MindResult::ErrBadCount);
			}
		}
		return;
	}
	for (auto& entry : entries)
	{
		stream.Value(entry.id);
		stream.Value(entry.unknown);
		stream.Value(entry.size);
		for (auto& pair : entry.pairs)
		{
			stream.Value(pair.number);
			stream.Value(pair.a);
			stream.Value(pair.b);
		}
		int32_t end = 0;
		stream.Value(end);
	}
	uint32_t end = 0;
	stream.Value(end);
}

void Layout(Stream& stream, MindFileData& data)
{
	stream.Value(data.version);
	stream.Value(data.speciesRow);
	if (!stream.Ok())
	{
		return;
	}
	if (data.version > k_CurrentVersion)
	{
		stream.Fail(MindResult::ErrUnsupportedVersion);
		return;
	}
	if (data.speciesRow >= k_SpeciesRows)
	{
		stream.Fail(MindResult::ErrNotAMindFile);
		return;
	}
	const auto version = data.version;
	EncryptedName(stream, data.savedAs);
	EncryptedName(stream, data.profile);
	{
		std::vector<char16_t> name(data.name.begin(), data.name.end());
		stream.Counted<uint32_t>(name, [&stream](char16_t& c) {
			auto value = static_cast<uint16_t>(c);
			stream.Value(value);
			c = static_cast<char16_t>(value);
		});
		data.name.assign(name.begin(), name.end());
	}
	// The counters aren't counted: there are six, and a seventh from version 17 on
	const size_t counters = version > 16 ? 7 : 6;
	if (stream.Reading())
	{
		data.counters.assign(counters, 0);
	}
	data.counters.resize(counters);
	for (auto& counter : data.counters)
	{
		stream.Value(counter);
	}
	stream.Counted<uint32_t>(data.desires, [&stream, version](MindDesire& desire) { Desire(stream, desire, version); });
	stream.Optional(version > 5 && version < 10, data.from6To9, [&stream](std::array<float, 5>& values) {
		for (auto& value : values)
		{
			stream.Value(value);
		}
	});

	// The trees are counted by desire, two to each
	if (stream.Reading())
	{
		stream.Value(data.treeDesireCount);
		if (data.treeDesireCount < 0 || static_cast<uint32_t>(data.treeDesireCount) > k_MaxCount)
		{
			stream.Fail(MindResult::ErrBadCount);
			return;
		}
		data.trees.assign(static_cast<size_t>(data.treeDesireCount) * 2, MindTree {});
	}
	else
	{
		auto count = static_cast<int32_t>(data.trees.size() / 2);
		stream.Value(count);
	}
	for (auto& tree : data.trees)
	{
		stream.Value(tree.type);
		stream.Value(tree.desire);
		stream.Value(tree.desireAgain);
		stream.Counted<uint32_t>(tree.episodes, [&stream](MindEpisode& episode) {
			stream.Value(episode.kind);
			stream.Value(episode.desire);
			stream.Value(episode.action);
			Belief(stream, episode.belief);
			stream.Value(episode.feedback);
		});
		if (!stream.Ok())
		{
			return;
		}
	}
	stream.Optional(version > 11, data.opinions, [&stream](std::vector<float>& opinions) {
		stream.Counted<uint32_t>(opinions, [&stream](float& value) { stream.Value(value); });
	});

	// A pair of numbers for each desire, as many as there are desires
	if (stream.Reading())
	{
		data.desireMemories.assign(data.desires.size(), {});
	}
	data.desireMemories.resize(data.desires.size());
	for (auto& memory : data.desireMemories)
	{
		stream.Value(memory.value);
		stream.Value(memory.count);
	}
	Sightings(stream, data.actionsSeen, version);
	Sightings(stream, data.miraclesSeen, version);
	stream.Value(data.unknown0);
	stream.Value(data.attitudeToPlayer);
	const auto fixedFloats = [&stream](size_t count) {
		return [&stream, count](std::vector<float>& values) {
			values.resize(count);
			for (auto& value : values)
			{
				stream.Value(value);
			}
		};
	};
	stream.Optional(version > 15, data.playerDesires, fixedFloats(40));
	stream.Optional(version > 15, data.townDesires, fixedFloats(17));
	for (auto& known : data.known)
	{
		stream.Counted<uint32_t>(known, [&stream, version](MindKnownAction& action) {
			stream.Value(action.id);
			stream.Optional(version < 20, action.before20);
		});
	}
	stream.Optional(version > 22, data.unknown1);
	stream.Optional(version < 11, data.before11, [&stream](std::vector<float>& values) {
		stream.Counted<uint32_t>(values, [&stream](float& value) { stream.Value(value); });
	});
	stream.Optional(version >= 11, data.alignment);
	stream.Value(data.developmentTimer);
	stream.Value(data.developmentPhase);
	Physique(stream, data.physique, version);
	stream.Optional(version > 17, data.database,
	                [&stream](std::vector<MindDatabaseEntry>& entries) { Database(stream, entries); });
	stream.Optional(version > 23, data.unknown2);
	stream.Optional(version > 26, data.unknown3, [&stream](std::array<uint32_t, 4>& values) {
		for (auto& value : values)
		{
			stream.Value(value);
		}
	});
	stream.Optional(version > 27, data.tattooHeader, [&stream](std::array<uint32_t, 8>& values) {
		for (auto& value : values)
		{
			stream.Value(value);
		}
	});
	stream.Optional(version > 27, data.tattoo, [&stream](std::vector<uint8_t>& bytes) {
		stream.Counted<uint32_t>(bytes, [&stream](uint8_t& byte) { stream.Value(byte); });
	});
	stream.Optional(version > 28, data.learningCounts, [&stream, version](std::vector<MindLearningCount>& counts) {
		counts.resize(45);
		for (auto& count : counts)
		{
			stream.Value(count.a);
			stream.Value(count.b);
			stream.Optional(version > 30, count.c);
		}
	});
	stream.Optional(version > 29, data.drawnScale);
	stream.Optional(version > 31, data.unknown4);
	stream.Optional(version > 32, data.unknown5, [&stream](std::array<int32_t, 2>& values) {
		stream.Value(values[0]);
		stream.Value(values[1]);
	});
	if (stream.Ok())
	{
		stream.Rest(data.trailing);
	}
}
} // namespace

std::string_view openblack::creaturemind::ResultToStr(MindResult result)
{
	switch (result)
	{
	case MindResult::Success:
		return "Success";
	case MindResult::ErrCantOpen:
		return "Can't open the file";
	case MindResult::ErrTruncated:
		return "The file ends early";
	case MindResult::ErrUnsupportedVersion:
		return "Newer version than known";
	case MindResult::ErrBadCount:
		return "A count is out of range";
	case MindResult::ErrNotAMindFile:
		return "Not a mind file the game can load";
	case MindResult::ErrCantWrite:
		return "Can't write the file";
	}
	return "Unknown";
}

void openblack::creaturemind::EncryptBlock(std::span<uint8_t> bytes)
{
	CipherStream cipher;
	for (auto& byte : bytes)
	{
		byte = static_cast<uint8_t>(byte + cipher.Next());
	}
}

void openblack::creaturemind::DecryptBlock(std::span<uint8_t> bytes)
{
	CipherStream cipher;
	for (auto& byte : bytes)
	{
		byte = static_cast<uint8_t>(byte - cipher.Next());
	}
}

std::string MindFileData::SavedAsText() const
{
	std::string text(savedAs.begin(), savedAs.end());
	text.resize(strnlen(text.c_str(), text.size()));
	return text;
}

std::u16string MindFileData::ProfileText() const
{
	std::u16string text;
	for (size_t i = 0; i + 1 < profile.size(); i += 2)
	{
		const auto c = static_cast<char16_t>(profile[i] | (profile[i + 1] << 8));
		if (c == 0)
		{
			break;
		}
		text.push_back(c);
	}
	return text;
}

MindResult openblack::creaturemind::Read(std::span<const uint8_t> bytes, MindFileData& data)
{
	data = {};
	Stream stream(bytes);
	Layout(stream, data);
	return stream.Result();
}

MindResult openblack::creaturemind::ReadFile(const std::filesystem::path& path, MindFileData& data)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return MindResult::ErrCantOpen;
	}
	const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return Read(bytes, data);
}

std::vector<uint8_t> openblack::creaturemind::Write(const MindFileData& data)
{
	std::vector<uint8_t> bytes;
	auto copy = data;
	// The names go out encrypted
	EncryptBlock(copy.savedAs);
	EncryptBlock(copy.profile);
	Stream stream(bytes);
	Layout(stream, copy);
	return bytes;
}

MindResult openblack::creaturemind::WriteFile(const std::filesystem::path& path, const MindFileData& data)
{
	const auto bytes = Write(data);
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file)
	{
		return MindResult::ErrCantWrite;
	}
	file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	return file ? MindResult::Success : MindResult::ErrCantWrite;
}
