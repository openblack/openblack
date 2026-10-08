/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "BuildingDamageSystem.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/BuildingDamage.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/Registry.h"
#include "ECS/SnowDust.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicWorldInterface.h"
#include "Magic/SpellRules.h"
#include "Physics/Body.h"
#include "Physics/BodyShapes.h"
#include "Physics/LivingRules.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace damage = openblack::physics::damage;

namespace
{
/// The bank the knocks and crashes of buildings play from
constexpr std::string_view k_BuildingSoundBank = "editor.sad";
/// The sound keys of a light knock, a heavier knock and a building breaking: the level of the sound, then its codes
constexpr std::array<int32_t, 5> k_KnockKeys {3, 0, 0x16, 0x10, 75};
constexpr std::array<int32_t, 5> k_HardKnockKeys {2, 0, 0x16, 0x10, 75};
constexpr std::array<int32_t, 5> k_CrashKeys {1, 0, 0x16, 9, 75};
/// A creature's blow on a building
constexpr std::array<int32_t, 5> k_SmashKeys {2, 1, 1, 9, 75};
/// The game's crush, which a breaking blow applies
constexpr size_t k_CrushPreset = 3;
/// How far behind each triangle its back face is drawn
constexpr float k_BackFace = 0.45f;
/// A creature's blow breaks a building this far round where it struck, for each of its size
constexpr float k_CreatureBlowReach = 3.75f;
/// The dust a piece sheds at each of its corners as it starts to fly
constexpr uint32_t k_PieceDustArgb = 0x80706050;
constexpr float k_PieceDustSize = 2.0f;
constexpr float k_PieceDustSpeed = 0.02f;
/// The random spread of a piece's spin and its dust, drawn from 0 to 200 about 100
constexpr uint32_t k_SpreadDraws = 201;
constexpr float k_SpreadMiddle = 100.0f;

Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

const LandIslandInterface* Land()
{
	return Locator::terrainSystem::has_value() ? &Locator::terrainSystem::value() : nullptr;
}

float LandHeight(glm::vec2 point)
{
	const auto* land = Land();
	return land != nullptr ? land->GetHeightAt(point) : 0.0f;
}

const graphics::L3DMesh* MeshOf(entt::id_type id)
{
	if (!Locator::resources::has_value() || id == 0)
	{
		return nullptr;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(id) ? &*meshes.Handle(id) : nullptr;
}

uint32_t GameRand(uint32_t n)
{
	return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameRand(n) : 0;
}

/// A random spread about nothing, scaled
float Spread(float scale)
{
	return (static_cast<float>(GameRand(k_SpreadDraws)) - k_SpreadMiddle) * scale;
}

/// Three spreads drawn z first, as the game draws them
glm::vec3 SpreadVector(float scale)
{
	const float z = Spread(scale);
	const float y = Spread(scale);
	const float x = Spread(scale);
	return {x, y, z};
}

void PlaySound(entt::entity building, std::span<const int32_t> keys)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	const auto* transform = Entities().TryGet<const Transform>(building);
	Locator::audio::value().PlayAnimEffect(std::string(k_BuildingSoundBank), keys, building,
	                                       transform != nullptr ? transform->position : glm::vec3(0.0f));
}

/// Whether a building is built
bool IsBuilt(entt::entity building)
{
	const auto* progress = Entities().TryGet<const BuildProgress>(building);
	return progress == nullptr || progress->built >= 1.0f;
}

/// How much of a building is drawn standing
float DrawShare(entt::entity building)
{
	const auto& registry = Entities();
	const auto* progress = registry.TryGet<const BuildProgress>(building);
	// Drawn by its repair only while it is broken and has a site for the repair
	const auto* broken = registry.TryGet<const BuildingDamage>(building);
	const auto* site = registry.TryGet<const RepairSite>(building);
	return damage::DrawShare(world_objects::LifeOf(building), progress != nullptr ? progress->built : 1.0f,
	                         broken != nullptr && site != nullptr ? std::optional(site->startLife) : std::nullopt);
}

/// The building's model, the drawn parts of its nearest level of detail, in the world
damage::Mesh MakeDamageMesh(entt::entity building)
{
	damage::Mesh mesh;
	const auto& registry = Entities();
	const auto* meshComponent = registry.TryGet<const Mesh>(building);
	const auto* transform = registry.TryGet<const Transform>(building);
	const auto* model = meshComponent != nullptr ? MeshOf(meshComponent->id) : nullptr;
	if (model == nullptr || transform == nullptr)
	{
		return mesh;
	}
	const auto world = [transform](glm::vec3 p) { return transform->position + transform->rotation * (p * transform->scale); };
	// A model lying over the land's shape is lowered with it, the land under each corner against the land under its origin
	const bool followsLand = registry.AllOf<MorphWithTerrain>(building);
	const float baseHeight = LandHeight(glm::vec2(transform->position.x, transform->position.z));
	const auto& subMeshes = model->GetSubMeshes();
	for (uint32_t s = 0; s < subMeshes.size(); ++s)
	{
		const auto& subMesh = *subMeshes[s];
		const auto flags = subMesh.GetFlags();
		if ((flags.lodMask & 1U) == 0 || flags.status != 0)
		{
			continue;
		}
		const auto& surface = subMesh.GetSurface();
		const auto& primitives = subMesh.GetPrimitives();
		for (uint32_t p = 0; p < primitives.size(); ++p)
		{
			damage::Primitive primitive {.material = (s << 16U) | p};
			const auto& source = primitives[p];
			for (uint32_t i = 0; i + 2 < source.indicesCount; i += 3)
			{
				std::array<damage::Corner, 3> corners;
				bool inside = true;
				for (uint32_t c = 0; c < 3; ++c)
				{
					const auto index = source.indicesOffset + i + c;
					if (index >= surface.indices.size() || surface.indices[index] >= surface.positions.size())
					{
						inside = false;
						break;
					}
					const auto vertex = surface.indices[index];
					auto position = world(surface.positions[vertex]);
					if (followsLand)
					{
						position.y -= baseHeight - LandHeight(glm::vec2(position.x, position.z));
					}
					corners.at(c) = {.position = position, .uv = surface.uvs.at(vertex)};
				}
				if (inside)
				{
					primitive.triangles.push_back(damage::MakeTriangle(corners));
				}
			}
			damage::LinkNeighbours(primitive);
			mesh.primitives.push_back(std::move(primitive));
		}
	}
	mesh.trianglesAtCreation = static_cast<uint32_t>(mesh.TriangleCount());
	mesh.remaining = 1.0f;
	return mesh;
}

/// Makes the model a broken mesh is drawn with: each triangle, its back face behind it and a wall on each open side,
/// every face lit flat by its own normal. Its corners are taken into the model's own space by a function.
entt::id_type MakeDrawMesh(const std::string& name, entt::id_type sourceMesh, const damage::Mesh& mesh,
                           const std::function<glm::vec3(glm::vec3)>& toModel)
{
	const auto* source = MeshOf(sourceMesh);
	if (source == nullptr || !Locator::resources::has_value())
	{
		return 0;
	}
	using MadePrimitive = graphics::L3DSubMesh::MadePrimitive;
	std::vector<std::vector<MadePrimitive>> subMeshes(1);
	size_t vertices = 0;
	const auto& sourceSubMeshes = source->GetSubMeshes();
	for (const auto& primitive : mesh.primitives)
	{
		const auto s = primitive.material >> 16U;
		const auto p = primitive.material & 0xFFFFU;
		if (primitive.triangles.empty() || s >= sourceSubMeshes.size() || p >= sourceSubMeshes[s]->GetPrimitives().size())
		{
			continue;
		}
		MadePrimitive made {.source = sourceSubMeshes[s].get(), .sourcePrimitive = p};
		const auto add = [&made](glm::vec3 position, glm::vec2 uv, glm::vec3 normal) {
			made.vertices.push_back({.position = position, .uv = uv, .normal = normal});
			return static_cast<uint16_t>(made.vertices.size() - 1);
		};
		for (const auto& triangle : primitive.triangles)
		{
			const auto& c = triangle.corners;
			auto normal = glm::cross(c[1].position - c[0].position, c[2].position - c[0].position);
			const float length = glm::length(normal);
			normal = length > 0.0f ? normal / length : glm::vec3(0.0f, 1.0f, 0.0f);
			std::array<glm::vec3, 3> front {};
			std::array<glm::vec3, 3> back {};
			for (size_t i = 0; i < 3; ++i)
			{
				front.at(i) = toModel(c.at(i).position);
				back.at(i) = toModel(c.at(i).position - normal * k_BackFace);
			}
			// The light falls on each face as its own normal turns it, in the model's space
			auto frontNormal = glm::cross(front[1] - front[0], front[2] - front[0]);
			const float frontLength = glm::length(frontNormal);
			frontNormal = frontLength > 0.0f ? frontNormal / frontLength : glm::vec3(0.0f, 1.0f, 0.0f);
			const auto backNormal = -frontNormal;
			std::array<uint16_t, 3> f {};
			std::array<uint16_t, 3> b {};
			for (size_t i = 0; i < 3; ++i)
			{
				f.at(i) = add(front.at(i), c.at(i).uv, frontNormal);
			}
			for (size_t i = 0; i < 3; ++i)
			{
				b.at(i) = add(back.at(i), c.at(i).uv, backNormal);
			}
			made.indices.insert(made.indices.end(), {f[0], f[1], f[2], b[0], b[2], b[1]});
			// A wall joins the front and back faces along each side nothing joins on to
			for (size_t side = 0; side < 3; ++side)
			{
				if (triangle.neighbours.at(side) >= 0)
				{
					continue;
				}
				const auto next = (side + 1) % 3;
				const auto fa = add(front.at(side), c.at(side).uv, frontNormal);
				const auto fb = add(front.at(next), c.at(next).uv, frontNormal);
				const auto ba = add(back.at(side), c.at(side).uv, backNormal);
				const auto bb = add(back.at(next), c.at(next).uv, backNormal);
				made.indices.insert(made.indices.end(), {fa, ba, fb, fb, ba, bb});
			}
		}
		if (made.vertices.size() > std::numeric_limits<uint16_t>::max())
		{
			continue;
		}
		if (vertices + made.vertices.size() > std::numeric_limits<uint16_t>::max())
		{
			subMeshes.emplace_back();
			vertices = 0;
		}
		vertices += made.vertices.size();
		subMeshes.back().push_back(std::move(made));
	}
	std::erase_if(subMeshes, [](const auto& primitives) { return primitives.empty(); });
	if (subMeshes.empty())
	{
		return 0;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto id = entt::hashed_string(name.c_str()).value();
	if (meshes.Contains(id))
	{
		meshes.Erase(id);
	}
	try
	{
		meshes.Load(id, resources::L3DLoader::FromMadeTag {}, name, *source, std::span(subMeshes));
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Can't make the model {}: {}", name, error.what());
		return 0;
	}
	return id;
}

void EraseMesh(entt::id_type id)
{
	if (id != 0 && Locator::resources::has_value() && Locator::resources::value().GetMeshes().Contains(id))
	{
		Locator::resources::value().GetMeshes().Erase(id);
	}
}

/// Draws the building with what is left of its model
void RedrawBuilding(entt::entity building, BuildingDamage& broken)
{
	const auto& registry = Entities();
	const auto& transform = registry.Get<const Transform>(building);
	const auto& meshComponent = registry.Get<const Mesh>(building);
	const bool followsLand = registry.AllOf<MorphWithTerrain>(building);
	const float baseHeight = LandHeight(glm::vec2(transform.position.x, transform.position.z));
	// Into the model's own space, undoing the lowering over the land, which its drawing lays on again
	const auto toModel = [&transform, followsLand, baseHeight](glm::vec3 world) {
		if (followsLand)
		{
			world.y += baseHeight - LandHeight(glm::vec2(world.x, world.z));
		}
		return (glm::transpose(transform.rotation) * (world - transform.position)) / transform.scale;
	};
	EraseMesh(broken.drawMesh);
	++broken.version;
	broken.drawMesh = MakeDrawMesh(fmt::format("broken/{}/{}", entt::to_integral(building), broken.version), meshComponent.id,
	                               broken.mesh, toModel);
	Entities().SetDirty();
}

void RemoveDamage(entt::entity building)
{
	auto& registry = Entities();
	if (auto* broken = registry.TryGet<BuildingDamage>(building))
	{
		EraseMesh(broken->drawMesh);
		registry.Remove<BuildingDamage>(building);
	}
}

/// What makes a piece fly: its velocity, its spin about its own axes and the building it passes through
void Launch(entt::entity piece, entt::entity parent, glm::vec3 velocity, glm::vec3 spin)
{
	if (!Locator::dynamicsSystem::has_value())
	{
		return;
	}
	auto& dynamics = Locator::dynamicsSystem::value();
	const auto started =
	    dynamics.InitialisePhysics(piece, {.velocity = velocity, .spin = spin, .thrower = parent, .add = true});
	if (started.entry != nullptr)
	{
		// Pieces hit nothing but the land
		started.entry->flags |= PhysicsEntry::k_NoObjectCollision;
	}
}

/// A piece sheds dust at its corners as it starts to fly, all but the first it finds; too thin a piece then goes
void ShedDustAndCheck(entt::entity piece)
{
	auto& registry = Entities();
	const auto& transform = registry.Get<const Transform>(piece);
	const auto& part = registry.Get<const BuildingPiece>(piece);
	if (part.mesh.primitives.empty())
	{
		return;
	}
	std::vector<std::array<glm::vec3, 3>> triangles;
	std::vector<glm::vec3> corners;
	for (const auto& triangle : part.mesh.primitives.front().triangles)
	{
		triangles.push_back({triangle.corners[0].position, triangle.corners[1].position, triangle.corners[2].position});
		for (const auto& corner : triangle.corners)
		{
			if (std::ranges::find(corners, corner.position) == corners.end())
			{
				corners.push_back(corner.position);
			}
		}
	}
	for (size_t i = 1; i < corners.size(); ++i)
	{
		const auto velocity = SpreadVector(k_PieceDustSpeed);
		if (Locator::dynamicsSystem::has_value())
		{
			// Where snow lies a puff grows by the snow's depth and takes on the land's colour
			const auto at = transform.position + transform.rotation * corners[i];
			const auto snow = snow_dust::SnowAt(at);
			Locator::dynamicsSystem::value().AddPuff(at, velocity, k_PieceDustSize * (1.0f + static_cast<float>(snow) / 255.0f),
			                                         snow_dust::Tint(k_PieceDustArgb, snow));
		}
	}
	if (physics::shapes::Fragment(triangles).tooThin)
	{
		EraseMesh(registry.Get<BuildingPiece>(piece).drawMesh);
		world_objects::Remove(piece);
	}
}

/// A piece of a mesh: its triangles about their middle, an object of its own there
entt::entity MakePiece(damage::Primitive primitive, glm::vec3 offset, entt::entity parent, entt::id_type sourceMesh,
                       uint8_t snowLevel)
{
	auto& registry = Entities();
	const auto centre = damage::CentreOf(primitive);
	damage::Offset(primitive, -centre);
	damage::LinkNeighbours(primitive);
	const auto piece = registry.Create();
	registry.Assign<Transform>(piece, offset + centre, glm::mat3(1.0f), glm::vec3(1.0f));
	const auto triangles = static_cast<uint32_t>(primitive.triangles.size());
	auto& part = registry.Assign<BuildingPiece>(piece);
	part.mesh.primitives.push_back(std::move(primitive));
	part.mesh.trianglesAtCreation = triangles;
	part.mesh.snowLevel = snowLevel;
	part.mesh.snowFrozen = true;
	part.parent = parent;
	part.sourceMesh = sourceMesh;
	part.turnsLeft = triangles * damage::k_TurnsPerTriangle;
	return piece;
}

void DrawPiece(entt::entity piece)
{
	auto& registry = Entities();
	auto& part = registry.Get<BuildingPiece>(piece);
	EraseMesh(part.drawMesh);
	part.drawMesh = MakeDrawMesh(fmt::format("piece/{}", entt::to_integral(piece)), part.sourceMesh, part.mesh,
	                             [](glm::vec3 p) { return p; });
	registry.AssignOrReplace<Mesh>(piece, part.drawMesh, static_cast<int8_t>(0), static_cast<int8_t>(1));
	registry.SetDirty();
}

/// The parts of a mesh that no longer hold on fall away as pieces, spinning a little, from where they are
void SplitLooseParts(damage::Mesh& mesh, glm::vec3 offset, bool groundTest, entt::entity parent, entt::id_type sourceMesh)
{
	const auto groups = damage::LabelGroups(mesh);
	std::optional<std::function<float(glm::vec2)>> land;
	if (groundTest)
	{
		// The mesh's corners are in the world when the land is tested
		land = [](glm::vec2 point) { return LandHeight(point); };
	}
	const auto anchors = damage::Anchors(mesh, groups, land);
	auto parts = damage::TakeAwayLooseGroups(mesh, anchors, groups);
	auto& registry = Entities();
	for (auto& part : parts)
	{
		const auto piece = MakePiece(std::move(part.primitive), offset, parent, sourceMesh, mesh.snowLevel);
		// As every piece is made, it lets go of its own parts that don't hold on to its first, before it spins away;
		// it stays about where it was made
		auto childMesh = std::move(registry.Get<BuildingPiece>(piece).mesh);
		const auto childPosition = registry.Get<const Transform>(piece).position;
		SplitLooseParts(childMesh, childPosition, false, parent, sourceMesh);
		registry.Get<BuildingPiece>(piece).mesh = std::move(childMesh);
		DrawPiece(piece);
		const auto spin = SpreadVector(damage::k_FallingSpin);
		Launch(piece, parent, glm::vec3(0.0f), spin);
		ShedDustAndCheck(piece);
	}
}

/// Breaks a building's model where a blow struck it: what broke off each primitive flies with the blow, and what no
/// longer holds on to the land falls away
void Strike(entt::entity building, BuildingDamage& broken, glm::vec3 point, glm::vec3 velocity, float reach)
{
	auto& registry = Entities();
	const auto sourceMesh = registry.Get<const Mesh>(building).id;
	auto brokenOff = damage::Strike(broken.mesh, point, velocity, reach);
	for (auto& primitive : brokenOff)
	{
		const auto piece = MakePiece(std::move(primitive), glm::vec3(0.0f), building, sourceMesh, broken.mesh.snowLevel);
		// The piece lets go of its own parts that don't hold on to its first, then gathers about what is left of it. Its
		// mesh is worked on apart, as the pieces falling from it are made.
		auto mesh = std::move(registry.Get<BuildingPiece>(piece).mesh);
		auto position = registry.Get<const Transform>(piece).position;
		SplitLooseParts(mesh, position, false, building, sourceMesh);
		if (!mesh.primitives.empty())
		{
			auto& kept = mesh.primitives.front();
			const auto centre = damage::CentreOf(kept);
			damage::Offset(kept, -centre);
			position += centre;
		}
		registry.Get<BuildingPiece>(piece).mesh = std::move(mesh);
		registry.Get<Transform>(piece).position = position;
		DrawPiece(piece);
		// Three draws the game makes and drops, then the spin
		for (int i = 0; i < 3; ++i)
		{
			GameRand(k_SpreadDraws);
		}
		const auto spin = SpreadVector(damage::k_BrokenSpin);
		Launch(piece, building, velocity, spin);
		ShedDustAndCheck(piece);
	}
	SplitLooseParts(broken.mesh, glm::vec3(0.0f), true, building, sourceMesh);
}

/// What breaking does to a building: it crashes, and loses life down to what is left of its model
void ApplyBreakage(entt::entity building, entt::entity hitter, std::optional<PlayerNames> player, bool hitterIsCreature)
{
	PlaySound(building, k_CrashKeys);
	if (!Locator::magicSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return;
	}
	auto& registry = Entities();
	auto values = magic::EffectValues::From(Locator::infoConstants::value().effect.at(k_CrushPreset));
	if (auto* broken = registry.TryGet<BuildingDamage>(building))
	{
		// The help's sprites about destroying buildings would show here for the local player's blow leaving less than
		// 0.4 of it; openblack has no help system yet
		values.Scale(damage::BreakageShare(world_objects::LifeOf(building), broken->mesh.remaining));
		// Divided by its defences, which taking the effect multiplies back, so its life comes down to what is left
		if (const auto* info = world_objects::InfoOf(building))
		{
			const auto defence = magic::EffectDefence::From(*info);
			for (size_t kind = 0; kind < values.numbers.size(); ++kind)
			{
				// (The game divides by a zero multiplier too; nothing ships one, and it is skipped here rather than made
				// infinite)
				if (defence.multipliers.at(kind) != 0.0f)
				{
					values.numbers.at(kind) /= defence.multipliers.at(kind);
				}
			}
		}
	}
	const bool validHitter = hitter != entt::null && registry.Valid(hitter);
	Locator::magicSystem::value().ApplyEffectToObject(building, values,
	                                                  magic::EffectSource {
	                                                      .player = player.value_or(PlayerNames::NEUTRAL),
	                                                      .casterCreature = hitterIsCreature ? hitter : entt::null,
	                                                      .appliedBy = validHitter ? hitter : entt::null,
	                                                      .playerless = !player.has_value(),
	                                                  });
}

/// After a blow: a building that lost nothing of its model forgets it was broken; otherwise it is drawn as it is left
bool Settle(entt::entity building, BuildingDamage& broken)
{
	const float remaining = damage::RemainingFraction(broken.mesh);
	if (remaining == 1.0f)
	{
		RemoveDamage(building);
		return false;
	}
	broken.mesh.remaining = remaining;
	RedrawBuilding(building, broken);
	return true;
}
} // namespace

void BuildingDamageSystem::ReactToImpact(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact)
{
	auto& registry = Entities();
	const auto building = entry.entity;
	const auto hitter = impact.hitBy;
	if (hitter == entt::null || !registry.Valid(hitter))
	{
		return;
	}
	auto* hitterEntry = dynamics.Find(hitter);
	// A creature may copy the player damaging the building by throwing at it, when the building's own body came from a
	// hand; a building's resting body never does, so this is never seen
	if (hitterEntry != nullptr && hitterEntry->player.has_value() && entry.Has(PhysicsEntry::k_FromHand) &&
	    Locator::creatureMindSystem::has_value())
	{
		const auto* place = registry.TryGet<const Transform>(building);
		Locator::creatureMindSystem::value().PlayerDid(physics::living::k_DeedDamageByThrowingAt,
		                                               place != nullptr ? place->position : glm::vec3(0.0f), building,
		                                               *hitterEntry->player);
	}
	if (!dynamics.PhysicallyDestroysAbodes(hitter) || hitterEntry == nullptr || hitterEntry->body == nullptr)
	{
		return;
	}
	const auto& rock = *hitterEntry->body;
	const float momentum = glm::length(rock.velocity) * rock.Mass();
	switch (damage::JudgeBlow(momentum))
	{
	case damage::Blow::Nothing:
		return;
	case damage::Blow::Knock:
		PlaySound(building, k_KnockKeys);
		return;
	case damage::Blow::HardKnock:
		PlaySound(building, k_HardKnockKeys);
		return;
	case damage::Blow::Breaks:
		break;
	}
	// TODO(buildings): while the blow's effects apply, the game marks a building struck by what a creature threw (a flag
	// whose reader wasn't found); the rock, not the creature, applies them
	if (IsBuilt(building))
	{
		auto* broken = registry.TryGet<BuildingDamage>(building);
		if (broken == nullptr || damage::RebuildsOnBlow(DrawShare(building)))
		{
			// A fresh broken model from the whole one, which no rock has struck yet
			if (broken != nullptr)
			{
				EraseMesh(broken->drawMesh);
			}
			auto& made = registry.AssignOrReplace<BuildingDamage>(building);
			made.mesh = MakeDamageMesh(building);
			broken = &made;
		}
		else if (broken->lastHitter == hitter)
		{
			// The same rock again, hard: the rock and the building pass through each other from now on
			// (The game also asks that the building's own body is the one struck, which only a temple's redirected blow
			// isn't; openblack's temple takes no blows yet)
			if (momentum > damage::k_PassThroughMomentum)
			{
				entry.thrower = hitter;
				hitterEntry->thrower = building;
			}
		}
		else
		{
			broken->lastHitter = hitter;
		}
		Strike(building, *broken, rock.Centre(), rock.velocity * damage::k_PieceSpeedShare,
		       rock.Radius() + damage::k_ReachBeyondRock);
		if (!Settle(building, registry.Get<BuildingDamage>(building)))
		{
			return;
		}
	}
	ApplyBreakage(building, hitter, impact.player, false);
}

void BuildingDamageSystem::Smash(entt::entity building, entt::entity creature, float creatureSize)
{
	auto& registry = Entities();
	if (!registry.Valid(building) || !registry.AllOf<Mesh, Transform>(building))
	{
		return;
	}
	if (!registry.AllOf<BuildingDamage>(building))
	{
		registry.Assign<BuildingDamage>(building).mesh = MakeDamageMesh(building);
	}
	PlaySound(building, k_SmashKeys);
	// Struck straight down where the building stands
	const auto point = registry.Get<const Transform>(building).position;
	Strike(building, registry.Get<BuildingDamage>(building), point, glm::vec3(0.0f, -1.0f, 0.0f),
	       creatureSize * k_CreatureBlowReach);
	if (!Settle(building, registry.Get<BuildingDamage>(building)))
	{
		return;
	}
	ApplyBreakage(building, creature, world_objects::PlayerOf(creature), true);
}

entt::entity BuildingDamageSystem::PieceAtRest(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity piece,
                                               bool insert)
{
	auto& registry = Entities();
	const auto kept = dynamics.EndPhysicsAsObject(piece, insert, entry != nullptr);
	if (!registry.Valid(piece))
	{
		return kept;
	}
	auto& part = registry.Get<BuildingPiece>(piece);
	const auto& transform = registry.Get<const Transform>(piece);
	float area = 0.0f;
	if (!part.mesh.primitives.empty())
	{
		for (const auto& triangle : part.mesh.primitives.front().triangles)
		{
			const auto& c = triangle.corners;
			area += 0.5f * glm::length(glm::cross(c[1].position - c[0].position, c[2].position - c[0].position));
		}
	}
	// A big piece goes back into its building as rubble, where it lies
	auto* broken =
	    part.parent != entt::null && registry.Valid(part.parent) ? registry.TryGet<BuildingDamage>(part.parent) : nullptr;
	if (area > damage::k_RubbleArea && broken != nullptr && !part.mesh.primitives.empty())
	{
		auto rubble = part.mesh.primitives.front();
		for (auto& triangle : rubble.triangles)
		{
			for (auto& corner : triangle.corners)
			{
				corner.position = transform.position + transform.rotation * corner.position;
			}
		}
		damage::LinkNeighbours(rubble);
		broken->mesh.primitives.push_back(std::move(rubble));
		RedrawBuilding(part.parent, *broken);
		EraseMesh(part.drawMesh);
		world_objects::Remove(piece);
		return entt::null;
	}
	part.parent = entt::null;
	part.mesh.snowFrozen = false;
	return kept;
}

void BuildingDamageSystem::ForgetHitter(entt::entity rock)
{
	Entities().Each<BuildingDamage>([rock](BuildingDamage& broken) {
		if (broken.lastHitter == rock)
		{
			broken.lastHitter = entt::null;
		}
	});
}

void BuildingDamageSystem::ProcessTurn()
{
	auto& registry = Entities();
	std::vector<entt::entity> expired;
	registry.Each<BuildingPiece>([&registry, &expired](entt::entity piece, BuildingPiece& part) {
		if (part.parent != entt::null && !registry.Valid(part.parent))
		{
			part.parent = entt::null;
		}
		if (part.turnsLeft > 0)
		{
			--part.turnsLeft;
		}
		if (part.turnsLeft == 0)
		{
			expired.push_back(piece);
		}
	});
	for (const auto piece : expired)
	{
		EraseMesh(registry.Get<BuildingPiece>(piece).drawMesh);
		world_objects::Remove(piece);
	}
}

std::optional<float> BuildingDamageSystem::PartialShare(entt::entity building) const
{
	const auto& registry = Entities();
	if (!registry.Valid(building) || (!registry.AllOf<BuildingDamage>(building) && IsBuilt(building)))
	{
		return std::nullopt;
	}
	// Nothing is drawn at none, and the whole model at all of it
	const float share = DrawShare(building);
	if (!(share > 0.0f) || share >= 1.0f)
	{
		return std::nullopt;
	}
	return share;
}

entt::id_type BuildingDamageSystem::DrawnMesh(entt::entity object, entt::id_type own) const
{
	const auto* broken = Entities().TryGet<const BuildingDamage>(object);
	return broken != nullptr && broken->drawMesh != 0 ? broken->drawMesh : own;
}

void BuildingDamageSystem::Reset()
{
	auto& registry = Entities();
	// The made models go, and the buildings forget they were broken (their pieces are taken with the level)
	registry.Each<BuildingDamage>([](BuildingDamage& broken) { EraseMesh(broken.drawMesh); });
	registry.Each<BuildingPiece>([](BuildingPiece& part) { EraseMesh(part.drawMesh); });
	std::vector<entt::entity> broken;
	registry.Each<const BuildingDamage>(
	    [&broken](entt::entity building, const BuildingDamage&) { broken.push_back(building); });
	for (const auto building : broken)
	{
		registry.Remove<BuildingDamage>(building);
	}
}
