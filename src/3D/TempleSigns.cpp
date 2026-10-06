/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleSigns.h"

#include <cmath>

#include <array>
#include <numbers>
#include <string_view>

#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/geometric.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "Gui/GameFont.h"
#include "Gui/TextDatabase.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;

namespace
{
/// The labels' colour where a room gives none
constexpr glm::u8vec3 k_DefaultColour {200, 200, 50};

/// Each room's signs: the submesh, its label's text and colour, in the order the rooms list them
struct SignPlace
{
	std::string_view subMesh;
	std::string_view text;
	glm::u8vec3 colour;
};
struct RoomSigns
{
	TempleRoom room;
	std::string_view mesh;
	/// The labels' size and how much the letters are stretched along the text, before the game's own 2.1 and 0.9
	float size;
	float stretch;
	/// Whether the label runs along the sign's x axis, as the creature's room and the library have it,
	/// rather than against its y axis
	bool alongX;
	std::array<SignPlace, 7> signs;
	size_t count;
};
const std::array k_Rooms {
    // The main room, each sign in its own colour
    RoomSigns {TempleRoom::Main,
               "main",
               1.0f,
               1.0f,
               false,
               {
                   SignPlace {"SIGN CREATURE", "HELP_TEXT_ROOM_CREATURE_TITLE", {236, 198, 100}},
                   SignPlace {"SIGN OPTIONS", "HELP_TEXT_ROOM_GAMEOPTIONS_TITLE", {229, 212, 65}},
                   SignPlace {"SIGN EXIT", "HELP_TEXT_ROOM_EXIT_TITLE", {222, 227, 239}},
                   SignPlace {"SIGN MULTIPLKAYER", "HELP_TEXT_ROOM_UNIVERSE_TITLE", {111, 146, 235}},
                   SignPlace {"SIGN CREDITS", "HELP_TEXT_ROOM_CREDITS_TITLE", {212, 107, 251}},
                   SignPlace {"SIGN SAVEGAME", "HELP_TEXT_ROOM_SAVEGAME_TITLE", {156, 209, 64}},
                   SignPlace {"SIGN CHALLENGE", "HELP_TEXT_ROOM_CHALLENGE_TITLE", {235, 146, 111}},
               },
               7},
    // The creature's room
    RoomSigns {TempleRoom::CreatureCave,
               "creature",
               1.3f,
               1.0f,
               true,
               {
                   SignPlace {"SIGN ATTRIBUTES", "HELP_TEXT_TEMPLE_SCROLLS_11", k_DefaultColour},
                   SignPlace {"SIGN LEARNING", "HELP_TEXT_TEMPLE_SCROLLS_08", k_DefaultColour},
                   SignPlace {"SIGN MIND", "HELP_TEXT_TEMPLE_SCROLLS_10", k_DefaultColour},
                   SignPlace {"SIGN MIRACLES", "HELP_TEXT_TEMPLE_SCROLLS_09", k_DefaultColour},
               },
               4},
    // The library. The signs are paired with their texts by what they say; the order of the game's own list of
    // their names is unverified.
    RoomSigns {TempleRoom::Credits,
               "credits",
               0.8f,
               1.0f,
               true,
               {
                   SignPlace {"SIGN CONTROL", "HELP_TEXT_TEMPLE_SCROLLS_02", k_DefaultColour},
                   SignPlace {"SIGN CREATURE", "HELP_TEXT_TEMPLE_SCROLLS_03", k_DefaultColour},
                   SignPlace {"SIGN CREDITS", "HELP_TEXT_TEMPLE_SCROLLS_01", k_DefaultColour},
                   SignPlace {"SIGN DIDYOUKNOW", "HELP_TEXT_TEMPLE_SCROLLS_06", k_DefaultColour},
                   SignPlace {"SIGN HISTORY", "HELP_TEXT_TEMPLE_SCROLLS_07", k_DefaultColour},
                   SignPlace {"SIGN MIRACLES", "HELP_TEXT_TEMPLE_SCROLLS_05", k_DefaultColour},
                   SignPlace {"SIGN VILLAGE", "HELP_TEXT_TEMPLE_SCROLLS_04", k_DefaultColour},
               },
               7},
};

const RoomSigns* RoomOf(TempleRoom room)
{
	for (const auto& rooms : k_Rooms)
	{
		if (rooms.room == room)
		{
			return &rooms;
		}
	}
	return nullptr;
}
} // namespace

