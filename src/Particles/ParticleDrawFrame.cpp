/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleDrawFrame.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <entt/core/hashed_string.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "Graphics/ZSort.h"
#include "ParticleCreators.h"
#include "ParticleSurfaces.h"

using namespace openblack;
using namespace openblack::particles;
using namespace openblack::particles::draw;
using openblack::graphics::render_modes::Mode;

namespace
{
/// A player's glows run through 32 frames of the sprite sheet backwards, one a little faster than the other, and the
/// second turns, each by so much a millisecond
constexpr float k_GlowFrames = 32.0f;
constexpr std::array<float, 2> k_GlowFrameRates {0.02f, 0.023f};
constexpr float k_GlowSpinRate = 0.002f;
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The glows are this much bigger than the symbol, and this faint of its alpha, out of 256
constexpr float k_GlowScale = 1.5f;
constexpr uint32_t k_GlowAlpha = 100;
/// The glows' sheet and the players' symbols, four to a row
constexpr std::string_view k_GlowTexture = "S_SpriteSheet3";
constexpr std::string_view k_SymbolTexture = "ChooseSymbol";
constexpr int k_GlowCellsPerRow = 8;
constexpr int k_SymbolCellsPerRow = 4;
/// A mist's animation counts so many a millisecond up to 900, then starts again
constexpr float k_MistCountRate = 0.255f;
constexpr int k_MistCountWrap = 900;
/// A light map lies this far across the land from its atom
constexpr glm::vec3 k_LightMapOffset {10.0f, 0.0f, 10.0f};
constexpr float k_MillisecondsPerSecond = 1000.0f;
constexpr float k_ByteMax = 255.0f;
/// A model's shape comes from its axes only when they are this long
constexpr float k_MinimumAxis = 1e-4f;

uint32_t Abgr(const std::array<uint8_t, 3>& rgb, float alpha)
{
	const auto a = static_cast<uint32_t>(std::clamp(alpha, 0.0f, k_ByteMax));
	return (a << 24u) | (static_cast<uint32_t>(rgb[2]) << 16u) | (static_cast<uint32_t>(rgb[1]) << 8u) | rgb[0];
}

/// A glow's frame at an age: it starts at the first and runs backwards
int GlowCell(float milliseconds, float rate)
{
	const float run = std::fmod(milliseconds * rate, k_GlowFrames);
	const float phase = run > 0.0f ? k_GlowFrames - run : 0.0f;
	return static_cast<int>(phase);
}

/// Sprites added to what is behind without writing depth give the same picture in any order
bool Commutes(const Frame& frame, const Item& item)
{
	if (item.kind != ItemKind::Sprite)
	{
		return false;
	}
	const auto& mode = graphics::render_modes::Desc(frame.materials[frame.spriteMaterials[item.index]].mode);
	return mode.blend == graphics::render_modes::Blend::Additive && !mode.zWrite;
}

/// Each run of items that commute is gathered by material, keeping their order otherwise, so that it takes a draw for
/// each material rather than one each time the material changes
void GatherCommutingRuns(const Frame& frame, std::vector<uint32_t>& items)
{
	const auto materialOf = [&frame](uint32_t index) { return frame.spriteMaterials[frame.items[index].index]; };
	auto first = items.begin();
	while (first != items.end())
	{
		if (!Commutes(frame, frame.items[*first]))
		{
			++first;
			continue;
		}
		const auto last =
		    std::find_if(first, items.end(), [&frame](uint32_t index) { return !Commutes(frame, frame.items[index]); });
		// By the material each first comes in, so the run starts with the material it started with
		std::vector<uint32_t> order;
		for (auto it = first; it != last; ++it)
		{
			if (std::ranges::find(order, materialOf(*it)) == order.end())
			{
				order.push_back(materialOf(*it));
			}
		}
		std::stable_sort(first, last, [&](uint32_t a, uint32_t b) {
			return std::ranges::find(order, materialOf(a)) < std::ranges::find(order, materialOf(b));
		});
		first = last;
	}
}

glm::vec3 SideOf(const glm::vec3& joint, const glm::vec3& along, const glm::vec3& eye)
{
	const auto side = glm::cross(eye - joint, along);
	const float length = glm::length(side);
	return length > 0.0f ? side / length : glm::vec3(0.0f);
}

sprites::SpriteInstance FacingSprite(const glm::vec3& position, float halfWidth, float angle, int cell, int cellsPerRow,
                                     const glm::vec4& colour)
{
	const auto uv = maths::SpriteCellUv(cell, cellsPerRow);
	return {
	    .positionHalfWidth = {position, halfWidth},
	    .shape = {halfWidth, angle, 0.0f, 0.0f},
	    .uv = {uv.corner, uv.size},
	    .colour = colour,
	    .flags = glm::vec4(0.0f),
	};
}

/// Adds one item of a group, as the frame builds it
class GroupBuilder
{
public:
	GroupBuilder(Frame& frame, const Sources& sources)
	    : _frame(frame)
	    , _sources(sources)
	{
	}

