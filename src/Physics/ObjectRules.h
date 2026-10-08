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

#include <optional>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The rules for trees and rocks as the physics lands and knocks them: whether a tree is planted again where it comes
/// down or falls dead, which forest a planted tree joins, how hard knocks wear a rock away, and how a rock breaks in two.
namespace openblack::physics::objects
{

/// What a tree does as its flight ends
enum class TreeLanding : uint8_t
{
	/// It stays where it is, standing: a forest miracle's tree put down gently, or one ending without going back on the map
	Stays,
	/// It takes root again where it was put down
	Replanted,
	/// It falls and lies as a dead tree
	Dies,
};

/// A tree put down gently on land and not burning takes root again; a forest miracle's tree put down gently just stays;
/// anything else (thrown, dropped tilted, in the water, burning) dies
[[nodiscard]] TreeLanding TreeLandingOf(bool putDownGently, bool forestMiracleTree, bool onLand, bool burning);

/// A replanted tree looks for company over this many cells of a spiral round it
inline constexpr int k_ReplantSearchCells = 1000;
/// It looks at what stands within this many metres of the edge of a thing
inline constexpr float k_ReplantNearEdge = 25.0f;
/// The spiral stops once its cell is this far from the tree
inline constexpr float k_ReplantSearchReach = k_ReplantNearEdge + 10.0f;

/// Which forest a replanted tree joins, as the things round it are met: the first fixed thing of a town (or a part of a
/// temple) within reach of its edge makes the tree near the town and takes it into a forest of that town if it has one;
/// otherwise the tree that is nearest by its edge lends its forest.
class ForestSearch
{
public:
	/// A fixed thing met in the spiral, with its distance from the tree less its own radius
	struct Thing
	{
		float edgeDistance {0.0f};
		/// It belongs to a town, or is part of a temple
		bool ofTown {false};
		/// The forest of its town, when it is of a town that has one
		std::optional<uint32_t> townForest;
		/// The forest it stands in, when it is a tree of one
		std::optional<uint32_t> forest;
	};

	/// Another thing of the current cell; false when the rest of the cell is skipped, as after a town's thing
	bool Meet(const Thing& thing);

	/// Whether something of a town was close
	[[nodiscard]] bool NearTown() const { return _nearTown; }
	/// The forest to join, none when the tree is near a town without a forest
	[[nodiscard]] std::optional<uint32_t> Forest() const { return _forest; }
	/// Whether the tree starts a forest of its own: nothing to join and no town near
	[[nodiscard]] bool StartsForest() const { return !_forest.has_value() && !_nearTown; }

private:
	/// The edge distance a tree's forest must beat; a town's forest wins outright
	float _best {99999.0f};
	std::optional<uint32_t> _forest;
	bool _nearTown {false};
};

/// The player planting a tree is something the player's creature may copy
inline constexpr uint32_t k_DeedPlantTree = 11;
/// A replanted tree away from towns shows the forest-made visual this strong, for this many turns
inline constexpr float k_ReplantVisualMagnitude = 0.3f;
inline constexpr int k_ReplantVisualTurns = 50;
/// A replanted tree throws up a white puff of smoke this big
inline constexpr float k_ReplantSmokeSize = 1.0f;
inline constexpr uint32_t k_ReplantSmokeColour = 0xFFFFFFu;

/// A turned thing's axes stood upright, keeping only its heading: what stands upright takes from a body
[[nodiscard]] glm::mat3 HeadingOnly(const glm::mat3& axes);

/// A rock knocked harder than this many times its own weight wears away
inline constexpr float k_RockWearG = 4.0f;
/// Only a rock taller than this wears away, and only a rock this tall can be broken by tapping it
inline constexpr float k_RockLeastHeight = 0.7f;
/// What a knock takes from its life for each g over the limit
inline constexpr float k_RockWearPerG = 0.005f;
/// A rock worn below this breaks in two
inline constexpr float k_RockBreakLife = 0.01f;

/// The life a knock takes from a rock: none when the knock is soft, the rock is short, or another rock struck it
[[nodiscard]] std::optional<float> RockWear(float g, float height, bool struckByRock);

/// Each half of a broken rock is this share of its size, half its volume
inline constexpr float k_RockHalfScale = 0.7935f;
/// Each half lies this share of the rock's radius across the ground from where it was, on opposite sides
inline constexpr float k_RockHalfSpread = k_RockHalfScale;
/// A flying rock's halves keep this share of its spin
inline constexpr float k_RockHalfSpin = 0.5f;

/// Tapping a rock to break it shows the player's creature how much fun the player is having, this much
inline constexpr float k_TapEmpathy = 0.5f;
/// Whether a rock can be broken by tapping it: only one tall enough
[[nodiscard]] bool RockBreaksWhenTapped(float height);

/// A tree that falls dead drops its roots: they are this share of the tree's model's half width, as big as the tree is
inline constexpr float k_RootsScale = 0.15f;
/// They come to rest this share of their own half width above the land
inline constexpr float k_RootsRestShare = 0.1f;
/// They fall from where the tree lies, faster and faster, this many metres times the square of their seconds
inline constexpr float k_RootsFall = 20.0f;
/// They begin to fade after this many seconds, and are gone after this many
inline constexpr float k_RootsFadeStart = 18.0f;
inline constexpr float k_RootsLife = 20.0f;
/// How fast they fade, in their share of themselves a second
inline constexpr float k_RootsFadePerSecond = 0.5f;

/// How high the falling roots are after some seconds: dropping from where they started until they reach where they rest
[[nodiscard]] float RootsHeight(float startHeight, float restHeight, float seconds);
/// How opaque the roots are, 0 to 255, once they have begun to fade; none while still whole
[[nodiscard]] std::optional<uint8_t> RootsAlpha(float seconds);
/// Whether the roots have gone
[[nodiscard]] bool RootsGone(float seconds);

/// The deed of making an artefact, which the player's creature may copy
inline constexpr uint32_t k_DeedMakeArtefact = 14;
/// How far from a building an object put down gently becomes its town's artefact
inline constexpr float k_ArtefactReach = 50.0f;
/// An artefact worth more than this impresses a town that isn't its own
inline constexpr float k_ArtefactImpressingWorth = 1.0f;
/// Whether an artefact, worth so much and belonging to a town (none for none), impresses another
[[nodiscard]] bool ArtefactWillImpress(float worth, bool ownTown);

/// How a felled tree starts to fall: away from the one who felled it, at a fifth of its height a second, turning about
/// a level axis across its fall
struct Felling
{
	glm::vec3 velocity {0.0f};
	/// Its spin about its own axes, as the game's bodies count turning
	glm::vec3 spin {0.0f};
};
inline constexpr float k_FellSpeedPerHeight = 0.2f;
inline constexpr float k_FellSpin = 0.4f;
/// Nearer than this (squared) the feller stands on the tree, and it falls with no heading
inline constexpr float k_FellLeastSquaredDistance = 0.001f;
/// A tree of a height felled from a place: the flat offset from the feller to the tree
[[nodiscard]] Felling FellingOf(float height, glm::vec2 fellerToTree);

} // namespace openblack::physics::objects
