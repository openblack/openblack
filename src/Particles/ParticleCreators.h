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
#include <optional>
#include <string>
#include <string_view>

#include <entt/core/fwd.hpp>
#include <glm/vec2.hpp>

#include "ParticleEffect.h"

/// The creators of the atoms drawn as something other than a sprite: models, ribbons, mists, light stamped on the land
/// and the casting player's symbol
namespace openblack::particles
{
class ParticleClassRegistry;

/// What these creators need from the game's resources. The game loads the models and light maps through its resource
/// caches; tests pass none, and the creators then have nothing to draw.
class CreatorResourcesInterface
{
public:
	CreatorResourcesInterface() = default;
	CreatorResourcesInterface(const CreatorResourcesInterface&) = delete;
	CreatorResourcesInterface& operator=(const CreatorResourcesInterface&) = delete;
	CreatorResourcesInterface(CreatorResourcesInterface&&) = delete;
	CreatorResourcesInterface& operator=(CreatorResourcesInterface&&) = delete;
	virtual ~CreatorResourcesInterface() = default;

	/// A model of the game's list by the name the files give it (such as "MSH_S_BLAST_CENTRE"); none for a name not in
	/// the list
	[[nodiscard]] virtual std::optional<entt::id_type> MeshByName(std::string_view name) = 0;
	/// A model file by the path the files give it (such as ".\Data\Spells\Meshes\Vortex.l3d"); none when it can't be read
	[[nodiscard]] virtual std::optional<entt::id_type> MeshByFile(std::string_view path) = 0;
	/// A light map by its path, frames of pitch by pitch texels of `channels` bytes; none when it can't be read or isn't
	/// that size
	[[nodiscard]] virtual std::optional<entt::id_type> LightMap(std::string_view path, int pitch, int channels,
	                                                            int framesInFile, int framesInUse) = 0;
};

/// A model for each atom, turned and scaled with it. The plain kind, one whose texture plays frames or slides, and one
/// that plays an animation.
struct MeshCreator final: Creator
{
	enum class Type : uint8_t
	{
		Plain,
		AnimTextured,
		Animated,
	};
	Type type {Type::Plain};
	/// None when the files name no model the game has
	std::optional<entt::id_type> mesh;
	/// Turned about its own vertical to face the camera, and its height stretched
	bool faceCamera {false};
	float heightStretch {1.0f};
	/// The model's own materials are replaced: added to what is behind or blended over it, writing depth or not, two
	/// sided or not. Without it the model is drawn as its materials have it.
	bool changeMaterial {true};
	bool additiveMaterial {false};
	bool writeDepthMaterial {false};
	bool doubleSided {true};
	/// Drawn in the atom's alpha; without it only the parts the model's materials blend show it
	bool useGlobalAlpha {false};
	/// Coloured by the land's light where it is
	bool landscapeColour {false};

	// A texture that plays frames or slides
	int textureWidth {64};
	int textureHeight {64};
	bool slideU {false};
	bool slideV {false};
	bool randomiseFrameRate {false};
	float frameRateMax {10.0f};
	float initialOffsetFraction {0.0f};
	float stretchY {1.0f};

	/// The frames a sliding texture or an animation runs over
	static constexpr int k_SlideFrames = 1000;
	[[nodiscard]] int FramesPerAtom() const { return slideU || slideV || type == Type::Animated ? k_SlideFrames : numFrames; }
	/// How far the texture has moved on at a frame: whole cells of the sheet in rows, or a slide across it
	[[nodiscard]] glm::vec2 UvOffset(int frame) const;

	void InitAtom(Effect& effect, Atom& atom) const override;
};

/// One joint of a ribbon for each atom: the collection's joints are drawn as one strip facing the camera, as wide as
/// each joint's scale, with the sheet's frames repeated along it
struct ChainCreator final: Creator
{
	int frameOfHead {0};
	int frameOfTail {0};
	/// The repeats of the frame along the whole ribbon, -1 for one a segment
	int texturesForWholeChain {-1};
	/// Texels across the ribbon and along one repeat of a 256 texel sheet
	int frameWidth {32};
	int frameHeight {64};

	/// The texture coordinates of a segment's four corners: its first joint's two sides, then its next joint's, the
	/// first side of each ahead. U runs across the ribbon, V along it.
	/// A ribbon may repeat its frame a number of times of its own in place of the creator's (-1 for the creator's).
	[[nodiscard]] std::array<glm::vec2, 4> SegmentUv(int segment, int segments, int repeatsOverride = -1) const;
};

/// A puff of mist for each atom, the mist mesh facing the camera and shrinking edge on
struct MistCreator final: Creator
{
	/// How much wider than tall it is seen level, none to give each mist a random shape
	float ratio {0.0f};
	/// Its shape comes from its atom's stretch
	bool ratioFromMatrix {false};
	float initialScaleMin {1.0f};

	/// The atom's creator values: its shape and where its animation starts
	void InitAtom(Effect& effect, Atom& atom) const override;
};

/// A light stamped on the land's colours under each atom, from frames of a light map the atom plays through
struct LightMapCreator final: Creator
{
	std::optional<entt::id_type> bitmap;
	int pitch {1};
	/// It moves about by up to this much each way every frame, when asked to
	bool useJitter {false};
	float jitter {0.0f};

	void InitAtom(Effect& effect, Atom& atom) const override;
};

/// The casting player's symbol for each atom, between two glows of the sprite sheet in the player's colour: one still,
/// one turning, both running through the sheet's frames
struct SymbolCreator final: Creator
{
};

/// The creators of models, ribbons, mists, light maps and symbols. Without resources the models and light maps have
/// nothing to draw.
void RegisterDrawnCreators(ParticleClassRegistry& registry, CreatorResourcesInterface* resources);

} // namespace openblack::particles