	void Sprite(const sprites::SpriteInstance& instance, std::string_view texture, Mode mode)
	{
		const auto ids = _sources.textures ? _sources.textures(texture) : std::nullopt;
		if (!ids.has_value())
		{
			return;
		}
		const auto material = _frame.MaterialIndex({.texture = ids->first, .alphaTexture = ids->second, .mode = mode});
		_frame.items.push_back({.kind = ItemKind::Sprite,
		                        .index = static_cast<uint32_t>(_frame.sprites.size()),
		                        .sortPoint = glm::vec3(instance.positionHalfWidth)});
		_frame.sprites.push_back(instance);
		_frame.spriteMaterials.push_back(material);
	}

private:
	Frame& _frame;
	const Sources& _sources;
};

/// A colour as the shader takes it, from the lowest byte: red, green, blue, alpha
uint32_t AbgrOf(surface::Argb argb)
{
	return (argb & 0xFF00FF00u) | ((argb >> 16u) & 0xFFu) | ((argb & 0xFFu) << 16u);
}

/// A surface of revolution: the atom's own surface placed in the world by the atom's frame, laid over the land again
/// when asked, its colours tinted by the atom's, its texture slid on
void AddSurface(Frame& frame, const SurfaceCreator& creator, const Effect::DrawAtom& atom, const Sources& sources)
{
	const auto ids = sources.textures ? sources.textures(creator.texture) : std::nullopt;
	if (!ids.has_value() || atom.surface == nullptr || atom.surface->mesh.vertices.empty())
	{
		return;
	}
	const auto& mesh = atom.surface->mesh;
	glm::mat4 toWorld(atom.rotation);
	toWorld[0] *= atom.scale;
	toWorld[1] *= atom.scale * atom.stretch;
	toWorld[2] *= atom.scale;
	toWorld[3] = glm::vec4(atom.position, 1.0f);
	const auto land = [&sources](glm::vec3 point) {
		return sources.landHeight ? sources.landHeight({point.x, point.z}) : 0.0f;
	};
	const float landAtMiddle = creator.clampToLand ? land(atom.position) : 0.0f;
	// The atom's colour and its drawn alpha tint every point, unless it is plain white
	const auto alpha = static_cast<uint32_t>(std::clamp(atom.alpha, 0.0f, k_ByteMax));
	const surface::Argb tint =
	    (alpha << 24u) | (static_cast<uint32_t>(atom.rgb[0]) << 16u) | (static_cast<uint32_t>(atom.rgb[1]) << 8u) | atom.rgb[2];
	const auto uvOffset = SurfaceUvOffset(*atom.surface, creator, atom.fraction);
	const auto mode = creator.additive
	                      ? (creator.writeDepth ? Mode::AlphaTexturedAlphaAdditive : Mode::AlphaTexturedAlphaAdditiveNz)
	                      : (creator.writeDepth ? Mode::AlphaTexturedAlpha : Mode::AlphaTexturedAlphaNz);
	const auto material = frame.MaterialIndex({.texture = ids->first, .alphaTexture = ids->second, .mode = mode});
	const auto firstVertex = static_cast<uint32_t>(frame.surfaceVertices.size());
	const bool lit = creator.lit && mesh.normals.size() == mesh.vertices.size();
	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		const auto& vertex = mesh.vertices[i];
		auto position = glm::vec3(toWorld * glm::vec4(vertex.position, 1.0f));
		if (creator.clampToLand)
		{
			position.y += land(position) - landAtMiddle;
		}
		const auto colour = tint != 0xFFFFFFFFu ? surface::Modulate(vertex.colour, tint) : vertex.colour;
		frame.surfaceVertices.push_back({
		    .position = position,
		    .uv = vertex.uv + uvOffset,
		    .abgr = AbgrOf(colour),
		    .specularAbgr = AbgrOf(vertex.specular),
		});
		frame.surfaceNormals.push_back(lit ? mesh.normals[i] : glm::vec3(0.0f));
	}
	const auto firstIndex = static_cast<uint32_t>(frame.surfaceIndices.size());
	frame.surfaceIndices.insert(frame.surfaceIndices.end(), mesh.indices.begin(), mesh.indices.end());
	frame.items.push_back(
	    {.kind = ItemKind::Surface, .index = static_cast<uint32_t>(frame.surfaces.size()), .sortPoint = atom.position});
	frame.surfaces.push_back({
	    .material = material,
	    .firstVertex = firstVertex,
	    .vertexCount = static_cast<uint32_t>(mesh.vertices.size()),
	    .firstIndex = firstIndex,
	    .indexCount = static_cast<uint32_t>(mesh.indices.size()),
	    .doubleSided = creator.doubleSided,
	    .lit = lit,
	    .worldToAtom = glm::inverse(toWorld),
	});
}

