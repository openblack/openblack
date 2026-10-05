/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TempleExteriorSystem.h"

#include <vector>

#include <L3DFile.h>
#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/TempleExteriorMorph.h"
#include "Common/Bitmap16B.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/TempleExterior.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// fn_0064ACC0 gives a player this share of the influence when nobody has any
constexpr float k_ShareOfNoInfluence = 0.01f;
/// The mesh the temples' are made from, which the outsides are blended into
constexpr std::string_view k_FirstTemple = "temple/b_first_temple_l3d";
} // namespace

void TempleExteriorSystem::UpdateTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& alignment = Locator::alignmentSystem::value();
	registry.Each<const Temple, TempleExterior, Mesh>(
	    [&alignment](const entt::entity entity, const Temple& temple, TempleExterior& exterior, Mesh& mesh) {
		    // Citadel::Process: toward its player's goodness, and twice their share of the influence
		    // TODO(raffclar): the player's share of the influence, once influence is simulated
		    exterior.alignmentTarget = (alignment.GetPlayerAlignment(temple.owner) + 1.0f) * 0.5f;
		    exterior.sizeTarget = 2.0f * k_ShareOfNoInfluence;
		    exterior.alignment = TempleExteriorMorph::Step(exterior.alignment, exterior.alignmentTarget);
		    exterior.size = TempleExteriorMorph::Step(exterior.size, exterior.sizeTarget);
		    const glm::vec2 look {exterior.size, exterior.alignment};
		    if (exterior.morphed != look)
		    {
			    Morph(entity, mesh, exterior, temple.owner);
			    exterior.morphed = look;
		    }
	    });
}

void TempleExteriorSystem::Morph(entt::entity entity, Mesh& mesh, const TempleExterior& exterior, PlayerNames owner)
{
	using namespace TempleExteriorMorph;
	auto& resources = Locator::resources::value();
	auto& files = resources.GetL3DFiles();
	const auto firstTemple = entt::hashed_string(k_FirstTemple.data()).value();
	if (!files.Contains(firstTemple))
	{
		return;
	}

	// The temple's vertices, each the blend of the four meshes' about its look
	auto blended = *files.Handle(firstTemple);
	auto vertices = blended.EditVertices();
	std::vector<l3d::L3DVertex> sum(vertices.size());
	for (const auto& corner : Corners(exterior.size, exterior.alignment))
	{
		const auto id = entt::hashed_string(fmt::format("temple/{}", MeshName(corner.size, corner.stage)).c_str()).value();
		if (corner.weight == 0.0f || !files.Contains(id))
		{
			continue;
		}
		const auto& source = files.Handle(id)->GetVertices();
		if (source.size() != sum.size())
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "The temple's mesh {} isn't the shape of the first temple's",
			                    MeshName(corner.size, corner.stage));
			return;
		}
		const auto add = [&corner](l3d::L3DPoint& to, const l3d::L3DPoint& from) {
			to.x += from.x * corner.weight;
			to.y += from.y * corner.weight;
			to.z += from.z * corner.weight;
		};
		for (size_t i = 0; i < sum.size(); ++i)
		{
			add(sum[i].position, source[i].position);
			add(sum[i].normal, source[i].normal);
			sum[i].texCoord.x += source[i].texCoord.x * corner.weight;
			sum[i].texCoord.y += source[i].texCoord.y * corner.weight;
		}
	}
	std::ranges::copy(sum, vertices.begin());

	// The temple's own mesh, made once from the first temple's and changed after
	auto& meshes = resources.GetMeshes();
	const auto name = fmt::format("temple/exterior/{}", static_cast<uint32_t>(entity));
	const auto id = entt::hashed_string(name.c_str()).value();
	if (!meshes.Contains(id))
	{
		meshes.Load(id, resources::L3DLoader::FromDynamicFileTag {}, name, blended);
		mesh.id = id;
	}
	auto& temple = *meshes.Handle(id);
	temple.UpdateVertices(blended);

	// Its texture, of its player's set, between the two looks about its alignment
	const auto texture = TextureOf(exterior.alignment);
	const auto set = static_cast<uint32_t>(owner) & 3;
	auto& bitmaps = resources.GetBitmaps();
	const auto from = entt::hashed_string(fmt::format("temple/{}", ImageName(texture.from, set)).c_str()).value();
	const auto to = entt::hashed_string(fmt::format("temple/{}", ImageName(texture.to, set)).c_str()).value();
	const auto skins = blended.EditSkins();
	if (skins.empty() || !bitmaps.Contains(from) || !bitmaps.Contains(to))
	{
		return;
	}
	auto& fromImage = *bitmaps.Handle(from);
	auto& toImage = *bitmaps.Handle(to);
	const auto texels = skins.front().texels.size();
	if (fromImage.Size() / 2 != texels || toImage.Size() / 2 != texels)
	{
		return;
	}
	std::vector<uint16_t> skin(texels);
	BlendTexels({fromImage.Data(), texels}, {toImage.Data(), texels}, texture.weight, skin);
	temple.UpdateSkin(skins.front().id, skin);
}
