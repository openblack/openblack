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
#include "ECS/Components/BuildingDamage.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/VillagerPose.h"
#include "ECS/PosedModel.h"
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

namespace
{
/// Along a line, how far (in its direction's lengths) it first meets a broken building's triangles, from either side; a
/// triangle nearly edge on to the line is passed by
std::optional<float> BrokenAlong(const physics::damage::Mesh& mesh, glm::vec3 from, glm::vec3 direction)
{
	constexpr float k_EdgeOn = 0.005f;
	std::optional<float> nearest;
	for (const auto& primitive : mesh.primitives)
	{
		for (const auto& triangle : primitive.triangles)
		{
			const auto a = triangle.corners[0].position;
			const auto b = triangle.corners[1].position;
			const auto c = triangle.corners[2].position;
			auto normal = glm::cross(b - a, c - a);
			if (normal != glm::vec3(0.0f))
			{
				normal = glm::normalize(normal);
			}
			const float facing = glm::dot(direction, normal);
			if (std::abs(facing) <= k_EdgeOn)
			{
				continue;
			}
			const float along = (glm::dot(normal, a) - glm::dot(from, normal)) / facing;
			const auto point = from + direction * along;
			// Inside when the point is on the same side of all three sides
			const auto side = [&normal, &point](glm::vec3 p, glm::vec3 q) {
				return glm::dot(glm::cross(point - p, q - p), normal) > 0.0f ? 1 : 0;
			};
			const int inside = side(a, b) + side(b, c) + side(c, a);
			if ((inside == 0 || inside == 3) && (!nearest.has_value() || along < *nearest))
			{
				nearest = along;
			}
		}
	}
	return nearest;
}
} // namespace

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
		// A broken building is picked by its broken model's triangles, along the cursor's line
		if (const auto* broken = registry.TryGet<const BuildingDamage>(entity); broken != nullptr && broken->drawMesh != 0)
		{
			const auto along = BrokenAlong(broken->mesh, view.camera, frame.nearPoint - view.camera);
			return along.has_value() ? std::optional<float>(*along * view.near) : std::nullopt;
		}
		const auto mesh = meshes.Handle(registry.Get<const Mesh>(entity).id);
		// A tree of a forest is picked through its leaves' holes
		const auto* info = world_objects::InfoOf(entity);
		const bool throughHoles = info != nullptr && info->type == ObjectType::ForestTree;
		std::optional<float> distance;
		for (size_t s = 0; s < mesh->GetSubMeshes().size() && !distance.has_value(); ++s)
		{
			const auto& subMesh = *mesh->GetSubMeshes()[s];
			if (!ecs::posed_model::IsDrawn(subMesh))
			{
				continue;
			}
			ecs::posed_model::Place(registry, entity, *mesh, s, model, _placed);
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
		ecs::posed_model::Place(registry, object, *mesh, s, found->model, placed);
		const auto hit = screen_pick::NearestIntersection(placed, mesh->GetSubMeshes()[s]->GetBodyGeometry().indices, origin,
		                                                  direction, false);
		if (hit.has_value() && (!nearest.has_value() || hit->distance < nearest->distance))
		{
			nearest = hit;
		}
	}
	return nearest;
}