void AddAtom(Frame& frame, GroupBuilder& builder, const Effect::DrawAtom& atom, int player, const Sources& sources,
             const glm::vec3& origin)
{
	const auto& creator = *atom.creator;
	switch (creator.kind)
	{
	case Creator::Kind::Sprite:
		builder.Sprite(sprites::InstanceOf(atom), creator.texture, sprites::RenderMode(creator));
		break;
	case Creator::Kind::Symbol:
	{
		const uint32_t rgb = sources.playerColour ? sources.playerColour(player) : 0xFFFFFFu;
		const auto symbol = SymbolOf(atom, rgb, player);
		// Added to what is behind, writing no depth
		const auto additive = Mode::AlphaTexturedAlphaAdditiveNz;
		builder.Sprite(symbol.instances[0], k_GlowTexture, additive);
		builder.Sprite(symbol.instances[1], k_GlowTexture, additive);
		builder.Sprite(symbol.instances[2], k_SymbolTexture, additive);
		break;
	}
	case Creator::Kind::Mesh:
	{
		const auto& mesh = static_cast<const MeshCreator&>(creator);
		if (!mesh.mesh.has_value())
		{
			break;
		}
		glm::mat3 axes = atom.rotation * atom.scale;
		axes[1] *= atom.stretch;
		glm::vec2 uv(0.0f);
		if (mesh.type == MeshCreator::Type::AnimTextured)
		{
			uv = mesh.UvOffset(maths::FrameIndex(atom.frame, mesh.FramesPerAtom(), mesh.loopAnim));
		}
		frame.items.push_back(
		    {.kind = ItemKind::Mesh, .index = static_cast<uint32_t>(frame.meshes.size()), .sortPoint = atom.position});
		frame.meshes.push_back({
		    .creator = &creator,
		    .mesh = *mesh.mesh,
		    .axes = axes,
		    .position = atom.position,
		    .colour = glm::vec4(glm::vec3(atom.rgb[0], atom.rgb[1], atom.rgb[2]) / k_ByteMax,
		                        std::clamp(atom.alpha / k_ByteMax, 0.0f, 1.0f)),
		    .uvOffset = uv,
		    .cutBelow = mesh.drawCutByPlane ? std::optional<float>(origin.y) : std::nullopt,
		});
		break;
	}
	case Creator::Kind::Mist:
	{
		const auto& mist = static_cast<const MistCreator&>(creator);
		float shape = atom.creatorValue.x;
		// The shape may come from the atom's stretch, as a storm's clouds gather
		const glm::mat3 matrix =
		    atom.rotation * glm::mat3(atom.scale, 0.0f, 0.0f, 0.0f, atom.scale * atom.stretch, 0.0f, 0.0f, 0.0f, atom.scale);
		if (mist.ratioFromMatrix && std::abs(matrix[0][0]) > k_MinimumAxis)
		{
			shape = matrix[1][1] / matrix[0][0];
		}
		const auto counted = static_cast<int>(atom.age * k_MillisecondsPerSecond * k_MistCountRate);
		const auto alpha = static_cast<uint32_t>(std::clamp(atom.alpha, 0.0f, k_ByteMax));
		frame.items.push_back(
		    {.kind = ItemKind::Mist, .index = static_cast<uint32_t>(frame.mists.size()), .sortPoint = atom.position});
		frame.mists.push_back({
		    .position = atom.position,
		    .size = atom.scale,
		    .argb = (alpha << 24u) | (static_cast<uint32_t>(atom.rgb[0]) << 16u) | (static_cast<uint32_t>(atom.rgb[1]) << 8u) |
		            atom.rgb[2],
		    .shape = shape,
		    .counter = (static_cast<int>(atom.creatorValue.y) + counted) % k_MistCountWrap,
		});
		// A storm's cloud shades the land under it, as strongly as it is opaque
		if (mist.shadow.has_value())
		{
			frame.lightStamps.push_back({
			    .bitmap = *mist.shadow,
			    .frame = 0,
			    .pitch = mist.shadowPitch,
			    .centre = atom.position,
			    .strength = static_cast<float>(alpha) / k_ByteMax,
			    .shadow = true,
			});
		}
		break;
	}
	case Creator::Kind::LightMap:
	{
		const auto& light = static_cast<const LightMapCreator&>(creator);
		if (!light.bitmap.has_value())
		{
			break;
		}
		glm::vec3 position = atom.position;
		// It moves about every frame when asked to: the first number across z, the next up, the last across x
		if (light.useJitter && sources.random)
		{
			const float first = sources.random(light.jitter);
			const float second = sources.random(light.jitter);
			const float third = sources.random(light.jitter);
			position += glm::vec3(third, second, first);
		}
		frame.lightStamps.push_back({
		    .bitmap = *light.bitmap,
		    .frame = maths::FrameIndex(atom.frame, light.numFrames, light.loopAnim) & 0xFF,
		    .pitch = light.pitch,
		    .centre = position + k_LightMapOffset,
		    .strength = std::trunc(std::clamp(atom.alpha, 0.0f, k_ByteMax)) / k_ByteMax,
		});
		break;
	}
	case Creator::Kind::Surface:
		AddSurface(frame, static_cast<const SurfaceCreator&>(creator), atom, sources);
		break;
	case Creator::Kind::Fragment:
		if (atom.fragment != nullptr)
		{
			frame.items.push_back({.kind = ItemKind::Fragment,
			                       .index = static_cast<uint32_t>(frame.fragments.size()),
			                       .sortPoint = atom.position});
			frame.fragments.push_back({
			    .shape = atom.fragment,
			    .axes = atom.rotation * atom.scale,
			    .position = atom.position,
			    .rgb = atom.rgb,
			    .alpha = std::clamp(atom.alpha / k_ByteMax, 0.0f, 1.0f),
			});
		}
		break;
	case Creator::Kind::Point:
	case Creator::Kind::Chain:
	case Creator::Kind::Other:
		break;
	}
}
} // namespace