TempleSigns::TempleSigns(const gui::TextDatabase& texts, const gui::GameFont& font)
    : _font(font)
{
	auto& meshes = Locator::resources::value().GetMeshes();
	for (const auto& room : k_Rooms)
	{
		const entt::id_type meshId = entt::hashed_string(fmt::format("temple/interior/{}_l3d", room.mesh).c_str()).value();
		for (size_t i = 0; i < room.count; ++i)
		{
			const auto& place = room.signs.at(i);
			Sign sign {.room = room.room, .text = std::u16string(texts.Get(place.text)), .colour = place.colour};
			// The signs are found by name
			if (meshes.Contains(meshId))
			{
				for (const auto& subMesh : meshes.Handle(meshId)->GetSubMeshes())
				{
					if (subMesh->GetName() == place.subMesh)
					{
						sign.frame = subMesh->GetFrame().toMesh;
						sign.min = subMesh->GetFrame().min;
						sign.max = subMesh->GetFrame().max;
						sign.found = true;
						break;
					}
				}
			}
			_signs.push_back(std::move(sign));
		}
	}
}

std::optional<size_t> TempleSigns::MainRoomSignOfDoor(std::optional<uint32_t> door)
{
	if (!door.has_value())
	{
		return std::nullopt;
	}
	// Only the challenge room's door is moved down to its sign, so the wall of scrolls would light that sign too
	return *door == 7 ? 6 : *door;
}

void TempleSigns::Append(std::vector<OrientedTextVertex>& vertices, TempleRoom room, std::optional<size_t> highlighted,
                         uint32_t milliseconds) const
{
	const auto* rooms = RoomOf(room);
	if (rooms == nullptr)
	{
		return;
	}
	// The highlighted label's grey, a second round
	const auto phase = static_cast<float>(milliseconds & 0x3FFu) * (2.0f * std::numbers::pi_v<float> / 1024.0f);
	const auto grey = static_cast<uint8_t>(std::sin(phase) * 24.0f + 230.0f);
	const float size = rooms->size * 2.1f;
	const float stretch = rooms->stretch * 0.9f;
	size_t index = 0;
	for (const auto& sign : _signs)
	{
		if (sign.room != room)
		{
			continue;
		}
		const auto label = index++;
		if (!sign.found || sign.text.empty())
		{
			continue;
		}
		// The sign's frame with its axes made unit long and its y axis turned round, at the middle of its box
		TextFrame frame;
		for (size_t i = 0; i < 3; ++i)
		{
			const auto axis = glm::vec3(sign.frame[static_cast<glm::length_t>(i)]);
			const auto length = glm::length(axis);
			frame.axes.at(i) = length > 0.0f ? axis / length : axis;
		}
		frame.axes[1] = -frame.axes[1];
		frame.origin = glm::vec3(sign.frame * glm::vec4((sign.min + sign.max) * 0.5f, 1.0f));
		const int across = rooms->alongX ? 0 : 1;
		// Centred along the text and down its letters
		const float x = _font.GetWidth(sign.text, size) * stretch * -0.5f;
		const float y = size * -0.5f;
		AppendOrientedText(vertices, _font, frame, across, 2, sign.text, {x, y - (rooms->size * 0.21f), -0.4f}, size, stretch,
		                   {0, 0, 0, 255});
		const auto colour = highlighted == label ? glm::u8vec3(grey) : sign.colour;
		AppendOrientedText(vertices, _font, frame, across, 2, sign.text, {x, y, -0.6f}, size, stretch,
		                   {colour.r, colour.g, colour.b, 255});
	}
}
