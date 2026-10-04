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
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "3D/OrientedText.h"
#include "3D/TempleInteriorInterface.h"
#include "Graphics/GraphicsHandle.h"

namespace openblack
{
namespace gui
{
class GameFont;
class TextDatabase;
} // namespace gui

/// The scrolls on the temple's walls that the rooms write their texts on: the world's statistics in the main room, the
/// creature's four in its room, the challenges, the save games and the library's seven. Each room's InitEngine makes a
/// texture for each of its LH_Scroll submeshes and writes the scroll's text on it, and SubOptionEntryScroll turns the
/// scroll up and down as the mouse drags it. Pressing a scroll has the camera look at it from close by, and while the
/// camera is close its text is drawn in front of it instead.
class TempleScrolls
{
public:
	/// What the player's creature's scrolls tell of it
	struct CreatureFacts
	{
		std::u16string name;
		int32_t age {0};
		/// Percentages
		int32_t health {0};
		int32_t energy {0};
		int32_t strength {0};
		int32_t fatness {0};
		int32_t exhaustion {0};
		int32_t dehydration {0};
		int32_t poo {0};
		/// From -1, evil, to 1, good
		float alignment {0.0f};
		/// From -1, cold, to 1, hot
		float warmth {0.0f};
		/// From 0, healthy, to 1, ill
		float illness {0.0f};
		/// The miracle cast on the creature, of the 16 HELP_TEXT_CREATURE_RECEIVED_SPELL texts
		std::optional<int32_t> miracleApplied;
		/// People killed, animals killed, creatures defeated, battles fought, battles won, poos and mushrooms eaten
		std::array<int32_t, 7> tallies {};
		/// Taking food from the fields, the totem, the village store, fishing and dancing
		std::array<bool, 5> actionsKnown {};
		/// Each desire the creature has, the text of it ("HELP_TEXT_ROOM_PERSONALITY_GREEDY" and so on) and how much,
		/// of the ten HELP_TEXT_CREATURE_ATTITUDE texts from "extremely"
		struct Desire
		{
			std::string_view text;
			int32_t attitude;
		};
		std::vector<Desire> desires;
		/// What the creature thinks of its god, from -1 to 1, if anything
		std::optional<float> opinionOfGod;
		/// How much attention its god has paid it, from 0 to 1
		float attention {0.0f};
		/// Whether it thinks it knows its god's dominant desire
		bool knowsGodsDesire {false};
		/// The creatures, temples, flocks and forests it knows about
		std::array<int32_t, 4> known {};
		/// The miracles it knows of, the text of each and how much of it it has learnt
		struct Miracle
		{
			std::string_view text;
			int32_t percent;
		};
		std::vector<Miracle> miracles;
	};

	/// A saved game in the save game room: its date and name
	struct SavedGame
	{
		std::u16string date;
		std::u16string name;
	};

	/// What the scrolls tell of the game
	struct Facts
	{
		/// GGame's count of the people in the world
		int32_t population {0};
		int32_t believersPercent {0};
		int32_t malePercent {0};
		int32_t deaths {0};
		int32_t births {0};
		int32_t sacrifices {0};
		int32_t buildings {0};
		int32_t wonders {0};
		int32_t disciples {0};
		/// The disciples of each kind fn_0064BA70 counts, in the order the scroll lists them: builders, breeders,
		/// fishermen, farmers, foresters, missionaries, craftsmen and traders
		std::array<int32_t, 8> discipleKinds {};
		int32_t challengesDiscovered {0};
		int32_t challengesCompleted {0};
		/// The titles of the challenges discovered
		std::vector<std::string_view> challenges;
		std::chrono::seconds timePlayed {0};
		int32_t saveCount {0};
		int32_t loadCount {0};
		std::vector<SavedGame> savedGames;
		/// The help the player has been shown, by the library's kinds: controls, the creature, village life, miracles
		/// and did you know
		std::array<std::vector<std::string_view>, 5> libraryHelp;
		/// The player's creature, without which most of the creature's scrolls aren't written
		std::optional<CreatureFacts> creature;

		/// Made up facts, for the systems openblack doesn't have yet
		static Facts Mock();
	};

