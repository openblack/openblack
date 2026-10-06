/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureTattoo.h"
#include "Enums.h"
#include "FileBrowser.h"
#include "Window.h"

namespace openblack::creaturemind
{
struct MindFileData;
}

namespace openblack::debug::gui
{

/// Spawns creatures for trying them out: pick the species, owner, alignment, physique and size, then right click on
/// the land to place one there, until placing is stopped. Lists the creatures on the land, to remove them again. The
/// selected creature can be commanded: while commanding, a right click on the land sends it walking or running there,
/// running away from there or turning to face it, picking up or knocking down what is there, throwing what it holds
/// there or pointing there, and its route is drawn over the land.
class CreatureSpawner final: public Window
{
public:
	CreatureSpawner() noexcept;

	void Close() noexcept override;
	/// Picks a creature, as its Select button in the list does, and opens the window on it
	void Select(entt::entity entity) noexcept;

	/// The window's parts as the editor hosts them: the settings creatures are placed with, the switches for every
	/// creature, and a creature's own sections
	void DrawSpawnSettings() noexcept
	{
		DrawSpawnMind();
		DrawSettings();
	}
	/// The debug file browser while it stands in for a file dialog, once a frame while hosted
	void DrawDialogs() noexcept { DrawFileBrowser(); }
	void DrawSharedSettings() noexcept;
	void DrawCreature(entt::entity entity) noexcept
	{
		_selected = entity;
		DrawSelected();
	}
	[[nodiscard]] std::optional<entt::entity> GetSelected() const noexcept { return _selected; }
	[[nodiscard]] CreatureType GetSpecies() const noexcept { return _species; }
	[[nodiscard]] float GetScale() const noexcept { return _scale; }
	/// Places a creature as the settings say, with the mind and tattoos of the mind file it is to be spawned from, if
	/// any, facing an angle about the up axis
	entt::entity SpawnAt(glm::vec3 position, float yawRadians) noexcept;
	/// While hosted, the clicks on the land that command the picked creature, and their orders
	[[nodiscard]] bool HostedTakesEvent(const SDL_Event& event) const noexcept { return TakesEvent(event); }
	void HostedProcessEvent(const SDL_Event& event) noexcept { ProcessEventOpen(event); }
	void HostedUpdate() noexcept { Update(); }

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;
	[[nodiscard]] bool TakesEvent(const SDL_Event& event) const noexcept override;

private:
	[[nodiscard]] static std::string_view SpeciesName(CreatureType species) noexcept;
	void DrawSettings() noexcept;
	/// The creature picked in the list: its alignment, physique and size, changed as it stands
	void DrawSelected() noexcept;
	/// The picked creature's mind: what it is doing, wants and looks at, and buttons to make it do things
	void DrawMind(entt::entity entity) noexcept;
	/// What the mind has learnt: the planner's plans, opinions, decision trees, recent actions and thoughts, and mind
	/// files to load into it or save it to
	void DrawLearning(entt::entity entity) noexcept;
	/// The picked creature's body: its needs as bars to set, the body's time to speed up, and buttons to make it see to
	/// a need
	void DrawBody(entt::entity entity) noexcept;
	/// The picked creature's tattoos and marks, to put on and take off, and how its shadow and seams are drawn
	void DrawAppearance(entt::entity entity) noexcept;
	/// The mute for every creature's sounds and the switch for other players' creatures' voices
	void DrawAudioSettings() noexcept;
	/// Whether the creatures' footprints are drawn, how many there are, and a switch to leave smiley faces
	void DrawFootprintSettings() noexcept;
	/// The picked creature's last sounds, and buttons to play each sound its animations make
	void DrawAudio(entt::entity entity) noexcept;
	/// The picked creature's leashes: which it knows, the one it wears and how taut it is, buttons to put leashes on,
	/// tie and untie them, and the rope drawn point by point over the scene
	void DrawLeash(entt::entity entity) noexcept;
	/// The picked creature's fights: starting one with another creature, both fighters' health, stamina, state and
	/// queue, orders as the player's clicks give them, fighting by itself, and knocking it out and bringing it round
	void DrawFight(entt::entity entity) noexcept;
	/// The rope's points over the scene, coloured by how far each segment is stretched
	void DrawRope(entt::entity entity) noexcept;
	void DrawPlacing() noexcept;
	/// The picked creature's movement: its speed and route, and the command mode
	void DrawMovement(entt::entity entity) noexcept;
	/// The picked creature's route, over the land
	void DrawRoute(entt::entity entity) noexcept;
	/// The picked creature's hands: what it holds and does with things, things to put by it, buttons to make it act on
	/// what it holds, and how the player's hand last treated it
	void DrawHands(entt::entity entity) noexcept;
	void Command(glm::vec2 screenCoord) noexcept;
	void DrawCreatures() noexcept;
	void Spawn(glm::vec2 screenCoord) noexcept;
	/// Places a creature with the settings, and the mind and tattoos of the mind file it is to be spawned from, if any
	entt::entity SpawnAt(const glm::vec3& position) noexcept;

