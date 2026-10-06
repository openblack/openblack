/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureSkinSystem.h"

#include <algorithm>
#include <span>
#include <string>

#include <L3DFile.h>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureSkin.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureSkin.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::creature::CreatureRig;

namespace
{
/// A creature mesh's file, read the first time a skin of it is painted, for its skins' texels
const l3d::L3DFile* MeshFile(const std::string& name)
{
	if (name.empty())
	{
		return nullptr;
	}
	auto& files = Locator::resources::value().GetL3DFiles();
	const auto id = entt::hashed_string(("creature/skins/" + name).c_str()).value();
	if (!files.Contains(id))
	{
		auto& fileSystem = Locator::filesystem::value();
		const auto path = fileSystem.GetPath<filesystem::Path::CreatureMesh>() / (name + ".l3d");
		try
		{
			files.Load(id, resources::L3DFileLoader::FromDiskTag {}, path);
		}
		catch (std::exception& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Can't read the skins of {}: {}", path.string(), err.what());
			return nullptr;
		}
	}
	return &*files.Handle(id);
}

std::span<const uint16_t> TexelsOf(const l3d::L3DTexture& skin)
{
	return {reinterpret_cast<const uint16_t*>(skin.texels.data()), skin.texels.size()};
}

/// The skins as the body is drawn: the base mesh's, and the evil or good mesh's it is blended towards, the base's
/// where the species has none
struct SkinSources
{
	const l3d::L3DFile* base;
	const l3d::L3DFile* variant;
	entt::id_type baseId;
	entt::id_type variantId;
};

std::optional<SkinSources> SourcesOf(const CreatureRig& rig, float evilGood)
{
	const auto variantMesh = evilGood < 0.0f ? CreatureRig::Mesh::Evil : CreatureRig::Mesh::Good;
	const auto& baseName = rig.meshNames.at(static_cast<size_t>(CreatureRig::Mesh::Base));
	const auto& variantName = rig.meshNames.at(static_cast<size_t>(variantMesh));
	const auto* base = MeshFile(baseName);
	if (base == nullptr)
	{
		return std::nullopt;
	}
	const auto* variant = rig.hasMesh.at(static_cast<size_t>(variantMesh)) ? MeshFile(variantName) : nullptr;
	return SkinSources {
	    .base = base,
	    .variant = variant != nullptr ? variant : base,
	    .baseId = entt::hashed_string(baseName.c_str()).value(),
	    .variantId = entt::hashed_string((variant != nullptr ? variantName : baseName).c_str()).value(),
	};
}

void Paint(CreatureSkin& skin, const SkinSources& sources, uint8_t weight, const CreatureTattoos& tattoos,
           const CreatureMarks& marks, const CreatureRig& rig, const creature_skin::Art* art)
{
	const auto& baseSkins = sources.base->GetSkins();
	const auto& variantSkins = sources.variant->GetSkins();
	std::vector<uint32_t> baseOrder;
	std::vector<uint32_t> variantOrder;
	std::ranges::transform(baseSkins, std::back_inserter(baseOrder), &l3d::L3DTexture::id);
	std::ranges::transform(variantSkins, std::back_inserter(variantOrder), &l3d::L3DTexture::id);

	skin.skins.resize(baseSkins.size());
	for (size_t i = 0; i < baseSkins.size(); ++i)
	{
		const auto& base = baseSkins[i];
		auto& painted = skin.skins[i];
		painted.id = base.id;
		painted.texels.resize(base.texels.size());
		// A neutral creature, or one whose evil or good mesh lacks the skin, starts from its base skin
		std::span<const uint16_t> variant;
		if (sources.variant != sources.base)
		{
			if (const auto paired = creature_skin::PairedSkin(baseOrder, variantOrder, base.id); paired.has_value())
			{
				const auto found = std::ranges::find(variantSkins, *paired, &l3d::L3DTexture::id);
				if (found != variantSkins.end())
				{
					variant = TexelsOf(*found);
				}
			}
		}
		creature_skin::Compose(painted.texels, TexelsOf(base), variant, weight,
		                       {.skinIndex = static_cast<uint8_t>(i),
		                        .tattoos = tattoos.slots,
		                        .sites = rig.tattooSites,
		                        .marks = marks.marks,
		                        .art = art});
	}
	++skin.revision;
}
} // namespace

void CreatureSkinSystem::Update()
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& resources = Locator::resources::value();
	const auto& rigs = resources.GetCreatureRigs();
	const auto& arts = resources.GetCreatureSkinArt();
	const creature_skin::Art* art = arts.Contains(creature_skin::k_ArtId) ? &*arts.Handle(creature_skin::k_ArtId) : nullptr;

	registry.Each<const Creature, const CreatureMorph, const CreatureTattoos, const CreatureMarks, CreatureSkin>(
	    [&](const Creature& creature, const CreatureMorph& morph, const CreatureTattoos& tattoos, const CreatureMarks& marks,
	        CreatureSkin& skin) {
		    const auto rigId = creature::GetRigId(creature.species);
		    if (!rigs.Contains(rigId))
		    {
			    return;
		    }
		    const auto& rig = *rigs.Handle(rigId);
		    // The skins follow the alignment the body is drawn with, which moves on only past the morph threshold
		    const auto sources = SourcesOf(rig, morph.drawn.evilGood);
		    if (!sources)
		    {
			    return;
		    }
		    const CreatureSkin::Painted wanted {
		        .baseMesh = sources->baseId,
		        .variantMesh = sources->variantId,
		        .weight = creature_skin::BlendWeight(morph.drawn.evilGood),
		        .tattoos = tattoos.revision,
		        .marks = marks.revision,
		        .art = art != nullptr,
		    };
		    if (skin.painted == wanted)
		    {
			    return;
		    }
		    Paint(skin, *sources, wanted.weight, tattoos, marks, rig, art);
		    skin.painted = wanted;
	    });
}

void CreatureSkinSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<CreatureMarks>([](CreatureMarks& marks) {
		if (creature_marks::Heal(marks.marks, creature_marks::k_CountsPerTurn))
		{
			++marks.revision;
		}
	});
}

void CreatureSkinSystem::SetTattoo(entt::entity creature, size_t slot, const creature_tattoo::Slot& tattoo)
{
	auto* tattoos = Locator::entitiesRegistry::value().TryGet<CreatureTattoos>(creature);
	if (tattoos == nullptr || slot >= tattoos->slots.size() || tattoos->slots.at(slot) == tattoo)
	{
		return;
	}
	tattoos->slots.at(slot) = tattoo;
	++tattoos->revision;
}

void CreatureSkinSystem::AddWound(entt::entity creature, const creature_marks::Mark& wound)
{
	if (auto* marks = Locator::entitiesRegistry::value().TryGet<CreatureMarks>(creature); marks != nullptr)
	{
		creature_marks::Add(marks->marks.wounds, wound);
		++marks->revision;
	}
}

void CreatureSkinSystem::AddBlood(entt::entity creature, const creature_marks::Mark& blood)
{
	if (auto* marks = Locator::entitiesRegistry::value().TryGet<CreatureMarks>(creature); marks != nullptr)
	{
		creature_marks::Add(marks->marks.blood, blood);
		++marks->revision;
	}
}

void CreatureSkinSystem::Heal(entt::entity creature, uint32_t counts)
{
	if (auto* marks = Locator::entitiesRegistry::value().TryGet<CreatureMarks>(creature);
	    marks != nullptr && creature_marks::Heal(marks->marks, counts))
	{
		++marks->revision;
	}
}
