/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleScrolls.h"

#include <cmath>

#include <algorithm>
#include <string>

#include <bgfx/bgfx.h>
#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/geometric.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/TempleScroll.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Gui/GameFont.h"
#include "Gui/TextDatabase.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;

namespace
{
/// Each room's scrolls: the room, its mesh, the scroll's submesh (the rooms find them by name, whatever the case) and
/// the text written on it
struct ScrollPlace
{
	TempleRoom room;
	std::string_view mesh;
	std::string_view subMesh;
	TempleScrolls::Content content;
};
constexpr std::array k_ScrollPlaces {
    ScrollPlace {TempleRoom::Main, "main", "LH_Scroll_World", TempleScrolls::Content::World},
    ScrollPlace {TempleRoom::CreatureCave, "creature", "LH_SCROLL_CREATURE", TempleScrolls::Content::CreatureAttributes},
    ScrollPlace {TempleRoom::CreatureCave, "creature", "LH_SCROLL_FIGHT", TempleScrolls::Content::CreatureActions},
    ScrollPlace {TempleRoom::CreatureCave, "creature", "LH_SCROLL_LIKES", TempleScrolls::Content::CreatureMind},
    ScrollPlace {TempleRoom::CreatureCave, "creature", "LH_SCROLL_MAGIC", TempleScrolls::Content::CreatureMiracles},
    ScrollPlace {TempleRoom::Challenge, "challenge", "LH_Scroll_Challenge", TempleScrolls::Content::Challenge},
    ScrollPlace {TempleRoom::SaveGame, "savegame", "LH_Scroll_Savegame", TempleScrolls::Content::SaveGame},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB01", TempleScrolls::Content::LibraryStaff},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB02", TempleScrolls::Content::LibraryControl},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB03", TempleScrolls::Content::LibraryCreature},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB04", TempleScrolls::Content::LibraryVillageLife},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB05", TempleScrolls::Content::LibraryMiracles},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB06", TempleScrolls::Content::LibraryDidYouKnow},
    ScrollPlace {TempleRoom::Credits, "credits", "LH_SCROLL_LIB07", TempleScrolls::Content::LibraryHistory},
};

/// The library's first scroll, the people who made the game (fn_0078BDA0)
constexpr std::array<std::u16string_view, 36> k_Staff {
    u"Aaron Ludlow",    u"Alex Evans",          u"Andy Bass",         u"Andy Robson",      u"Catherine Tutton",
    u"Cathy Campos",    u"Christian Bravery",   u"Claire Hedley",     u"Daniel Deptford",  u"Eric Bailey",
    u"Georg Backer",    u"Giles Jermy",         u"James Leach",       u"Jamie Durrant",    u"Janice Nussey",
    u"Jason Hutchens",  u"Jean-Claude Cottier", u"Jeremy Chatelaine", u"Joe Borthwick",    u"Jonty Barnes",
    u"Ken Malcolm",     u"Mark Healey",         u"Mark Webley",       u"Nathan Smethurst", u"Oliver Purkiss",
    u"Paul McLaughlin", u"Paul Nettleton",      u"Pete Hawley",       u"Peter Molyneux",   u"Richard Evans",
    u"Russell Shaw",    u"Scawen Roberts",      u"Steve Jackson",     u"Steve Lawrie",     u"Thomas Barnet-Lamb",
    u"Tim Rance",
};

/// The squeaks of a scroll turning, InGame.sad 54 to 59, one by GetTickCount
constexpr std::array k_Squeaks {
    audio::SoundId::G_ScrollSqueak_01, audio::SoundId::G_ScrollSqueak_02, audio::SoundId::G_ScrollSqueak_03,
    audio::SoundId::G_ScrollSqueak_04, audio::SoundId::G_ScrollSqueak_05, audio::SoundId::G_ScrollSqueak_06,
};

bool SameName(std::string_view a, std::string_view b)
{
	return std::ranges::equal(a, b, [](char x, char y) {
		return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
	});
}