void Frame::Clear()
{
	materials.clear();
	sprites.clear();
	spriteMaterials.clear();
	chainVertices.clear();
	chains.clear();
	meshes.clear();
	mists.clear();
	surfaceVertices.clear();
	surfaceIndices.clear();
	surfaceNormals.clear();
	surfaces.clear();
	fragments.clear();
	lightStamps.clear();
	items.clear();
	groups.clear();
}

uint32_t Frame::MaterialIndex(const Material& material)
{
	const auto found = std::ranges::find(materials, material);
	if (found != materials.end())
	{
		return static_cast<uint32_t>(std::distance(materials.begin(), found));
	}
	materials.push_back(material);
	return static_cast<uint32_t>(materials.size() - 1);
}

SymbolSprites draw::SymbolOf(const Effect::DrawAtom& atom, uint32_t playerRgb, int player)
{
	// The symbol takes the player's colour times the atom's; the glows are a faint copy, the second white
	const auto channel = [&](int shift, uint8_t value) {
		return static_cast<float>((((playerRgb >> static_cast<uint32_t>(shift)) & 0xFFu) * value) >> 8u);
	};
	const auto atomAlpha = static_cast<uint32_t>(std::clamp(atom.alpha, 0.0f, k_ByteMax));
	const uint32_t symbolAlpha = (atomAlpha * 0xFFu) >> 8u;
	const uint32_t glowAlpha = (symbolAlpha * k_GlowAlpha) >> 8u;
	const glm::vec3 rgb = glm::vec3(channel(16, atom.rgb[0]), channel(8, atom.rgb[1]), channel(0, atom.rgb[2])) / k_ByteMax;
	const float milliseconds = atom.age * k_MillisecondsPerSecond;
	const float spin = std::fmod(milliseconds * k_GlowSpinRate, k_TwoPi);
	const float glow = static_cast<float>(glowAlpha) / k_ByteMax;
	const float scale = std::max(atom.scale, sprites::k_MinimumHalfWidth);
	return {{
	    FacingSprite(atom.position, scale * k_GlowScale, 0.0f, GlowCell(milliseconds, k_GlowFrameRates[0]), k_GlowCellsPerRow,
	                 glm::vec4(rgb, glow)),
	    FacingSprite(atom.position, scale * k_GlowScale, spin, GlowCell(milliseconds, k_GlowFrameRates[1]), k_GlowCellsPerRow,
	                 glm::vec4(1.0f, 1.0f, 1.0f, glow)),
	    FacingSprite(atom.position, scale, 0.0f, std::max(player, 0), k_SymbolCellsPerRow,
	                 glm::vec4(rgb, static_cast<float>(symbolAlpha) / k_ByteMax)),
	}};
}

