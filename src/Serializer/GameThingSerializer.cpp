/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "GameThingSerializer.h"

#include <cassert>

#include <FileStream.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

using namespace openblack::serializer;

template <>
constexpr GameThingType openblack::serializer::k_GameThingTypeEnum<GameThingSerializer::Footpath> = GameThingType::Footpath;

template <>
constexpr GameThingType openblack::serializer::k_GameThingTypeEnum<GameThingSerializer::FootpathLink> =
    GameThingType::FootpathLink;

template <>
constexpr GameThingType openblack::serializer::k_GameThingTypeEnum<GameThingSerializer::FootpathNode> =
    GameThingType::FootpathNode;

template <>
constexpr GameThingType openblack::serializer::k_GameThingTypeEnum<GameThingSerializer::FootpathLinkSave> =
    GameThingType::FootpathLinkSave;

GameThingSerializer::GameThingSerializer(FileStream& stream)
    : _stream(stream)
{
}

template <typename T>
T GameThingSerializer::ReadValue()
{
	auto result = _stream.ReadValue<T>();
	_checkSum += static_cast<uint32_t>(*reinterpret_cast<uint8_t*>(&result)) + sizeof(result);
	return result;
}

void GameThingSerializer::ReadChecksum()
{
	auto expectedSum = _checkSum;
	auto readSum = ReadValue<uint32_t>();
	if (expectedSum != readSum)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed checksum (expected={:08X}, read={:08X}) at {:08X}", expectedSum,
		                    readSum, _stream.Position());
		assert(false);
	}
}

std::shared_ptr<GameThingSerializer::GameThing> GameThingSerializer::DeserializeOne(std::optional<GameThingType> requiredType)
{
	[[maybe_unused]] const auto offset = _stream.Position();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "offset={}", offset);
	auto index = ReadValue<uint32_t>();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "index={}, len={}", index, _cache.size());

	if (index == 0)
	{
		// TODO(@bwrsandman): not sure why there are these empty entries
		return nullptr;
	}

	if (index == _cache.size() + 1)
	{
		auto type = ReadValue<GameThingType>();
		if (type != requiredType.value_or(type))
		{
			throw std::runtime_error(fmt::format("Type mismatch while parsing GameThing: got {} but expected {} at 0x{:08x}",
			                                     static_cast<uint32_t>(type), static_cast<uint32_t>(*requiredType),
			                                     _stream.Position() - sizeof(GameThingType)));
		}
		[[maybe_unused]] const auto playerId = ReadValue<uint32_t>();
		ReadChecksum();
		// TODO(#479): validate checksum by adding up first byte of every read

		std::shared_ptr<GameThing> thing;

		switch (type)
		{
		case GameThingType::Footpath:
			thing = std::make_unique<Footpath>();
			break;
		case GameThingType::FootpathLink:
			thing = std::make_unique<FootpathLink>();
			break;
		case GameThingType::FootpathNode:
			thing = std::make_unique<FootpathNode>();
			break;
		case GameThingType::FootpathLinkSave:
			thing = std::make_unique<FootpathLinkSave>();
			break;
		default:
			assert(false);
		}

		_cache.push_back(thing);

		if (!thing->Deserialize(*this))
		{
			return nullptr;
		}

		// TODO(@bwrsandman): return something?

		return thing;
	}
	if (index > _cache.size())
	{
		assert(false); // weird case
		return nullptr;
	}
	// referring to a previously seen entry
	return _cache[index - 1];
}

template <typename T>
std::vector<T> GameThingSerializer::DeserializeList()
{
	auto count = ReadValue<uint32_t>();
	std::vector<T> list;
	list.reserve(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "i={}", i);
		auto ptr = std::dynamic_pointer_cast<T>(DeserializeOne(k_GameThingTypeEnum<T>));
		if (ptr)
		{
			list.push_back(*ptr);
		}
	}
	return list;
}

bool GameThingSerializer::GameThing::Deserialize(GameThingSerializer& deserializer)
{
	unknown1 = deserializer.ReadValue<uint32_t>();
	unknown2 = deserializer.ReadValue<uint8_t>();
	return true;
}

bool GameThingSerializer::GameThing::operator==(const GameThingSerializer::GameThing& rhs) const
{
	return unknown1 == rhs.unknown1 && unknown2 == rhs.unknown2;
}

bool GameThingSerializer::GameThing::operator!=(const GameThingSerializer::GameThing& rhs) const
{
	return !(*this == rhs);
}

bool GameThingSerializer::Footpath::Deserialize(GameThingSerializer& deserializer)
{
	GameThing::Deserialize(deserializer);
	nodes = deserializer.DeserializeList<FootpathNode>();
	unknown = deserializer.ReadValue<uint32_t>();
	return true;
}

bool GameThingSerializer::Footpath::operator==(const GameThingSerializer::Footpath& rhs) const
{
	return static_cast<const GameThing&>(*this) == static_cast<const GameThing&>(rhs) && nodes == rhs.nodes &&
	       unknown == rhs.unknown;
}

bool GameThingSerializer::Footpath::operator!=(const GameThingSerializer::Footpath& rhs) const
{
	return !(*this == rhs);
}

bool GameThingSerializer::FootpathLink::Deserialize(GameThingSerializer& deserializer)
{
	GameThing::Deserialize(deserializer);
	footpaths = deserializer.DeserializeList<Footpath>();
	return true;
}

bool GameThingSerializer::FootpathNode::Deserialize(GameThingSerializer& deserializer)
{
	GameThing::Deserialize(deserializer);
	coords = deserializer.ReadValue<MapCoords>();
	unknown = deserializer.ReadValue<uint8_t>();
	return true;
}

bool GameThingSerializer::FootpathNode::operator==(const GameThingSerializer::FootpathNode& rhs) const
{
	return static_cast<const GameThing&>(*this) == static_cast<const GameThing&>(rhs) && coords == rhs.coords &&
	       unknown == rhs.unknown;
}

bool GameThingSerializer::FootpathNode::operator!=(const GameThingSerializer::FootpathNode& rhs) const
{
	return !(*this == rhs);
}

bool GameThingSerializer::FootpathLinkSave::Deserialize(GameThingSerializer& deserializer)
{
	GameThing::Deserialize(deserializer);
	coords = deserializer.ReadValue<MapCoords>();
	auto thing = deserializer.DeserializeOne(GameThingType::FootpathLink);
	if (thing)
	{
		assert(dynamic_cast<FootpathLink*>(thing.get()));
		link = *dynamic_cast<FootpathLink*>(thing.get());
		return true;
	}
	return false;
}

// Force initialization of template function
template std::vector<GameThingSerializer::FootpathLinkSave> GameThingSerializer::DeserializeList();