/// swprintf with the conversions the save game room's texts have: "%i", "%d" and "%.2d"
std::u16string Format(std::u16string_view format, std::initializer_list<int32_t> values)
{
	std::u16string result;
	auto value = values.begin();
	for (size_t i = 0; i < format.size(); ++i)
	{
		if (format[i] != u'%' || i + 1 >= format.size())
		{
			result += format[i];
			continue;
		}
		size_t digits = 0;
		size_t j = i + 1;
		if (format[j] == u'.' && j + 2 < format.size())
		{
			digits = static_cast<size_t>(format[j + 1] - u'0');
			j += 2;
		}
		if ((format[j] == u'i' || format[j] == u'd') && value != values.end())
		{
			auto number = std::to_string(std::abs(*value));
			if (number.size() < digits)
			{
				number.insert(0, digits - number.size(), '0');
			}
			result += gui::ToUtf16((*value < 0 ? "-" : "") + number);
			++value;
			i = j;
		}
		else
		{
			result += format[i];
		}
	}
	return result;
}
} // namespace

TempleScrolls::TempleScrolls(const gui::TextDatabase& texts, const gui::GameFont& font, std::vector<uint8_t> parchment)
    : _texts(texts)
    , _font(font)
    , _parchment(std::move(parchment))
{
}

TempleScrolls::~TempleScrolls()
{
	for (auto& scroll : _scrolls)
	{
		if (scroll.texture)
		{
			bgfx::destroy(graphics::toBgfx(*scroll.texture));
		}
	}
}

void TempleScrolls::Create(const Facts& facts)
{
	auto& meshes = Locator::resources::value().GetMeshes();
	for (const auto& place : k_ScrollPlaces)
	{
		Scroll scroll {.room = place.room, .content = place.content, .subMesh = std::nullopt, .texture = std::nullopt};
		const entt::id_type meshId = entt::hashed_string(fmt::format("temple/interior/{}_l3d", place.mesh).c_str()).value();
		if (meshes.Contains(meshId))
		{
			const auto& subMeshes = meshes.Handle(meshId)->GetSubMeshes();
			for (uint32_t i = 0; i < subMeshes.size(); ++i)
			{
				if (SameName(subMeshes[i]->GetName(), place.subMesh))
				{
					scroll.subMesh = i;
					const auto& frame = subMeshes[i]->GetFrame();
					scroll.frame = frame.toMesh;
					scroll.min = frame.min;
					scroll.max = frame.max;
					break;
				}
			}
		}
		if (scroll.subMesh.has_value())
		{
			// fn_008379E0: a texture in memory, 256 texels square, 16 bits a texel
			scroll.texture = graphics::fromBgfx(bgfx::createTexture2D(TempleScrollTexture::k_Size, TempleScrollTexture::k_Size,
			                                                          false, 1, bgfx::TextureFormat::BGRA4, BGFX_TEXTURE_NONE));
			bgfx::setName(graphics::toBgfx(*scroll.texture), fmt::format("Scroll {}", place.subMesh).c_str());
		}
		_scrolls.push_back(scroll);
	}
	for (auto& scroll : _scrolls)
	{
		Redraw(scroll, facts);
	}
}

void TempleScrolls::Redraw(Scroll& scroll, const Facts& facts)
{
	if (!scroll.texture)
	{
		return;
	}
	const bool textInFront = _focusedText && _focused.has_value() && &_scrolls[*_focused] == &scroll;
	std::vector<uint16_t> texels(static_cast<size_t>(TempleScrollTexture::k_Size) * TempleScrollTexture::k_Size);
	if (const auto text = Write(scroll.content, _texts, _font, facts); text.has_value())
	{
		scroll.textHeight =
		    TempleScrollTexture::Draw(texels, _parchment, *text, static_cast<uint32_t>(scroll.position), _font, !textInFront);
	}
	else
	{
		// TODO(raffclar): A scroll its room leaves unwritten keeps what fn_008379E0 made it from: 128 KiB of the game's
		// own memory after the texture's name, read as texels. It is the parchment here.
		TempleScrollTexture::Draw(texels, _parchment, u"", 0, _font);
	}
	bgfx::updateTexture2D(graphics::toBgfx(*scroll.texture), 0, 0, 0, 0, TempleScrollTexture::k_Size,
	                      TempleScrollTexture::k_Size,
	                      bgfx::copy(texels.data(), static_cast<uint32_t>(texels.size() * sizeof(texels[0]))));
}