void draw::AppendChain(std::vector<ChainVertex>& out, const Creator& creator, std::span<const Effect::DrawAtom> joints,
                       int textureRepeats, float textureScroll)
{
	if (joints.size() < 2)
	{
		return;
	}
	const auto& chain = static_cast<const ChainCreator&>(creator);
	const auto segments = static_cast<int>(joints.size()) - 1;
	for (int i = 0; i < segments; ++i)
	{
		const auto& head = joints[static_cast<size_t>(i)];
		const auto& tail = joints[static_cast<size_t>(i) + 1];
		const auto uv = chain.SegmentUv(i, segments, textureRepeats, textureScroll);
		const bool before = i > 0;
		const bool after = i + 1 < segments;
		const auto& previous = before ? joints[static_cast<size_t>(i) - 1].position : head.position;
		const auto& next = after ? joints[static_cast<size_t>(i) + 2].position : tail.position;
		const uint32_t headColour = Abgr(head.rgb, head.alpha);
		const uint32_t tailColour = Abgr(tail.rgb, tail.alpha);
		// The first joint's two sides, then the next joint's
		const auto corner = [&](const Effect::DrawAtom& joint, const glm::vec3& other, const glm::vec3& adjacent, float side,
		                        float end, bool neighbour, glm::vec2 cornerUv, uint32_t colour) {
			out.push_back({
			    .joint = glm::vec4(joint.position, side * joint.scale),
			    .other = other,
			    .adjacent = adjacent,
			    .uv = cornerUv,
			    .abgr = colour,
			    .flags = {end, neighbour ? 1.0f : 0.0f},
			});
		};
		corner(head, tail.position, previous, 1.0f, 1.0f, before, uv[0], headColour);
		corner(head, tail.position, previous, -1.0f, 1.0f, before, uv[1], headColour);
		corner(tail, head.position, next, 1.0f, -1.0f, after, uv[2], tailColour);
		corner(tail, head.position, next, -1.0f, -1.0f, after, uv[3], tailColour);
	}
}

