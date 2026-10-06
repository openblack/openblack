/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleFile.h"

#include <charconv>
#include <cstdlib>

#include <algorithm>

using namespace openblack::psys;

namespace
{
/// The name the editor writes for an empty string or reference
constexpr std::string_view k_NullString = "NULL_STRING";

/// Splits the text into whitespace separated words
class Tokens
{
public:
	explicit Tokens(std::string_view text)
	    : _text(text)
	{
	}

	/// The next word, nullopt at the end
	std::optional<std::string_view> Next()
	{
		const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v'; };
		while (_position < _text.size() && isSpace(_text[_position]))
		{
			++_position;
		}
		if (_position >= _text.size())
		{
			return std::nullopt;
		}
		const auto start = _position;
		while (_position < _text.size() && !isSpace(_text[_position]))
		{
			++_position;
		}
		return _text.substr(start, _position - start);
	}

	/// The next word is the one expected
	bool Expect(std::string_view word)
	{
		const auto next = Next();
		return next.has_value() && *next == word;
	}

	std::optional<int> NextInt()
	{
		const auto word = Next();
		if (!word.has_value())
		{
			return std::nullopt;
		}
		int value = 0;
		const auto* end = word->data() + word->size();
		const auto [ptr, error] = std::from_chars(word->data(), end, value);
		if (error != std::errc() || ptr != end)
		{
			return std::nullopt;
		}
		return value;
	}

	/// The editor writes floats as C's %g does, such as 1e+006
	std::optional<float> NextFloat()
	{
		const auto word = Next();
		if (!word.has_value())
		{
			return std::nullopt;
		}
		const std::string copy(*word);
		char* end = nullptr;
		const float value = std::strtof(copy.c_str(), &end);
		if (end != copy.c_str() + copy.size())
		{
			return std::nullopt;
		}
		return value;
	}

	std::optional<std::string> NextText()
	{
		const auto word = Next();
		if (!word.has_value())
		{
			return std::nullopt;
		}
		return *word == k_NullString ? std::string() : std::string(*word);
	}

private:
	std::string_view _text;
	size_t _position {0};
};

/// The value of a property after its name and type
std::optional<Property> ReadValue(Tokens& tokens, std::string_view type)
{
	Property property;
	if (type == "BOOL" || type == "INTEGER")
	{
		property.type = type == "BOOL" ? Property::Type::Bool : Property::Type::Integer;
		const auto value = tokens.NextInt();
		if (!value.has_value())
		{
			return std::nullopt;
		}
		property.integer = *value;
	}
	else if (type == "FLOAT")
	{
		property.type = Property::Type::Float;
		const auto value = tokens.NextFloat();
		if (!value.has_value())
		{
			return std::nullopt;
		}
		property.number = *value;
	}
	else if (type == "STRING" || type == "ENUM" || type == "PERSIS_PNTR")
	{
		property.type = type == "PERSIS_PNTR" ? Property::Type::Reference : Property::Type::String;
		auto text = tokens.NextText();
		if (!text.has_value())
		{
			return std::nullopt;
		}
		property.text = std::move(*text);
	}
	else if (type == "ARRAY")
	{
		// ARRAY SIZE <count> <elements...>
		property.type = Property::Type::Array;
		const auto count = tokens.Expect("SIZE") ? tokens.NextInt() : std::nullopt;
		if (!count.has_value() || *count < 0)
		{
			return std::nullopt;
		}
		property.numbers.reserve(static_cast<size_t>(*count));
		for (int i = 0; i < *count; ++i)
		{
			const auto element = tokens.NextFloat();
			if (!element.has_value())
			{
				return std::nullopt;
			}
			property.numbers.push_back(*element);
		}
	}
	else if (type == "SOUND_ACTION")
	{
		// <sound> LOOPING b ONLYONE b SOFTRELEASE b USESURFACE b
		property.type = Property::Type::SoundAction;
		auto sound = tokens.NextText();
		if (!sound.has_value())
		{
			return std::nullopt;
		}
		property.sound.sound = std::move(*sound);
		property.text = property.sound.sound;
		for (auto [key, flag] :
		     {std::pair {"LOOPING", &property.sound.looping}, std::pair {"ONLYONE", &property.sound.onlyOne},
		      std::pair {"SOFTRELEASE", &property.sound.softRelease}, std::pair {"USESURFACE", &property.sound.useSurface}})
		{
			const auto value = tokens.Expect(key) ? tokens.NextInt() : std::nullopt;
			if (!value.has_value())
			{
				return std::nullopt;
			}
			*flag = *value != 0;
		}
	}
	else
	{
		return std::nullopt;
	}
	return property;
}

/// The PROPERTY lines up to and including ENDPROPERTIES, BEGINPROPERTIES already read
bool ReadProperties(Tokens& tokens, ParticleObject& object)
{
	while (const auto word = tokens.Next())
	{
		if (*word == "ENDPROPERTIES")
		{
			return true;
		}
		if (*word != "PROPERTY")
		{
			return false;
		}
		const auto name = tokens.Next();
		const auto type = tokens.Next();
		if (!name.has_value() || !type.has_value())
		{
			return false;
		}
		auto value = ReadValue(tokens, *type);
		if (!value.has_value())
		{
			return false;
		}
		object.properties.insert_or_assign(std::string(*name), std::move(*value));
	}
	return false;
}
} // namespace