bool TempleScrolls::Hold(bool pressed, float mouseY, const std::optional<TempleCursorHit>& hit, const Facts& facts,
                         std::optional<Focus>& focus)
{
	const bool pressing = pressed && !_wasPressed;
	_wasPressed = pressed;
	if (pressing && hit.has_value() && hit->subMesh.has_value())
	{
		for (size_t i = 0; i < _scrolls.size(); ++i)
		{
			const auto& scroll = _scrolls[i];
			if (scroll.room == hit->room && scroll.subMesh == hit->subMesh && scroll.texture)
			{
				_held = i;
				_heldMouseY = mouseY;
				_squeaked = false;
				// The rooms' scroll callbacks: Temple::SetCameraToLookAtSubMesh(scroll, -30, 0, 0). InnerCamera::
				// FocusOnSubMesh looks at the middle of the scroll's box from 30 units back along its frame's x axis.
				if (_focused != i)
				{
					if (_focused.has_value() && _focusedText)
					{
						// The scroll looked at before is written on its texture again
						_focusedText = false;
						Redraw(_scrolls[*_focused], facts);
					}
					_focused = i;
				}
				const auto middle = (scroll.min + scroll.max) * 0.5f;
				focus = Focus {
				    .position = glm::vec3(scroll.frame * glm::vec4(middle + glm::vec3(-30.0f, 0.0f, 0.0f), 1.0f)),
				    .lookAt = glm::vec3(scroll.frame * glm::vec4(middle, 1.0f)),
				};
				break;
			}
		}
	}
	if (!pressed)
	{
		_held.reset();
		return false;
	}
	if (!_held.has_value())
	{
		return false;
	}

	// The scroll turns by as many texels as the mouse moves pixels, its text following the mouse
	auto& scroll = _scrolls[*_held];
	const auto delta = static_cast<int32_t>(mouseY) - static_cast<int32_t>(_heldMouseY);
	_heldMouseY = mouseY;
	if (delta == 0)
	{
		_squeaked = false;
		return true;
	}
	const auto previous = scroll.position;
	scroll.position = TempleScrollTexture::ClampPosition(scroll.position - delta, scroll.textHeight);
	Redraw(scroll, facts);
	if (scroll.position != previous && !_squeaked)
	{
		_squeaked = true;
		if (Locator::audio::has_value())
		{
			const auto ticks =
			    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			        .count();
			const auto squeak = k_Squeaks.at(static_cast<size_t>(ticks % static_cast<int64_t>(k_Squeaks.size())));
			Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(squeak), std::nullopt);
		}
	}
	return true;
}

void TempleScrolls::SetFocus(float zoom, const Facts& facts)
{
	if (!_focused.has_value())
	{
		return;
	}
	// CreatureRoom::Draw and WorldRoom::Draw: the text goes in front of the scroll once the camera is over half way to
	// it, and back on the texture when it is back under half way, when the camera no longer looks at it
	const bool textInFront = zoom > 0.5f;
	if (textInFront != _focusedText)
	{
		_focusedText = textInFront;
		Redraw(_scrolls[*_focused], facts);
		if (!textInFront)
		{
			_focused.reset();
		}
	}
}

bool TempleScrolls::IsControl(TempleRoom room, uint32_t subMesh) const
{
	return std::ranges::any_of(
	    _scrolls, [room, subMesh](const Scroll& scroll) { return scroll.room == room && scroll.subMesh == subMesh; });
}

