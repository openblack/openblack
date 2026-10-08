/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "PickingSystem.h"

#include <span>
#include <vector>

#include <LNDFile.h>
#include <glm/geometric.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/LandLine.h"
#include "3D/MapCoords.h"
#include "Creature/CreatureMorph.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The land's corner heights as the game reads them for its line test: the cell's own block holds its far corners
std::optional<land_line::CellHeights> CornersOf(int32_t x, int32_t z)
{
	if (!Locator::terrainSystem::has_value())
	{
		return std::nullopt;
	}
	const auto* cell = Locator::terrainSystem::value().FindCell({static_cast<uint16_t>(x), static_cast<uint16_t>(z)});
	if (cell == nullptr)
	{
		return std::nullopt;
	}
	// Within a block's 17 by 17 cells, +1 is across z and +17 across x
	return land_line::CellHeights {
	    .here = cell[0].altitude, .acrossX = cell[17].altitude, .acrossZ = cell[1].altitude, .acrossBoth = cell[18].altitude};
}

float HeightAt(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}

/// A model as it is drawn: each submesh's corners in the world, with their texture coordinates
struct DrawnSubMesh
{
	const graphics::L3DSubMesh* subMesh;
	std::vector<glm::vec3> corners;
	std::vector<glm::vec2> uvs;
};

/// The bones a model is posed by as it is drawn: an animal by its clip, a creature by its animations, anything else in
/// the pose its model rests in
std::span<const glm::mat4> BonesOf(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh)
{
	const auto& rest = mesh.GetBoneMatrices();
	if (const auto* pose = registry.TryGet<const AnimalPose>(entity); pose != nullptr && pose->bones.size() == rest.size())
	{
		return pose->bones;
	}
	if (const auto* animation = registry.TryGet<const CreatureAnimation>(entity);
	    animation != nullptr && animation->boneMatrices.size() == rest.size())
	{
		return animation->boneMatrices;
	}
	return rest;
}

/// The corners of a submesh in the world as it is drawn: posed by its bones, a creature's body blended from its
/// meshes, and a model following the land moved up and down with it
DrawnSubMesh Place(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh, size_t index,
                   const glm::mat4& model)
{
	const auto& subMesh = *mesh.GetSubMeshes()[index];
	const auto& geometry = subMesh.GetBodyGeometry();
	DrawnSubMesh drawn {.subMesh = &subMesh, .corners = {}, .uvs = geometry.uvs};
	drawn.corners.reserve(geometry.positions.size());

	// A creature's body is its base mesh moved towards the meshes of how evil or good, thin or fat and weak or strong
	// it is drawn
	std::array<const graphics::L3DSubMesh*, 3> morphs {};
	const creature_morph::Morph* morph = nullptr;
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		if (const auto* shape = registry.TryGet<const CreatureMorph>(entity))
		{
			auto& meshes = Locator::resources::value().GetMeshes();
			const auto ids = creature_morph::MeshesOf(creature->species, shape->drawn,
			                                          [&meshes](entt::id_type id) { return meshes.Contains(id); });
			const auto subMeshOf = [&meshes, index](entt::id_type id) -> const graphics::L3DSubMesh* {
				const auto& subs = meshes.Handle(id)->GetSubMeshes();
				return index < subs.size() ? subs[index].get() : nullptr;
			};
			morphs = {subMeshOf(ids.evilGood), subMeshOf(ids.thinFat), subMeshOf(ids.weakStrong)};
			morph = &shape->drawn;
		}
	}
	const auto bones = BonesOf(registry, entity, mesh);
	const bool followsLand = registry.AllOf<MorphWithTerrain>(entity);
	const glm::vec3 origin(model[3]);
	const glm::vec3 up(model[1]);
	const float scale = glm::length(glm::vec3(model[0]));
	for (size_t i = 0; i < geometry.positions.size(); ++i)
	{
		auto position = geometry.positions[i];
		if (morph != nullptr)
		{
			const auto partner = [&](const graphics::L3DSubMesh* other) {
				const auto& positions = other != nullptr ? other->GetBodyGeometry().positions : geometry.positions;
				return i < positions.size() ? positions[i] : geometry.positions[i];
			};
			position = creature_morph::Blend(position, partner(morphs[0]), partner(morphs[1]), partner(morphs[2]), *morph);
		}
		glm::vec4 local(position, 1.0f);
		if (i < geometry.bones.size() && geometry.bones[i] != graphics::L3DSubMesh::k_NoBone &&
		    geometry.bones[i] < bones.size())
		{
			local = bones[geometry.bones[i]] * local;
		}
		auto world = glm::vec3(model * local);
		if (followsLand)
		{
			// Each corner moves along the model's up by how much higher the land is under it than under its origin
			const float rise = HeightAt({world.x, world.z}) - HeightAt({origin.x, origin.z});
			if (up.x == 0.0f && up.z == 0.0f)
			{
				world.y += rise;
			}
			else
			{
				world += up * (rise / scale);
			}
		}
		drawn.corners.push_back(world);
	}
	return drawn;
}