glm::vec3 draw::ChainCorner(const ChainVertex& vertex, const glm::vec3& eye)
{
	const glm::vec3 joint(vertex.joint);
	// Along the segment from its first joint to its next, and along the neighbouring one the same way
	auto side = SideOf(joint, (vertex.other - joint) * vertex.flags.x, eye);
	if (vertex.flags.y > 0.5f)
	{
		side = (side + SideOf(joint, (joint - vertex.adjacent) * vertex.flags.x, eye)) * 0.5f;
	}
	return joint + side * vertex.joint.w;
}

void draw::AddEffect(Frame& frame, const Effect::DrawWalk& walk, DrawPath path, const glm::vec3& origin, int player,
                     const Sources& sources)
{
	const auto firstItem = static_cast<uint32_t>(frame.items.size());
	const int shown = player >= 0 ? player : k_NeutralPlayer;
	GroupBuilder builder(frame, sources);
	for (const auto& step : walk.steps)
	{
		if (!step.chain)
		{
			AddAtom(frame, builder, walk.atoms.at(step.index), shown, sources, origin);
			continue;
		}
		const auto& chain = walk.chains.at(step.index);
		const auto ids = sources.textures ? sources.textures(chain.creator->texture) : std::nullopt;
		if (!ids.has_value())
		{
			continue;
		}
		const auto joints = std::span(walk.joints).subspan(chain.firstJoint, chain.jointCount);
		const auto material = frame.MaterialIndex(
		    {.texture = ids->first, .alphaTexture = ids->second, .mode = sprites::RenderMode(*chain.creator)});
		// It takes its place by its middle joint
		frame.items.push_back({.kind = ItemKind::Chain,
		                       .index = static_cast<uint32_t>(frame.chains.size()),
		                       .sortPoint = joints[joints.size() / 2].position});
		frame.chains.push_back({.material = material,
		                        .firstVertex = static_cast<uint32_t>(frame.chainVertices.size()),
		                        .segments = chain.jointCount - 1});
		AppendChain(frame.chainVertices, *chain.creator, joints, chain.textureRepeats, chain.textureScroll);
	}
	const auto count = static_cast<uint32_t>(frame.items.size()) - firstItem;
	if (count > 0)
	{
		frame.groups.push_back({.path = path, .origin = origin, .firstItem = firstItem, .itemCount = count});
	}
}

void draw::AddLightSheet(Frame& frame, std::span<const LightSheet::Vertex> vertices, std::span<const uint32_t> triangles,
                         const glm::vec3& sortPoint)
{
	if (vertices.empty() || triangles.empty())
	{
		return;
	}
	static constexpr auto k_Stars = entt::hashed_string("raw/S_LightSheetStars");
	static constexpr auto k_StarsAlpha = entt::hashed_string("raw/S_LightSheetStarsa");
	const auto material = frame.MaterialIndex(
	    {.texture = k_Stars.value(), .alphaTexture = k_StarsAlpha.value(), .mode = Mode::AlphaTexturedAlphaAdditiveNz});
	const auto firstItem = static_cast<uint32_t>(frame.items.size());
	const auto firstVertex = static_cast<uint32_t>(frame.surfaceVertices.size());
	for (const auto& vertex : vertices)
	{
		frame.surfaceVertices.push_back({.position = vertex.position,
		                                 .uv = vertex.uv,
		                                 .abgr = AbgrOf(vertex.argb),
		                                 .specularAbgr = AbgrOf(vertex.specularArgb)});
		frame.surfaceNormals.emplace_back(0.0f);
	}
	const auto firstIndex = static_cast<uint32_t>(frame.surfaceIndices.size());
	frame.surfaceIndices.insert(frame.surfaceIndices.end(), triangles.begin(), triangles.end());
	frame.items.push_back(
	    {.kind = ItemKind::Surface, .index = static_cast<uint32_t>(frame.surfaces.size()), .sortPoint = sortPoint});
	frame.surfaces.push_back({
	    .material = material,
	    .firstVertex = firstVertex,
	    .vertexCount = static_cast<uint32_t>(vertices.size()),
	    .firstIndex = firstIndex,
	    .indexCount = static_cast<uint32_t>(triangles.size()),
	    .doubleSided = true,
	    .lit = false,
	    .worldToAtom = glm::mat4(1.0f),
	});
	frame.groups.push_back({.path = DrawPath::Sorted, .origin = sortPoint, .firstItem = firstItem, .itemCount = 1});
}