std::vector<TempleSubMeshTexture> TempleScrolls::GetTextures(TempleRoom room) const
{
	std::vector<TempleSubMeshTexture> textures;
	for (const auto& scroll : _scrolls)
	{
		if (scroll.room == room && scroll.subMesh.has_value() && scroll.texture.has_value())
		{
			textures.push_back({*scroll.subMesh, *scroll.texture});
		}
	}
	return textures;
}

void TempleScrolls::AppendFocusedText(std::vector<OrientedTextVertex>& vertices, const Facts& facts) const
{
	if (!_focusedText || !_focused.has_value())
	{
		return;
	}
	const auto& scroll = _scrolls[*_focused];
	const auto text = Write(scroll.content, _texts, _font, facts);
	if (!text.has_value())
	{
		return;
	}
	// FormatTextureForScroll with a submesh: the frame's axes made unit long, its y and z axes turned round, at the
	// middle of the scroll's box. The texture's 237 texels across fit the box's depth along z and its 255 down its
	// height along y.
	TextFrame frame;
	std::array<float, 3> lengths {};
	for (size_t i = 0; i < 3; ++i)
	{
		const auto axis = glm::vec3(scroll.frame[static_cast<glm::length_t>(i)]);
		lengths.at(i) = glm::length(axis);
		frame.axes.at(i) = lengths.at(i) > 0.0f ? axis / lengths.at(i) : axis;
	}
	frame.axes[1] = -frame.axes[1];
	frame.axes[2] = -frame.axes[2];
	frame.origin = glm::vec3(scroll.frame * glm::vec4((scroll.min + scroll.max) * 0.5f, 1.0f));
	const float across = std::abs(scroll.max.z - scroll.min.z) * lengths[2] / ((256.0f / 237.0f) * 256.0f);
	const float down = std::abs(scroll.max.y - scroll.min.y) * lengths[1] / ((256.0f / 255.0f) * 256.0f);
	if (down <= 0.0f)
	{
		return;
	}
	const float size = static_cast<float>(TempleScrollTexture::k_LineHeight) * down;
	const float stretch = TempleScrollTexture::k_Stretch * across / down;
	constexpr float k_Size = TempleScrollTexture::k_Size;
	constexpr float k_Middle = k_Size * 0.5f;
	TempleScrollTexture::LayOut(
	    *text, static_cast<uint32_t>(scroll.position), _font, [&](std::u16string_view line, float x, float y) {
		    if (y < -static_cast<float>(TempleScrollTexture::k_LineHeight) || y > k_Size)
		    {
			    return;
		    }
		    // GatheringText::DrawTextRawOriented along the frame's z axis and down its y, 0.7 units in front: the shadow
		    // a texel down and right, then the yellow
		    AppendOrientedText(vertices, _font, frame, 2, 1, line,
		                       {(x - k_Middle + 1.0f) * across, ((y - k_Middle) * down) + down, -0.7f}, size, stretch,
		                       {0, 0, 0, 255});
		    AppendOrientedText(vertices, _font, frame, 2, 1, line, {(x - k_Middle) * across, (y - k_Middle) * down, -0.7f},
		                       size, stretch, {255, 255, 0, 255});
	    });
}