/// Whether the submesh is drawn at the level of detail and stage the model is drawn at: the nearest level of detail and
/// the first stage, as openblack draws every model
bool Drawn(const graphics::L3DSubMesh& subMesh)
{
	const auto flags = subMesh.GetFlags();
	return (flags.lodMask & 1) != 0 && flags.status == 0;
}

} // namespace

std::optional<glm::vec3> PickingSystem::LandAlong(glm::vec3 from, glm::vec3 to) const
{
	if (const auto hit = land_line::LandAlong(from, to, CornersOf))
	{
		return glm::vec3(hit->x, HeightAt(*hit), hit->y);
	}
	return std::nullopt;
}

std::optional<glm::vec3> PickingSystem::LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera) const
{
	if (const auto hit = land_line::LandOrSeaAlong(from, to, camera, CornersOf))
	{
		return glm::vec3(hit->x, HeightAt(*hit), hit->y);
	}
	return std::nullopt;
}

std::optional<glm::vec3> PickingSystem::LandUnderPixel(glm::vec3 camera, glm::vec3 nearPoint, bool withSea) const
{
	if (const auto point = land_line::UnderPixel(camera, nearPoint, withSea, CornersOf, HeightAt))
	{
		return land_line::KeptInReach(*point);
	}
	return std::nullopt;
}

