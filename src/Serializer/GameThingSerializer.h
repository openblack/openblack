/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "Common.h"

namespace openblack
{
class FileStream;
}

namespace openblack::serializer
{

enum class GameThingType : uint32_t
{
	Footpath = 0x1,
	FootpathLink = 0x2,
	FootpathNode = 0x3,
	FootpathLinkSave = 0x4,
};

template <typename T>
GameThingType GameThingTypeEnum;

class GameThingSerializer
{
public:
	struct GameThing
	{
		uint32_t _unknown1;
		uint8_t _unknown2;

		virtual ~GameThing() = default;

		virtual bool Deserialize(GameThingSerializer& deserializer);
		bool operator==(const GameThing& rhs) const;
		bool operator!=(const GameThing& rhs) const;
	};

	struct FootpathNode final: GameThing
	{
		MapCoords _coords;
		uint8_t _unknown;

		bool Deserialize(GameThingSerializer& deserializer) override;
		bool operator==(const FootpathNode& rhs) const;
		bool operator!=(const FootpathNode& rhs) const;
	};

	struct Footpath final: GameThing
	{
		std::vector<FootpathNode> _nodes;
		uint32_t _unknown;

		bool Deserialize(GameThingSerializer& deserializer) override;
		bool operator==(const Footpath& rhs) const;
		bool operator!=(const Footpath& rhs) const;
	};

	struct FootpathLink final: GameThing
	{
		std::vector<Footpath> _footpaths;

		bool Deserialize(GameThingSerializer& deserializer) override;
	};

	struct FootpathLinkSave final: GameThing
	{
		MapCoords _coords;
		FootpathLink _link;

		bool Deserialize(GameThingSerializer& deserializer) override;
	};

	explicit GameThingSerializer(FileStream& stream);

	template <typename T>
	T ReadValue();

	void ReadChecksum();

	std::shared_ptr<GameThing> DeserializeOne(std::optional<GameThingType> required_type = std::nullopt);
	template <typename T>
	std::vector<T> DeserializeList();

private:
	FileStream& _stream;
	uint32_t _checkSum;
	std::vector<std::shared_ptr<GameThing>> _cache;
};
} // namespace openblack::serializer