TempleScrolls::Facts TempleScrolls::Facts::Mock()
{
	// TODO(raffclar): Made up until openblack keeps the game's statistics, the player's creature, its challenges, saved
	// games and the help it has shown
	Facts facts;
	facts.believersPercent = 62;
	facts.malePercent = 48;
	facts.deaths = 12;
	facts.births = 17;
	facts.sacrifices = 3;
	facts.buildings = 24;
	facts.wonders = 1;
	facts.disciples = 9;
	facts.discipleKinds = {2, 1, 1, 2, 1, 0, 1, 1};
	facts.challengesDiscovered = 2;
	facts.challengesCompleted = 1;
	// The titles come from the lands' scripts, which openblack doesn't give them yet
	facts.challenges = {"HELP_TEXT_GENERAL_CHALLENGE_START_02", "HELP_TEXT_GENERAL_CHALLENGE_START_04"};
	facts.saveCount = 4;
	facts.loadCount = 2;
	facts.savedGames = {{u"04/10/2026 21:14", u"Land 1"}, {u"03/10/2026 18:02", u""}};
	facts.libraryHelp = {
	    std::vector<std::string_view> {"HELP_TEXT_DYK_01"},
	    std::vector<std::string_view> {"HELP_TEXT_DYK_02"},
	    std::vector<std::string_view> {"HELP_TEXT_DYK_03"},
	    std::vector<std::string_view> {},
	    std::vector<std::string_view> {"HELP_TEXT_DYK_01", "HELP_TEXT_DYK_02", "HELP_TEXT_DYK_03"},
	};
	CreatureFacts creature;
	creature.name = u"Ape";
	creature.age = 3;
	creature.health = 87;
	creature.energy = 64;
	creature.strength = 42;
	creature.fatness = 35;
	creature.exhaustion = 18;
	creature.dehydration = 22;
	creature.poo = 40;
	creature.alignment = 0.3f;
	creature.warmth = 0.1f;
	creature.illness = 0.1f;
	creature.tallies = {5, 12, 1, 2, 1, 8, 3};
	creature.actionsKnown = {true, false, true, false, true};
	creature.desires = {
	    {"HELP_TEXT_ROOM_PERSONALITY_GREEDY", 6},        {"HELP_TEXT_ROOM_PERSONALITY_LETHARGIC", 8},
	    {"HELP_TEXT_ROOM_PERSONALITY_COMPASSIONATE", 2}, {"HELP_TEXT_ROOM_PERSONALITY_TERRIFIED", 1},
	    {"HELP_TEXT_ROOM_PERSONALITY_PLAYFUL", 4},
	};
	creature.opinionOfGod = 0.3f;
	creature.known = {0, 1, 2, 3};
	creature.miracles = {
	    {"HELP_TEXT_CREATURE_LESSON_LEARN_MAGIC_ACTION_02", 12},
	    {"HELP_TEXT_CREATURE_LESSON_LEARN_MAGIC_ACTION_03", 9},
	    {"HELP_TEXT_CREATURE_LESSON_LEARN_MAGIC_ACTION_10", 97},
	};
	facts.creature = creature;
	return facts;
}

