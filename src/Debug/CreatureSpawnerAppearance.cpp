/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <array>
#include <random>
#include <string>

#include <fmt/format.h>

#include "3D/CreatureBody.h"
#include "Creature/CreatureMarks.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureSkin.h"
#include "Creature/CreatureTattoo.h"
#include "CreatureSpawner.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureSkin.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "EngineConfig.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureMarks;
using openblack::ecs::components::CreatureTattoos;

namespace
{
/// The kinds of wound fights leave, and the burns fire leaves, strong and weak
constexpr std::array<uint8_t, 3> k_FightWounds {3, 4, 5};
constexpr uint8_t k_StrongBurn = 1;
/// A trail of blood runs this many texels down the skin
constexpr int k_BloodTrailLength = 12;

std::string SlotLabel(size_t index, const creature_tattoo::Slot& slot)
{
	if (slot.Empty())
	{
		return fmt::format("{}: empty", index);
	}
	return fmt::format("{}: design {} on site {}, #{:02X}{:02X}{:02X}", index, slot.design, slot.site, slot.colour.r,
	                   slot.colour.g, slot.colour.b);
}
} // namespace

void CreatureSpawner::DrawAppearance(entt::entity entity) noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* creature = registry.TryGet<const Creature>(entity);
	const auto* tattoos = registry.TryGet<const CreatureTattoos>(entity);
	const auto* marks = registry.TryGet<const CreatureMarks>(entity);
	if (creature == nullptr || tattoos == nullptr || marks == nullptr || !Locator::creatureSkinSystem::has_value())
	{
		return;
	}
	auto& skins = Locator::creatureSkinSystem::value();
	auto& resources = Locator::resources::value();
	const auto& rigs = resources.GetCreatureRigs();
	const auto rigId = creature::GetRigId(creature->species);
	const auto* sites =
	    rigs.Contains(rigId) && rigs.Handle(rigId)->tattooSites.has_value() ? &*rigs.Handle(rigId)->tattooSites : nullptr;
	const auto& arts = resources.GetCreatureSkinArt();
	const auto* art = arts.Contains(creature_skin::k_ArtId) ? &*arts.Handle(creature_skin::k_ArtId) : nullptr;

	ImGui::SeparatorText("Tattoos");
	if (sites == nullptr)
	{
		ImGui::TextUnformatted("The species has no places for tattoos");
	}
	for (size_t i = 0; i < tattoos->slots.size(); ++i)
	{
		const auto& slot = tattoos->slots.at(i);
		if (ImGui::Selectable(SlotLabel(i, slot).c_str(), static_cast<size_t>(_tattooSlot) == i))
		{
			_tattooSlot = static_cast<int>(i);
			if (!slot.Empty())
			{
				_tattooDesign = slot.design;
				_tattooSite = slot.site;
			}
		}
	}
	ImGui::SliderInt("Design", &_tattooDesign, 0, static_cast<int>(creature_tattoo::k_DesignCount) - 1);
	const auto siteLabel = [sites](int site) {
		if (sites == nullptr)
		{
			return fmt::format("{}", site);
		}
		const auto& place = sites->at(static_cast<size_t>(site));
		return place.enabled
		           ? fmt::format("{} at {}, {} of skin {}, size {:.2f}", site, place.u, place.v, place.skin, place.size)
		           : fmt::format("{} (not on this species)", site);
	};
	ImGui::SliderInt("Site", &_tattooSite, 0, static_cast<int>(creature_tattoo::k_SlotCount) - 1,
	                 siteLabel(_tattooSite).c_str());
	ImGui::SliderInt("Palette column", &_paletteColumn, 0, static_cast<int>(creature_tattoo::k_PaletteColumns) - 1);
	ImGui::SliderInt("Palette row", &_paletteRow, 0, static_cast<int>(creature_tattoo::k_PaletteRows) - 1);
	ImGui::SliderFloat("Brightness", &_tattooBrightness, 0.0f, 1.0f, "%.2f");
	const auto colour = art != nullptr ? creature_tattoo::PaletteColour(art->palette, static_cast<uint32_t>(_paletteColumn),
	                                                                    static_cast<uint32_t>(_paletteRow), _tattooBrightness)
	                                   : glm::u8vec3(255);
	ImGui::ColorButton("Colour", ImVec4(colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, 1.0f));
	ImGui::SameLine();
	const creature_tattoo::Slot edited {
	    .design = static_cast<uint8_t>(_tattooDesign),
	    .site = static_cast<uint8_t>(_tattooSite),
	    .colour = colour,
	};
	if (ImGui::Button("Set slot"))
	{
		skins.SetTattoo(entity, static_cast<size_t>(_tattooSlot), edited);
	}
	ImGui::SameLine();
	if (ImGui::Button("Put on"))
	{
		// As the game's tattoo editor drops a design on a site
		if (const auto slot = creature_tattoo::SlotFor(tattoos->slots, edited.site, edited.design); slot.has_value())
		{
			skins.SetTattoo(entity, *slot, edited);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Into the slot with this design on this site, else the first empty one, else the one on this site");
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear slot"))
	{
		skins.SetTattoo(entity, static_cast<size_t>(_tattooSlot), creature_tattoo::Slot {});
	}

	ImGui::SeparatorText("Wounds and blood");
	ImGui::Text("%zu wounds and burns, %zu drops of blood, healed %u of %u", marks->marks.wounds.size(),
	            marks->marks.blood.size(), marks->marks.counts, creature_marks::k_CountsPerStep);
	ImGui::Checkbox("Anywhere", &_randomMarkPlace);
	if (!_randomMarkPlace)
	{
		ImGui::SliderInt("U", &_markU, 0, 255);
		ImGui::SliderInt("V", &_markV, 0, 255);
		ImGui::SliderInt("Skin", &_markSkin, 0, 3);
	}
	ImGui::SliderInt("Kind", &_woundType, 0, 7);
	ImGui::SliderInt("Column", &_woundColumn, 0, 7);
	const auto place = [this](uint8_t type, uint8_t column) {
		creature_marks::Mark mark {.u = static_cast<uint8_t>(_markU),
		                           .v = static_cast<uint8_t>(_markV),
		                           .skin = static_cast<uint8_t>(_markSkin),
		                           .age = 0,
		                           .type = type,
		                           .column = column};
		if (_randomMarkPlace)
		{
			std::uniform_int_distribution<int> texel(16, 239);
			mark.u = static_cast<uint8_t>(texel(_random));
			mark.v = static_cast<uint8_t>(texel(_random));
		}
		return mark;
	};
	if (ImGui::Button("Add wound"))
	{
		const auto type = k_FightWounds.at(std::uniform_int_distribution<size_t>(0, k_FightWounds.size() - 1)(_random));
		skins.AddWound(entity, place(type, static_cast<uint8_t>(_woundColumn)));
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of one of the kinds a fight leaves, from the column picked");
	}
	ImGui::SameLine();
	if (ImGui::Button("Add burn"))
	{
		skins.AddWound(entity, place(k_StrongBurn, static_cast<uint8_t>(_woundColumn)));
	}
	ImGui::SameLine();
	if (ImGui::Button("Add of kind"))
	{
		skins.AddWound(entity, place(static_cast<uint8_t>(_woundType), static_cast<uint8_t>(_woundColumn)));
	}
	if (ImGui::Button("Add blood"))
	{
		// A trail down the skin from the place; the game runs drops of blood down the posed body instead
		auto drop = place(0, 0);
		for (int i = 0; i < k_BloodTrailLength && drop.v < 255; ++i, ++drop.v)
		{
			skins.AddBlood(entity, drop);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Heal a step"))
	{
		skins.Heal(entity, creature_marks::k_CountsPerStep);
	}
	ImGui::SameLine();
	if (ImGui::Button("Heal effect"))
	{
		skins.Heal(entity, creature_marks::k_HealEffectCounts);
	}

	ImGui::SeparatorText("Drawing");
	auto& config = Locator::config::value();
	ImGui::Checkbox("Blend seams", &config.blendCreatureSeams);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Vertices where the bones meet are drawn part of the way to their partners, as the game does");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Shadows", &config.drawCreatureShadows);
}
