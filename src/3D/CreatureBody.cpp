/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <spdlog/spdlog.h>

#include "Common/StringUtils.h"
#include "ECS/Components/Creature.h"
#include "Enums.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::creature;

using S = CreatureType;
const std::map<std::string, CreatureType> k_MeshNameToSpecies = {
    {"c_cow", S::Cow},        {"a_tiger2", S::Tiger},         {"c_leopard", S::Leopard},   {"c_wolf", S::Wolf},
    {"c_lion", S::Lion},      {"a_horse", S::Horse},          {"c_tortoise", S::Tortoise}, {"c_zebra", S::Zebra},
    {"a_bear", S::BrownBear}, {"c_polar_bear", S::PolarBear}, {"c_sheep", S::Sheep},       {"c_chimp", S::Chimp},
    {"a_greek", S::Ogre},     {"c_mandrill", S::Mandrill},    {"c_rhino", S::Rhino},       {"c_gorilla", S::Gorilla},
    {"c_ape", S::GiantApe}};

using A = CreatureBody::Appearance;
const std::map<std::string, CreatureBody::Appearance> k_MeshNameToAppearance = {
    {"boned", A::Base}, {"base", A::Base},     {"good2", A::Good}, {"base2", A::Base}, {"good", A::Good}, {"evil", A::Evil},
    {"evil2", A::Evil}, {"strong", A::Strong}, {"weak", A::Weak},  {"fat", A::Fat},    {"thin", A::Thin}};

CreatureType creature::GetSpeciesFromMeshName(const std::string& name)
{
	for (const auto& [creatureTerm, species] : k_MeshNameToSpecies)
	{
		if (string_utils::BeginsWith(name, creatureTerm))
		{
			return species;
		}
	}

	return CreatureType::Unknown;
}

CreatureBody::Appearance creature::GetAppearanceFromMeshName(const std::string& name)
{
	for (const auto& [creatureTerm, appearance] : k_MeshNameToAppearance)
	{
		if (string_utils::EndsWith(name, creatureTerm))
		{
			return appearance;
		}
	}

	return CreatureBody::Appearance::Unknown;
};

entt::id_type creature::GetIdFromType(CreatureType species)
{
	auto defaultBodyType = CreatureBody::Appearance::Base;
	return entt::hashed_string(
	    fmt::format("creature/{}/{}", static_cast<uint8_t>(species), static_cast<uint32_t>(defaultBodyType)).c_str());
}

entt::id_type creature::GetIdFromMeshName(const std::string& name)
{
	const std::string lowerCaseName = string_utils::LowerCase(name);
	auto species = creature::GetSpeciesFromMeshName(lowerCaseName);
	if (species == CreatureType::Unknown)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unknown creature species: {}", name);
	}

	auto appearance = creature::GetAppearanceFromMeshName(lowerCaseName);

	if (appearance == CreatureBody::Appearance::Unknown)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unknown creature appearance: {}", name);
	}

	return entt::hashed_string(
	    fmt::format("creature/{}/{}", static_cast<uint8_t>(species), static_cast<uint32_t>(appearance)).c_str());
}