std::optional<std::u16string> TempleScrolls::Write(Content content, const gui::TextDatabase& texts, const gui::GameFont& font,
                                                   const Facts& facts)
{
	TempleScrollText text(font);
	const auto get = [&texts](std::string_view name) { return texts.Get(name); };
	const auto number = [&](std::string_view name, int32_t value) { text.Add(TempleScrollText::WithNumber(get(name), value)); };
	const auto named = [&](std::string_view name, std::u16string_view value) {
		text.Add(TempleScrollText::WithString(get(name), value));
	};
	// GAlignment::GetDiscreteAlignmentValue's seven texts, from totally evil to angelic
	const auto alignment = [&](float value) {
		const auto n = std::clamp(static_cast<int32_t>((value + 1.0f) * 0.5f * 6.9999995f), 0, 6);
		return get(fmt::format("HELP_TEXT_ALIGNMENT_{:02}", n + 1));
	};
	const auto* creature = facts.creature ? &*facts.creature : nullptr;
	// AddText's texts about the player's creature are a blank line without one
	const auto aboutCreature = [&](const auto& write) {
		if (creature != nullptr)
		{
			write(*creature);
		}
		else
		{
			text.AddNewLine();
		}
	};

	switch (content)
	{
	case Content::World: // WorldRoom::MakeScrollText
	{
		text.Add(get("HELP_TEXT_ROOM_WORLD_TITLE"));
		text.AddNewLine();
		number("HELP_TEXT_ROOM_TOTAL_POPULATION", facts.population);
		number("HELP_TEXT_ROOM_PERCENT_OF_POPULATION_BELIEVE_IN_YOU", facts.believersPercent);
		number("HELP_TEXT_ROOM_PERCENT_OF_POPULATION_THAT_ARE_MALE", facts.malePercent);
		text.AddNewLine();
		number("HELP_TEXT_ROOM_NUMBER_OF_DEATHS_IN_YOUR_REGION", facts.deaths);
		number("HELP_TEXT_ROOM_NUMBER_OF_BIRTHS_IN_YOUR_REGION", facts.births);
		// "%s: %i" of the pause screen's statistic
		text.Add(std::u16string(get("HELP_TEXT_PAUSE_STATS_108")) + u": " + gui::ToUtf16(std::to_string(facts.sacrifices)));
		text.AddNewLine();
		number("HELP_TEXT_ROOM_NUMBER_OF_BUILDINGS_BUILT", facts.buildings);
		number("HELP_TEXT_ROOM_NUMBER_OF_WONDERS_BUILT", facts.wonders);
		text.AddNewLine();
		number("HELP_TEXT_ROOM_NUMBER_OF_DISCIPLES", facts.disciples);
		// Protectors aren't listed
		constexpr std::array<std::string_view, 8> k_Disciples {
		    "HELP_TEXT_ROOM_NUMBER_OF_BUILDERS",  "HELP_TEXT_ROOM_NUMBER_OF_BREEDERS",  "HELP_TEXT_ROOM_NUMBER_OF_FISHERMEN",
		    "HELP_TEXT_ROOM_NUMBER_OF_FARMERS",   "HELP_TEXT_ROOM_NUMBER_OF_FORESTERS", "HELP_TEXT_ROOM_NUMBER_OF_MISSIONARIES",
		    "HELP_TEXT_ROOM_NUMBER_OF_CRAFTSMEN", "HELP_TEXT_ROOM_NUMBER_OF_TRADERS",
		};
		for (size_t i = 0; i < k_Disciples.size(); ++i)
		{
			number(k_Disciples.at(i), facts.discipleKinds.at(i));
		}
		text.End();
		break;
	}
	case Content::CreatureAttributes: // CreatureRoom::MakeCreatureScrollText
	{
		text.Add(get("HELP_TEXT_ROOM_CREATURE_SCROLL_TITLE"));
		text.AddNewLine();
		aboutCreature([&](const CreatureFacts& c) { named("HELP_TEXT_ROOM_CREATURE_SCROLL_NAME", c.name); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_AGE", c.age); });
		text.AddNewLine();
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_HEALTH", c.health); });
		aboutCreature(
		    [&](const CreatureFacts& c) { named("HELP_TEXT_ROOM_CREATURE_SCROLL_ALIGNMENT", alignment(c.alignment)); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_ENERGY", c.energy); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_STRENGTH", c.strength); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_FATNESS", c.fatness); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_EXHAUSTION", c.exhaustion); });
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_DEHYDRATION", c.dehydration); });
		aboutCreature([&](const CreatureFacts& c) {
			const auto n = std::clamp(static_cast<int32_t>((c.warmth + 1.0f) * 0.5f * 5.0f), 0, 4);
			named("HELP_TEXT_ROOM_CREATURE_SCROLL_WARMTH", get(fmt::format("HELP_TEXT_HOW_WARM_{:02}", n + 1)));
		});
		aboutCreature([&](const CreatureFacts& c) {
			const auto n = std::clamp(static_cast<int32_t>(c.illness * 5.0f), 0, 4);
			named("HELP_TEXT_ROOM_CREATURE_SCROLL_ILLNESS", get(fmt::format("HELP_TEXT_HOW_ILL_{:02}", n + 1)));
		});
		aboutCreature([&](const CreatureFacts& c) {
			named("HELP_TEXT_ROOM_CREATURE_SCROLL_SPELLS_RECEIVED",
			      c.miracleApplied ? get(fmt::format("HELP_TEXT_CREATURE_RECEIVED_SPELL_{:02}", *c.miracleApplied + 1))
			                       : get("HELP_TEXT_DIALOG_ADDITION_101"));
		});
		aboutCreature([&](const CreatureFacts& c) { number("HELP_TEXT_ROOM_CREATURE_SCROLL_AMOUNTOFPOO", c.poo); });
		text.AddNewLine();
		constexpr std::array<std::string_view, 7> k_Tallies {
		    "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFPEOPLEKILLED",    "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFANIMALSKILLED",
		    "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFCREATURESKILLED", "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFBATTLESFOUGHT",
		    "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFBATTLESWON",      "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFPOOS",
		    "HELP_TEXT_ROOM_CREATURE_SCROLL_NUMBEROFMUSHROOMSEATEN",
		};
		for (size_t i = 0; i < k_Tallies.size(); ++i)
		{
			aboutCreature([&](const CreatureFacts& c) { number(k_Tallies.at(i), c.tallies.at(i)); });
		}
		text.End();
		break;
	}
	case Content::CreatureActions: // CreatureRoom::MakeActionsLearntScrollText, only for the player's creature
		if (creature == nullptr)
		{
			return std::nullopt;
		}
		text.Add(get("HELP_TEXT_ROOM_ACTIONS_LEARNT_SCROLL_TITLE"));
		text.AddNewLine();
		// What it has learnt of taking food from the fields, the totem, the village store, fishing and dancing, or how
		// to teach it
		for (size_t i = 0; i < creature->actionsKnown.size(); ++i)
		{
			text.Add(get(creature->actionsKnown.at(i)
			                 ? fmt::format("HELP_TEXT_CREATURE_LESSON_LEARN_NORMAL_ACTION_{:02}", i + 2)
			                 : fmt::format("HELP_TEXT_CREATURE_LESSON_HASNT_LEARN_NORMAL_ACTION_{:02}", i + 2)));
			text.AddNewLine();
		}
		text.End();
		break;
	case Content::CreatureMind: // CreatureRoom::MakePersonalityScrollText, only for the player's creature
		if (creature == nullptr)
		{
			return std::nullopt;
		}
		text.Add(get("HELP_TEXT_ROOM_LIKES_SCROLL_TITLE"));
		text.AddNewLine();
		for (const auto& desire : creature->desires)
		{
			named(desire.text, get(fmt::format("HELP_TEXT_CREATURE_ATTITUDE_{:02}", std::clamp(desire.attitude, 0, 9) + 1)));
		}
		text.AddNewLine();
		if (creature->opinionOfGod)
		{
			named("HELP_TEXT_ROOM_PERSONALITY_ATTITUDE_TO_PLAYER", alignment(*creature->opinionOfGod));
		}
		named("HELP_TEXT_ROOM_PERSONALITY_HOW_MUCH_ATTENTION_PLAYER_HAS_GIVEN_ME",
		      get(fmt::format("HELP_TEXT_HOW_MUCH_{:02}",
		                      std::clamp(static_cast<int32_t>(creature->attention * 5.0f), 0, 4) + 1)));
		if (creature->knowsGodsDesire)
		{
			// Always "I want to impress somebody.", whatever the desire
			named("HELP_TEXT_ROOM_PERSONALITY_PERCEIVED_PLAYER_DESIRE",
			      get("HELP_TEXT_CREATURE_CURRENT_DESIRE_FIRST_PERSON_01"));
			text.AddNewLine();
		}
		// TODO(raffclar): What it thinks of each creature it knows of
		text.AddNewLine();
		number("HELP_TEXT_ROOM_PERSONALITY_BELIEFS_ABOUT_CREATURES", creature->known[0]);
		number("HELP_TEXT_ROOM_PERSONALITY_BELIEFS_ABOUT_CITADELS", creature->known[1]);
		number("HELP_TEXT_ROOM_PERSONALITY_BELIEFS_ABOUT_FLOCKS", creature->known[2]);
		number("HELP_TEXT_ROOM_PERSONALITY_BELIEFS_ABOUT_FORESTS", creature->known[3]);
		// The game doesn't end this scroll's text, which reads the same
		break;
	case Content::CreatureMiracles: // CreatureRoom::MakeMagicScrollText
		text.Add(get("HELP_TEXT_ROOM_MAGIC_SCROLL_TITLE"));
		text.AddNewLine();
		if (creature != nullptr)
		{
			for (const auto& miracle : creature->miracles)
			{
				text.Add(get(miracle.text));
				text.Add(gui::ToUtf16(fmt::format("{}%", miracle.percent)));
				text.AddNewLine();
			}
		}
		text.End();
		break;
	case Content::Challenge: // ChallengeRoom::MakeScrollText
		text.Add(get("HELP_TEXT_ROOM_CHALLENGE_TITLE"));
		text.AddNewLine();
		number("HELP_TEXT_ROOM_CHALLENGE_LIST_NUMBER_OF_DISCOVERED_CHALLENGES", facts.challengesDiscovered);
		number("HELP_TEXT_ROOM_CHALLENGE_LIST_NUMBER_OF_COMPLETED_CHALLENGES", facts.challengesCompleted);
		text.AddNewLine();
		for (const auto challenge : facts.challenges)
		{
			text.Add(get(challenge));
		}
		text.End();
		break;
	case Content::SaveGame: // SaveGameRoom::MakeScrollText
	{
		text.Add(get("HELP_TEXT_ROOM_SAVEGAME_TITLE"));
		text.AddNewLine();
		const auto seconds = static_cast<int32_t>(facts.timePlayed.count());
		text.Add(Format(get("HELP_TEXT_DIALOG_ADDITION_136"), {seconds / 3600, (seconds / 60) % 60, seconds % 60}));
		text.Add(Format(get("HELP_TEXT_DIALOG_ADDITION_119"), {facts.saveCount}));
		text.Add(Format(get("HELP_TEXT_DIALOG_ADDITION_84"), {facts.loadCount}));
		text.AddNewLine();
		for (const auto& saved : facts.savedGames)
		{
			text.Add(saved.date);
			text.Add(saved.name.empty() ? get("HELP_TEXT_NONAME_SAVED_GAME_01") : std::u16string_view(saved.name));
			text.AddNewLine();
		}
		text.End();
		break;
	}
	case Content::LibraryStaff:
		for (const auto name : k_Staff)
		{
			text.Add(name);
		}
		text.End();
		break;
	case Content::LibraryControl:     // CreditsRoom::MakeNavigationText
	case Content::LibraryCreature:    // CreditsRoom::MakeCreatureText
	case Content::LibraryVillageLife: // CreditsRoom::MakeVillageLifeText
	case Content::LibraryMiracles:    // CreditsRoom::MakeMiraclesText
	case Content::LibraryDidYouKnow:  // CreditsRoom::MakeMiscText
	{
		constexpr std::array<std::string_view, 5> k_Titles {
		    "HELP_TEXT_LIBRARY_ROOM_02", "HELP_TEXT_LIBRARY_ROOM_03", "HELP_TEXT_LIBRARY_ROOM_04",
		    "HELP_TEXT_LIBRARY_ROOM_05", "HELP_TEXT_LIBRARY_ROOM_06",
		};
		const auto kind = static_cast<size_t>(content) - static_cast<size_t>(Content::LibraryControl);
		text.Add(get(k_Titles.at(kind)));
		text.AddNewLine();
		for (const auto help : facts.libraryHelp.at(kind))
		{
			text.Add(get(help));
			text.AddNewLine();
		}
		text.End();
		break;
	}
	case Content::LibraryHistory: // CreditsRoom::MakeHistoryText
		text.Add(get("HELP_TEXT_LIBRARY_ROOM_07"));
		text.AddNewLine();
		// TODO(raffclar): Up to five entries of the story so far, each its caption and text and two blank lines
		text.End();
		break;
	}
	return text.GetText();
}
