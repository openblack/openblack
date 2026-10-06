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
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <entt/core/fwd.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Graphics/RenderModes.h"
#include "ParticleDrawPath.h"
#include "ParticleEffect.h"
#include "ParticleSprites.h"

/// What the particle effects draw in a frame, gathered once whatever the camera, and the order a camera draws it in,
/// each effect in one of the three ways of DrawPath
namespace openblack::particles::draw
{

/// The sheets a sprite or a ribbon is drawn from and how it blends: the things that share one can share a draw
struct Material
{
	entt::id_type texture;
	entt::id_type alphaTexture;
	graphics::render_modes::Mode mode;

	[[nodiscard]] bool operator==(const Material& other) const = default;
};

/// One corner of a ribbon. Where it lies depends on where the camera is, so the vertex shader works it out: the joint
/// moved to one side of the segment as seen from the camera, by the joint's half width. Where two segments meet, their
/// corners there meet half way.
struct ChainVertex
{
	/// The joint, and its half width signed by the side the corner is on
	glm::vec4 joint;
	/// The segment's other joint
	glm::vec3 other;
	/// The other joint of the neighbouring segment that shares this joint; the joint itself when there is none
	glm::vec3 adjacent;
	glm::vec2 uv;
	/// Its colour and alpha, as bytes from the lowest: red, green, blue, alpha
	uint32_t abgr;
	/// x: 1 at the segment's first joint, -1 at its next. y: 1 when a neighbouring segment shares the joint.
	glm::vec2 flags;
};
static_assert(sizeof(ChainVertex) == 60);

/// The corners of a ribbon, in its chain's vertices: four to each segment
struct ChainDraw
{
	uint32_t material;
	uint32_t firstVertex;
	uint32_t segments;
};

/// A model drawn for an atom
struct MeshDraw
{
	/// What the model is and how it is drawn (a MeshCreator)
	const Creator* creator;
	entt::id_type mesh;
	/// Its axes, each scaled, and where it is. A model that faces the camera is turned to it where it is drawn.
	glm::mat3 axes;
	glm::vec3 position;
	/// Its colour and alpha, 0..1
	glm::vec4 colour;
	/// How far its texture has moved on
	glm::vec2 uvOffset;
};

/// A puff of mist drawn for an atom
struct MistDraw
{
	glm::vec3 position;
	float size;
	/// The atom's colour, 0xAARRGGBB, which the land's light tints where it is drawn
	uint32_t argb;
	/// How much wider than tall it is seen level
	float shape;
	/// Its animation's counter
	int counter;
};

/// A light map frame stamped on the land's colours, centred on a point
struct LightStamp
{
	entt::id_type bitmap;
	int frame;
	int pitch;
	glm::vec3 centre;
	/// 0..1
	float strength;
};

enum class ItemKind : uint8_t
{
	Sprite,
	Chain,
	Mesh,
	Mist,
};

/// One thing drawn, by its index in the frame's list of its kind
struct Item
{
	ItemKind kind;
	uint32_t index;
	/// Where it takes its place among what blends when its effect is sorted
	glm::vec3 sortPoint;
};

/// An effect's items, in the order the game draws it all at once
struct Group
{
	DrawPath path;
	/// Where a queued effect takes its place
	glm::vec3 origin;
	uint32_t firstItem;
	uint32_t itemCount;
};

struct Frame
{
	std::vector<Material> materials;
	std::vector<sprites::SpriteInstance> sprites;
	/// Each sprite's material
	std::vector<uint32_t> spriteMaterials;
	std::vector<ChainVertex> chainVertices;
	std::vector<ChainDraw> chains;
	std::vector<MeshDraw> meshes;
	std::vector<MistDraw> mists;
	std::vector<LightStamp> lightStamps;
	std::vector<Item> items;
	std::vector<Group> groups;

	void Clear();
	[[nodiscard]] bool Empty() const { return items.empty() && lightStamps.empty(); }
	/// The index of a material, added when it is new
	uint32_t MaterialIndex(const Material& material);
};

/// What building a frame needs from the game
struct Sources
{
	/// A sheet and its alpha by the name a creator spells it with; none when there is no such sheet
	std::function<std::optional<std::pair<entt::id_type, entt::id_type>>(std::string_view)> textures;
	/// A player's colour, 0xRRGGBB
	std::function<uint32_t(int)> playerColour;
	/// A random number up to a value, drawn again each frame for the light maps that move about
	std::function<float(float)> random;
};

/// The player whose symbol and colour an effect without one shows
constexpr int k_NeutralPlayer = 7;

/// Adds an effect's walk to the frame, drawn in one of the three ways, its symbols those of a player
void AddEffect(Frame& frame, const Effect::DrawWalk& walk, DrawPath path, const glm::vec3& origin, int player,
               const Sources& sources);

/// The three sprites of a player's symbol, in the order drawn: a still glow, a turning glow, then the symbol. Their
/// materials are the glow's for the first two and the symbol's for the third.
struct SymbolSprites
{
	std::array<sprites::SpriteInstance, 3> instances;
};
[[nodiscard]] SymbolSprites SymbolOf(const Effect::DrawAtom& atom, uint32_t playerRgb, int player);

/// The corners of a ribbon through its joints, four to a segment, its texture coordinates from its creator
void AppendChain(std::vector<ChainVertex>& out, const Creator& creator, std::span<const Effect::DrawAtom> joints,
                 int textureRepeats = -1);
/// Where a ribbon's corner lies seen from a point, as the vertex shader places it
[[nodiscard]] glm::vec3 ChainCorner(const ChainVertex& vertex, const glm::vec3& eye);

/// One draw: a run of sprites that share a material, or one ribbon, model or mist
struct Command
{
	ItemKind kind;
	/// Sprites: a run of the pass's sprite order. The others: the index of their kind.
	uint32_t first;
	uint32_t count;
	/// Sprites and ribbons: their material
	uint32_t material;
	/// Its place among what blends: the greatest first
	uint32_t depth;
};

/// The order a camera draws the frame in, with the sprites, in the order the commands draw them, as indices of the
/// frame's sprites. A sorted effect's things each go by their distance from the camera; runs of sprites next to each
/// other in that order that share a material are drawn together. A queued effect goes by its origin, and one drawn in
/// the hand just after the hand, when there is one, else by its origin. The things of one place are drawn in turn.
/// Sprites added to what is behind that write no depth give the same picture in any order, so each run of them is drawn
/// a material at a time.
void Order(const Frame& frame, const glm::vec3& camera, std::optional<glm::vec3> hand, std::vector<Command>& commands,
           std::vector<uint32_t>& spriteOrder);

} // namespace openblack::particles::draw