void PickingSystem::PickUnderCursor(const Frame& frame)
{
	_handPick = _pick;
	_pick = {};
	const auto& view = frame.view;

	// The land or sea under the cursor, counted a little further than it is
	float landDistance = std::numeric_limits<float>::max();
	if (const auto land = land_line::UnderPixel(view.camera, frame.nearPoint, true, CornersOf, HeightAt))
	{
		landDistance = screen_pick::Depth(view, *land) + land_line::k_LandDistanceAllowance;
		_pick.land = land_line::KeptInReach(*land);
	}

	// The drawn object nearest the camera under the cursor
	if (!Locator::rendereringSystem::has_value() || !Locator::resources::has_value())
	{
		_pick.point = _pick.land;
		_pick.distance = landDistance;
		return;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	auto& meshes = Locator::resources::value().GetMeshes();
	std::optional<entt::entity> picked;
	float pickedDistance = std::numeric_limits<float>::max();
	glm::vec3 pickedOrigin(0.0f);
	glm::vec2 pickedExtents(0.0f);
	for (const auto& [entity, model] : Locator::rendereringSystem::value().GetContext().drawnObjects)
	{
		if (!registry.Valid(entity) || registry.AnyOf<Hand, InHand>(entity))
		{
			continue;
		}
		// A building not begun is no more there for the cursor than it is for anything else
		if (const auto* progress = registry.TryGet<const BuildProgress>(entity);
		    progress != nullptr && registry.AllOf<Abode>(entity) && progress->built == 0.0f)
		{
			continue;
		}
		const auto* meshComponent = registry.TryGet<const Mesh>(entity);
		if (meshComponent == nullptr || !meshes.Contains(meshComponent->id))
		{
			continue;
		}
		const auto mesh = meshes.Handle(meshComponent->id);
		const auto* transform = registry.TryGet<const Transform>(entity);
		const float scale = transform != nullptr ? transform->scale.x : 1.0f;
		const auto box = mesh->GetBoundingBox();
		const auto halfExtents = box.Size() * 0.5f;
		const float radius = glm::length(halfExtents) * scale;
		const auto centre = glm::vec3(model * glm::vec4(box.Center(), 1.0f));
		const glm::vec3 origin(model[3]);
		if (!screen_pick::CursorOverSphere(view, centre, radius, origin))
		{
			continue;
		}
		// Nothing past the nearest object picked so far is tested
		if (picked.has_value() && pickedDistance + radius < screen_pick::Depth(view, centre))
		{
			continue;
		}

		// A tree of a forest is picked through its leaves' holes
		const auto* info = world_objects::InfoOf(entity);
		const bool throughHoles = info != nullptr && info->type == ObjectType::ForestTree;
		std::optional<float> distance;
		for (size_t s = 0; s < mesh->GetSubMeshes().size() && !distance.has_value(); ++s)
		{
			const auto& subMesh = *mesh->GetSubMeshes()[s];
			if (!Drawn(subMesh))
			{
				continue;
			}
			const auto drawn = Place(registry, entity, *mesh, s, model);
			std::vector<screen_pick::ClipCorner> corners;
			corners.reserve(drawn.corners.size());
			for (size_t i = 0; i < drawn.corners.size(); ++i)
			{
				corners.push_back(
				    screen_pick::ToClip(view, drawn.corners[i], i < drawn.uvs.size() ? drawn.uvs[i] : glm::vec2(0.0f)));
			}
			const auto& indices = subMesh.GetBodyGeometry().indices;
			for (const auto& primitive : subMesh.GetPrimitives())
			{
				if (primitive.indicesOffset + primitive.indicesCount > indices.size())
				{
					continue;
				}
				distance = screen_pick::FirstHit(
				    view, {.corners = corners,
				           .indices = std::span(indices).subspan(primitive.indicesOffset, primitive.indicesCount),
				           .twoSided = primitive.twoSided,
				           .mask = throughHoles ? mesh->GetSkinMask(primitive.skinID) : nullptr});
				if (distance.has_value())
				{
					break;
				}
			}
		}
		if (!distance.has_value())
		{
			continue;
		}
		// A building following the land that is only partly built is hit only below what of it stands
		if (const auto* progress = registry.TryGet<const BuildProgress>(entity);
		    progress != nullptr && registry.AllOf<MorphWithTerrain>(entity) && progress->built < 1.0f)
		{
			const auto hit = view.camera + (frame.nearPoint - view.camera) * (*distance / view.near);
			if (hit.y > origin.y + halfExtents.y * 2.0f * scale * progress->built)
			{
				continue;
			}
		}
		if (*distance < pickedDistance)
		{
			picked = entity;
			pickedDistance = *distance;
			pickedOrigin = origin;
			pickedExtents = glm::vec2(halfExtents.x, halfExtents.z);
		}
	}

	// The land wins over the object when it is as near or nearer and its point is off the object's footprint
	if (picked.has_value() && _pick.land.has_value())
	{
		const auto land = map_coords::ToMetres(map_coords::FromMetres({_pick.land->x, _pick.land->z}));
		if (screen_pick::LandHidesObject(landDistance, pickedDistance, land, {pickedOrigin.x, pickedOrigin.z}, pickedExtents))
		{
			picked.reset();
		}
	}
	if (picked.has_value())
	{
		_pick.object = picked;
		const auto* transform = registry.TryGet<const Transform>(*picked);
		_pick.point = transform != nullptr ? transform->position : pickedOrigin;
		_pick.distance = pickedDistance;
	}
	else
	{
		_pick.point = _pick.land;
		_pick.distance = landDistance;
	}
}

std::optional<screen_pick::MeshHit> PickingSystem::FeelModel(entt::entity object, glm::vec3 origin, glm::vec3 direction) const
{
	if (!Locator::rendereringSystem::has_value() || !Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* meshComponent = registry.Valid(object) ? registry.TryGet<const Mesh>(object) : nullptr;
	auto& meshes = Locator::resources::value().GetMeshes();
	if (meshComponent == nullptr || !meshes.Contains(meshComponent->id))
	{
		return std::nullopt;
	}
	const auto& drawn = Locator::rendereringSystem::value().GetContext().drawnObjects;
	const auto found = std::ranges::find(drawn, object, &RenderContext::DrawnObject::entity);
	if (found == drawn.end())
	{
		return std::nullopt;
	}
	const auto mesh = meshes.Handle(meshComponent->id);
	std::optional<screen_pick::MeshHit> nearest;
	for (size_t s = 0; s < mesh->GetSubMeshes().size(); ++s)
	{
		// The submeshes of the nearest level of detail
		if ((mesh->GetSubMeshes()[s]->GetFlags().lodMask & 1) == 0)
		{
			continue;
		}
		const auto placed = Place(registry, object, *mesh, s, found->model);
		const auto hit = screen_pick::NearestIntersection(placed.corners, mesh->GetSubMeshes()[s]->GetBodyGeometry().indices,
		                                                  origin, direction, false);
		if (hit.has_value() && (!nearest.has_value() || hit->distance < nearest->distance))
		{
			nearest = hit;
		}
	}
	return nearest;
}
