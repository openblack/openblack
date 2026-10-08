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
#include "ECS/Components/VillagerPose.h"
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

/// The bones a model is posed by as it is drawn: an animal by its clip, a creature by its animations, anything else in
/// the pose its model rests in
std::span<const glm::mat4> BonesOf(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh)
{
	const auto& rest = mesh.GetBoneMatrices();
	if (const auto* pose = registry.TryGet<const AnimalPose>(entity); pose != nullptr && pose->bones.size() == rest.size())
	{
		return pose->bones;
	}
	if (const auto* pose = registry.TryGet<const VillagerPose>(entity); pose != nullptr && pose->bones.size() == rest.size())
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
void Place(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh, size_t index,
           const glm::mat4& model, std::vector<glm::vec3>& corners)
{
	const auto& subMesh = *mesh.GetSubMeshes()[index];
	const auto& geometry = subMesh.GetBodyGeometry();
	corners.clear();
	corners.reserve(geometry.positions.size());

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
		corners.push_back(world);
	}
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

PickingSystemInterface::Pick picking::Locked(const PickingSystemInterface::Pick& previous, std::optional<glm::vec3> land,
                                             float landDistance)
{
	auto pick = previous;
	if (land.has_value())
	{
		pick.land = land;
		pick.point = land;
		pick.distance = landDistance;
	}
	return pick;
}

void picking::CarryHover(const PickingSystemInterface::Pick& previous, PickingSystemInterface::Pick& next, float seconds)
{
	if (next.object == previous.hoverObject)
	{
		next.hoverObject = previous.hoverObject;
		next.hoverSeconds = previous.hoverSeconds + seconds;
	}
	else
	{
		next.hoverObject = next.object;
		next.hoverSeconds = 0.0f;
	}
}

void PickingSystem::PickUnderCursor(const Frame& frame)
{
	const auto& view = frame.view;

	// The land or sea under the cursor, counted a little further than it is
	float landDistance = std::numeric_limits<float>::max();
	std::optional<glm::vec3> land;
	if (const auto under = land_line::UnderPixel(view.camera, frame.nearPoint, true, CornersOf, HeightAt))
	{
		landDistance = screen_pick::Depth(view, *under) + land_line::k_LandDistanceAllowance;
		land = land_line::KeptInReach(*under);
	}

	// Gripping the land, the interface keeps what it picked and follows only the land
	if (frame.locked)
	{
		_pick = picking::Locked(_pick, land, landDistance);
		return;
	}

	const auto previous = _pick;
	_pick = {};
	_pick.land = land;
	if (!Locator::rendereringSystem::has_value() || !Locator::resources::has_value())
	{
		_pick.point = _pick.land;
		_pick.distance = landDistance;
		picking::CarryHover(previous, _pick, frame.seconds);
		return;
	}

	// The objects drawn this frame, in the order they are drawn: a hand, what a hand holds and a building not begun are
	// not there for the cursor
	const auto& registry = Locator::entitiesRegistry::value();
	auto& meshes = Locator::resources::value().GetMeshes();
	_candidates.clear();
	_candidateEntities.clear();
	_candidateModels.clear();
	for (const auto& [entity, model] : Locator::rendereringSystem::value().GetContext().drawnObjects)
	{
		if (!registry.Valid(entity) || registry.AnyOf<Hand, InHand>(entity))
		{
			continue;
		}
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
		_candidates.push_back({
		    .centre = glm::vec3(model * glm::vec4(box.Center(), 1.0f)),
		    .radius = glm::length(halfExtents) * scale,
		    .origin = glm::vec3(model[3]),
		    .halfExtents = glm::vec2(halfExtents.x, halfExtents.z),
		});
		_candidateEntities.push_back(entity);
		_candidateModels.push_back(model);
	}

	// How far along the view an object's first triangle under the cursor is
	const auto distanceOf = [&](size_t i) -> std::optional<float> {
		const auto entity = _candidateEntities[i];
		const auto& model = _candidateModels[i];
		const auto mesh = meshes.Handle(registry.Get<const Mesh>(entity).id);
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
			Place(registry, entity, *mesh, s, model, _placed);
			const auto& geometry = subMesh.GetBodyGeometry();
			_corners.clear();
			for (size_t c = 0; c < _placed.size(); ++c)
			{
				_corners.push_back(
				    screen_pick::ToClip(view, _placed[c], c < geometry.uvs.size() ? geometry.uvs[c] : glm::vec2(0.0f)));
			}
			const auto& indices = geometry.indices;
			for (const auto& primitive : subMesh.GetPrimitives())
			{
				if (primitive.indicesOffset + primitive.indicesCount > indices.size())
				{
					continue;
				}
				distance = screen_pick::FirstHit(
				    view,
				    {.corners = _corners,
				     .indices = std::span(indices).subspan(primitive.indicesOffset, primitive.indicesCount),
				     .twoSided = primitive.twoSided,
				     .mask = throughHoles ? mesh->GetSkinMask(primitive.skinID) : nullptr},
				    _scratch);
				if (distance.has_value())
				{
					break;
				}
			}
		}
		if (!distance.has_value())
		{
			return std::nullopt;
		}
		// A building following the land that is only partly built is hit only below what of it stands
		if (const auto* progress = registry.TryGet<const BuildProgress>(entity);
		    progress != nullptr && registry.AllOf<MorphWithTerrain>(entity) && progress->built < 1.0f)
		{
			const auto* transform = registry.TryGet<const Transform>(entity);
			const float scale = transform != nullptr ? transform->scale.x : 1.0f;
			const auto hit = view.camera + (frame.nearPoint - view.camera) * (*distance / view.near);
			const float halfHeight = mesh->GetBoundingBox().Size().y * 0.5f;
			if (hit.y > _candidates[i].origin.y + halfHeight * 2.0f * scale * progress->built)
			{
				return std::nullopt;
			}
		}
		return distance;
	};
	auto picked = screen_pick::PickAmong(view, _candidates, distanceOf);

	// The land wins over the object when it is as near or nearer and its point is off the object's footprint
	if (picked.has_value() && _pick.land.has_value())
	{
		const auto landPoint = map_coords::ToMetres(map_coords::FromMetres({_pick.land->x, _pick.land->z}));
		const auto& candidate = _candidates[picked->index];
		if (screen_pick::LandHidesObject(landDistance, picked->distance, landPoint, {candidate.origin.x, candidate.origin.z},
		                                 candidate.halfExtents))
		{
			picked.reset();
		}
	}
	if (picked.has_value())
	{
		const auto entity = _candidateEntities[picked->index];
		_pick.object = entity;
		const auto* transform = registry.TryGet<const Transform>(entity);
		_pick.point = transform != nullptr ? transform->position : _candidates[picked->index].origin;
		_pick.distance = picked->distance;
	}
	else
	{
		_pick.point = _pick.land;
		_pick.distance = landDistance;
	}
	picking::CarryHover(previous, _pick, frame.seconds);
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
		std::vector<glm::vec3> placed;
		Place(registry, object, *mesh, s, found->model, placed);
		const auto hit = screen_pick::NearestIntersection(placed, mesh->GetSubMeshes()[s]->GetBodyGeometry().indices, origin,
		                                                  direction, false);
		if (hit.has_value() && (!nearest.has_value() || hit->distance < nearest->distance))
		{
			nearest = hit;
		}
	}
	return nearest;
}