	TempleScrolls(const gui::TextDatabase& texts, const gui::GameFont& font, std::vector<uint8_t> parchment);
	~TempleScrolls();
	TempleScrolls(const TempleScrolls&) = delete;
	TempleScrolls& operator=(const TempleScrolls&) = delete;

	/// The rooms' InitEngine: finds each scroll's submesh, makes its texture and writes it
	void Create(const Facts& facts);

	/// Where the camera looks at a scroll from, and the point it looks at, in the temple
	struct Focus
	{
		glm::vec3 position;
		glm::vec3 lookAt;
	};
	/// SubOptionEntryScroll::UpdateMouse and the rooms' scroll callbacks: a press on a scroll takes hold of it, and
	/// while held the mouse moving up and down turns it, rewriting it, with a squeak each time it starts to turn. True
	/// while a scroll has the press, which the camera then doesn't see. A press on a scroll gives where the camera is to
	/// look at it from (Temple::SetCameraToLookAtSubMesh) in focus.
	bool Hold(bool pressed, float mouseY, const std::optional<TempleCursorHit>& hit, const Facts& facts,
	          std::optional<Focus>& focus);
	/// How close the camera is to the scroll it looks at, from 0 to 1. Past half way the scroll's text is drawn in
	/// front of it each frame (Make*Text(1)), and once back under half way it is written on its texture again.
	void SetFocus(float zoom, const Facts& facts);
	/// Whether a scroll is being dragged
	[[nodiscard]] bool IsHeld() const { return _held.has_value(); }
	/// Whether a submesh of a room's mesh is one of its scrolls, a control the cursor makes glow
	[[nodiscard]] bool IsControl(TempleRoom room, uint32_t subMesh) const;
	/// The scrolls' textures for a room's mesh, by submesh
	[[nodiscard]] std::vector<TempleSubMeshTexture> GetTextures(TempleRoom room) const;
	/// A room's scroll, as its callbacks see it to choose the tooltip
	struct Control
	{
		uint32_t subMesh;
		/// Whether the camera was last sent to look at it
		bool focused;
	};
	/// The scrolls of a room's mesh, in the order its callbacks are run
	[[nodiscard]] std::vector<Control> GetControls(TempleRoom room) const;
	/// FormatTextureForScroll with a submesh: the text of the scroll the camera is close to, drawn in front of it in
	/// the temple
	void AppendFocusedText(std::vector<OrientedTextVertex>& vertices, const Facts& facts) const;

	/// The text of a scroll, written as its room's Make*Text writes it
	enum class Content
	{
		World,
		CreatureAttributes,
		CreatureActions,
		CreatureMind,
		CreatureMiracles,
		Challenge,
		SaveGame,
		LibraryStaff,
		LibraryControl,
		LibraryCreature,
		LibraryVillageLife,
		LibraryMiracles,
		LibraryDidYouKnow,
		LibraryHistory,
	};
	/// The text for a scroll, or none when the room leaves the scroll unwritten
	[[nodiscard]] static std::optional<std::u16string> Write(Content content, const gui::TextDatabase& texts,
	                                                         const gui::GameFont& font, const Facts& facts);

private:
	struct Scroll
	{
		TempleRoom room;
		Content content;
		std::optional<uint32_t> subMesh;
		std::optional<graphics::TextureHandle> texture;
		int32_t position {0};
		uint32_t textHeight {0};
		/// The scroll's frame and box, from its mesh
		glm::mat4 frame {1.0f};
		glm::vec3 min {0.0f};
		glm::vec3 max {0.0f};
	};
	/// FormatTextureForScroll into the scroll's texture, its text left off while the camera is close to it
	void Redraw(Scroll& scroll, const Facts& facts);

	const gui::TextDatabase& _texts;
	const gui::GameFont& _font;
	/// ChallengeScroll.raw, 256 by 256 RGB, which PictureRoomBase::InitEngine reads for every scroll
	std::vector<uint8_t> _parchment;
	std::vector<Scroll> _scrolls;
	/// The scroll being dragged and where the mouse was
	std::optional<size_t> _held;
	float _heldMouseY {0.0f};
	bool _wasPressed {false};
	/// Whether the scroll has squeaked since it started turning
	bool _squeaked {false};
	/// The scroll the camera looks at, and whether it is close enough for its text to be drawn in front of it
	std::optional<size_t> _focused;
	bool _focusedText {false};
};

} // namespace openblack