void draw::Order(const Frame& frame, const glm::vec3& camera, std::optional<glm::vec3> hand, std::vector<Command>& commands,
                 std::vector<uint32_t>& spriteOrder)
{
	commands.clear();
	spriteOrder.clear();
	// Items in turn: runs of sprites that share a material join one command, and each command takes a place a little
	// nearer than the one before, so the things of one place keep their order
	const auto emit = [&](std::span<const uint32_t> items, auto depthOf) {
		const auto firstCommand = commands.size();
		for (const auto index : items)
		{
			const auto& item = frame.items[index];
			const uint32_t material = item.kind == ItemKind::Sprite    ? frame.spriteMaterials[item.index]
			                          : item.kind == ItemKind::Chain   ? frame.chains[item.index].material
			                          : item.kind == ItemKind::Surface ? frame.surfaces[item.index].material
			                                                           : 0;
			if (item.kind == ItemKind::Sprite && commands.size() > firstCommand && commands.back().kind == ItemKind::Sprite &&
			    commands.back().material == material)
			{
				++commands.back().count;
				spriteOrder.push_back(item.index);
				continue;
			}
			uint32_t depth = depthOf(item);
			if (commands.size() > firstCommand)
			{
				depth = std::min(depth, commands.back().depth > 0 ? commands.back().depth - 1 : 0);
			}
			if (item.kind == ItemKind::Sprite)
			{
				commands.push_back({.kind = ItemKind::Sprite,
				                    .first = static_cast<uint32_t>(spriteOrder.size()),
				                    .count = 1,
				                    .material = material,
				                    .depth = depth});
				spriteOrder.push_back(item.index);
			}
			else
			{
				commands.push_back({.kind = item.kind, .first = item.index, .count = 1, .material = material, .depth = depth});
			}
		}
	};

	// The sorted effects' things all together, the farthest first
	struct Keyed
	{
		float key;
		uint32_t item;
	};
	std::vector<Keyed> sorted;
	std::vector<uint32_t> items;
	for (const auto& group : frame.groups)
	{
		if (group.path != DrawPath::Sorted)
		{
			continue;
		}
		for (uint32_t i = group.firstItem; i < group.firstItem + group.itemCount; ++i)
		{
			sorted.push_back({graphics::zsort::Key(frame.items[i].sortPoint, camera), i});
		}
	}
	std::ranges::stable_sort(sorted, std::greater {}, &Keyed::key);
	items.reserve(sorted.size());
	std::ranges::transform(sorted, std::back_inserter(items), &Keyed::item);
	GatherCommutingRuns(frame, items);
	emit(items, [&camera](const Item& item) { return graphics::zsort::Depth(item.sortPoint, camera); });

	// Each of the others at one place, in its own order
	for (const auto& group : frame.groups)
	{
		if (group.path == DrawPath::Sorted)
		{
			continue;
		}
		items.clear();
		for (uint32_t i = group.firstItem; i < group.firstItem + group.itemCount; ++i)
		{
			items.push_back(i);
		}
		GatherCommutingRuns(frame, items);
		// Just after the hand, which draws itself at its own place
		const bool afterHand = group.path == DrawPath::Immediate && hand.has_value();
		const auto base = graphics::zsort::Depth(afterHand ? *hand : group.origin, camera);
		const uint32_t depth = afterHand && base > 0 ? base - 1 : base;
		emit(items, [depth](const Item&) { return depth; });
	}
}
