/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "Common.h"

namespace openblack::filesystem
{
class Stream;
}

namespace openblack::serializer
{

enum class GameThingType : uint32_t
{
	Invalid = 0x0,
	Footpath = 0x1,
	FootpathLink = 0x2,
	FootpathNode = 0x3,
	FootpathLinkSave = 0x4,
};

template <typename T>
static const GameThingType k_GameThingTypeEnum = GameThingType::Invalid;

class GameThingSerializer
{
public:
	struct GameThing
	{
		uint32_t unknown1;
		uint8_t unknown2;

		virtual ~GameThing() = default;

		virtual bool Deserialize(GameThingSerializer& deserializer);
		bool operator==(const GameThing& rhs) const;
		bool operator!=(const GameThing& rhs) const;
	};

	struct FootpathNode final: GameThing
	{
		MapCoords coords;
		uint8_t unknown;

		bool Deserialize(GameThingSerializer& deserializer) override;
		bool operator==(const FootpathNode& rhs) const;
		bool operator!=(const FootpathNode& rhs) const;
	};

	struct Footpath final: GameThing
	{
		std::vector<FootpathNode> nodes;
		uint32_t unknown;

		bool Deserialize(GameThingSerializer& deserializer) override;
		bool operator==(const Footpath& rhs) const;
		bool operator!=(const Footpath& rhs) const;
	};

	struct FootpathLink final: GameThing
	{
		std::vector<Footpath> footpaths;

		bool Deserialize(GameThingSerializer& deserializer) override;
	};

	struct FootpathLinkSave final: GameThing
	{
		MapCoords coords;
		FootpathLink link;

		bool Deserialize(GameThingSerializer& deserializer) override;
	};

	explicit GameThingSerializer(filesystem::Stream& stream);

	template <typename T>
	T ReadValue();

	void ReadChecksum();

	std::shared_ptr<GameThing> DeserializeOne(std::optional<GameThingType> requiredType = std::nullopt);
	template <typename T>
	std::vector<T> DeserializeList();

private:
	filesystem::Stream& _stream;
	uint32_t _checkSum {0};
	std::vector<std::shared_ptr<GameThing>> _cache;
};
} // namespace openblack::serializer