	/// What a mind file chosen with a file dialog is for
	enum class MindFileUse : uint8_t
	{
		/// Loaded into the selected creature
		LoadIntoSelected,
		/// New creatures are spawned from it: its species, name, body, tattoos and mind
		Spawn,
		/// The selected creature's mind is saved to it
		SaveSelected,
	};
	/// Asks for a mind file with the platform's file dialog, or the debug file browser where there is none
	void ChooseMindFile(MindFileUse use) noexcept;
	/// Loads, spawns from or saves to the chosen mind file, and says what became of it
	void UseMindFile(MindFileUse use, const std::filesystem::path& path) noexcept;
	/// The mind file buttons of the Spawn tab, and the mind file new creatures are spawned from
	void DrawSpawnMind() noexcept;
	/// The debug file browser, while it stands in for a file dialog
	void DrawFileBrowser() noexcept;
	/// Starts the body as a new creature of the species is: its size, fatness and strength
	void UseSpeciesDefaults() noexcept;

	CreatureType _species {CreatureType::Tiger};
	PlayerNames _owner {PlayerNames::PLAYER_ONE};
	float _alignment {0.0f};
	float _fatness {0.5f};
	float _strength {0.5f};
	float _scale {1.0f};
	/// The species the body was last started for, once the game's creature tables are loaded
	std::optional<CreatureType> _defaultsFor;
	float _facingDegrees {180.0f};
	bool _randomFacing {false};

	bool _placing {false};
	/// Where on the screen, 0 to 1, the land was right clicked to place a creature, until it is placed
	std::optional<glm::vec2> _placeAt;
	/// What a right click on the land tells the selected creature to do while commanding, and where it was clicked
	enum class Order : uint8_t
	{
		Walk,
		Run,
		Flee,
		Face,
		PickUp,
		Throw,
		Destroy,
		Point,
	};
	bool _commanding {false};
	Order _order {Order::Walk};
	std::optional<glm::vec2> _commandAt;
	bool _showRoute {true};
	bool _showRope {false};
	/// What became of the last order
	std::string _lastOrder;
	/// What became of the last need it was told to see to
	std::string _lastNeed;
	std::mt19937 _random {std::random_device {}()};
	std::optional<entt::entity> _selected;
	/// The creature the selected tab was last brought to the front for
	std::optional<entt::entity> _tabFor;
	/// The thing to put by the creature, what it does to what it holds, and what became of the last thing it was told to
	/// do with its hands
	int _objectType {0};
	int _keepAction {3};
	std::string _lastHands;
	/// The action and gesture picked to play on the selected creature
	size_t _action {0};
	size_t _gesture {0};
	/// The tattoo being edited: its slot, design, site and colour from the palette at a brightness
	int _tattooSlot {0};
	int _tattooDesign {0};
	int _tattooSite {0};
	int _paletteColumn {0};
	int _paletteRow {64};
	float _tattooBrightness {0.5f};
	/// Where the next wound, burn or blood goes, unless at random, and the wound's kind and column of the damage atlas
	bool _randomMarkPlace {true};
	int _markU {128};
	int _markV {128};
	int _markSkin {0};
	int _woundType {3};
	int _woundColumn {3};
	/// The creature to fight, by its place among the others nearest first, how long blows ordered here are charged,
	/// whether a fight started here is fought by itself, and what became of the last fight started
	int _fightOpponent {0};
	float _fightChargeMs {0.0f};
	bool _fightAuto {true};
	std::string _lastFight;
	/// What became of the last mind file loaded, spawned from or saved
	std::string _lastMindFile;
	/// The mind file new creatures are spawned from, its name, and the tattoos it gives them
	std::shared_ptr<const creaturemind::MindFileData> _spawnMind;
	std::string _spawnMindName;
	std::optional<creature_tattoo::Slots> _spawnTattoos;
	/// Stands in for the file dialog where the platform has none, and what the file it gives is for
	FileBrowser _fileBrowser;
	MindFileUse _browserUse {MindFileUse::LoadIntoSelected};
	/// The skill, miracle and player's deed picked to show the selected creature
	int _skill {0};
	int _miracle {0};
	int _deed {0};
};

} // namespace openblack::debug::gui