bool ParticleObject::Bool(std::string_view key, bool fallback) const
{
	const auto it = properties.find(key);
	if (it == properties.end())
	{
		return fallback;
	}
	return it->second.type == Property::Type::Float ? it->second.number != 0.0f : it->second.integer != 0;
}

int ParticleObject::Int(std::string_view key, int fallback) const
{
	const auto it = properties.find(key);
	if (it == properties.end())
	{
		return fallback;
	}
	return it->second.type == Property::Type::Float ? static_cast<int>(it->second.number) : it->second.integer;
}

float ParticleObject::Float(std::string_view key, float fallback) const
{
	const auto it = properties.find(key);
	if (it == properties.end())
	{
		return fallback;
	}
	return it->second.type == Property::Type::Float ? it->second.number : static_cast<float>(it->second.integer);
}

std::string ParticleObject::String(std::string_view key) const
{
	const auto it = properties.find(key);
	return it == properties.end() ? std::string() : it->second.text;
}

std::vector<int> ParticleObject::IntArray(std::string_view key) const
{
	std::vector<int> result;
	for (const float element : FloatArray(key))
	{
		result.push_back(static_cast<int>(element));
	}
	return result;
}

std::span<const float> ParticleObject::FloatArray(std::string_view key) const
{
	const auto it = properties.find(key);
	return it == properties.end() ? std::span<const float>() : std::span<const float>(it->second.numbers);
}

SoundActionValue ParticleObject::Sound(std::string_view key) const
{
	const auto it = properties.find(key);
	if (it == properties.end() || it->second.type != Property::Type::SoundAction)
	{
		return {.sound = "NO_SOUND"};
	}
	return it->second.sound;
}

const ParticleObject* ParticleFile::Find(std::string_view objectName) const
{
	if (objectName.empty())
	{
		return nullptr;
	}
	const auto found = std::ranges::find(objects, objectName, &ParticleObject::name);
	return found != objects.end() ? &*found : nullptr;
}

std::optional<ParticleFile> ParticleFile::Parse(std::string_view text)
{
	Tokens tokens(text);
	ParticleFile file;
	if (!tokens.Expect("BEGINPROPERTIES") || !ReadProperties(tokens, file.header))
	{
		return std::nullopt;
	}
	while (const auto word = tokens.Next())
	{
		if (*word != "BEGINCLASS")
		{
			return std::nullopt;
		}
		ParticleObject object;
		const auto className = tokens.Next();
		const auto name = tokens.Next();
		if (!className.has_value() || !name.has_value() || !tokens.Expect("BEGINPROPERTIES") ||
		    !ReadProperties(tokens, object) || !tokens.Expect("ENDCLASS"))
		{
			return std::nullopt;
		}
		object.className = *className;
		object.name = *name;
		file.objects.push_back(std::move(object));
	}
	return file;
}

std::optional<CompressedParticleFile> openblack::psys::SplitCompressed(std::span<const uint8_t> data)
{
	if (data.size() <= sizeof(uint32_t))
	{
		return std::nullopt;
	}
	// Little endian, whatever the host
	const uint32_t size = static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8u) |
	                      (static_cast<uint32_t>(data[2]) << 16u) | (static_cast<uint32_t>(data[3]) << 24u);
	return CompressedParticleFile {.textSize = size, .deflated = data.subspan(sizeof(uint32_t))};
}
