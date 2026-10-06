/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellBehaviours.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include "ECS/Components/Spell.h"
#include "InfoConstants.h"
#include "MagicTables.h"
#include "MagicWorldInterface.h"
#include "SpellRules.h"

using namespace openblack;
using namespace openblack::magic;
using openblack::ecs::components::Spell;
using openblack::particles::SpellEventInfo;

namespace
{
/// A drop of the water miracle's rain lands this far above the land
constexpr float k_DropHeight = 0.2f;
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;

bool IsFireball(MagicType type)
{
	return type == MagicType::Fireball || type == MagicType::FireballPowerUpOne || type == MagicType::FireballPowerUpTwo;
}

/// How far the heal reaches and how many it heals, by its tables and the caster's tribal power
struct HealReach
{
	float radius;
	size_t maximum;
};
std::optional<HealReach> HealReachOf(const InfoConstants& info, MagicType type, float tribalPower)
{
	const auto* heal = GetMagicInfoAs<GMagicHealInfo>(info, type);
	if (heal == nullptr)
	{
		return std::nullopt;
	}
	const auto maximum = heal->maxToHeal != 0 ? heal->maxToHeal : k_DefaultHealTargets;
	return HealReach {
	    .radius = heal->dummyVar * tribalPower,
	    .maximum = static_cast<size_t>(std::lrint(static_cast<float>(maximum) * tribalPower)),
	};
}

float PayEvent(SpellServicesInterface& services, Spell& spell)
{
	return magic::PayForOneEvent(spell.chants, spells::RulesOf(services, spell), services.CasterOf(spell));
}

bool HasAnyEffect(const EffectValues& values)
{
	return std::ranges::any_of(values.numbers, [](float number) { return number != 0.0f; });
}

/// The food and wood miracles: each grain or log that lands pays for what it brings, which is put down there on dry
/// land while the miracle has strength left
bool ApplyResourceEvent(SpellServicesInterface& services, Spell& spell, const SpellEventInfo& event)
{
	const bool acted = spells::ApplyDefaultEffect(services, spell, event);
	if (spell.closedDown)
	{
		return acted;
	}
	const auto& info = services.Info();
	const auto* resource = GetMagicInfoAs<GMagicResourceInfo>(info, spell.magicType);
	if (resource == nullptr)
	{
		return acted;
	}
	const auto cost = ResourceEvent(*resource, spell.resourceFirstDone);
	magic::PayFor(spell.chants, spells::RulesOf(services, spell), services.CasterOf(spell), cost.chants);
	spell.resourceFirstDone = true;
	auto& world = services.World();
	if (!world.InBounds(event.position) || !world.IsDryLand(event.position) || !(spells::StrengthOf(services, spell) > 0.0f))
	{
		// Paid for, but nothing lands: in the sea, off the map or with nothing left
		return acted;
	}
	const auto amount = static_cast<uint32_t>(services.TribalPower(spell) * static_cast<float>(cost.units));
	// The power-up's "poisoned" flag makes its food sparkle: the people who eat it work faster
	const bool sparkles = resource->resourceType == ResourceType::Food && resource->poisoned == 1;
	world.AddResource(resource->resourceType, event.position, amount, sparkles);
	return true;
}
} // namespace

SpellChantRules spells::RulesOf(SpellServicesInterface& services, const Spell& spell)
{
	const auto& info = services.Info();
	auto rules = ChantRulesFor(spell.magicType, GetMagicInfo(info, spell.magicType), GetMagicEffectInfo(info, spell.magicType));
	rules.costToMaintain = CostToMaintain(info, spell);
	rules.tribalPower = services.TribalPower(spell);
	rules.seedPower = services.SeedPower(spell);
	return rules;
}

float spells::StrengthOf(SpellServicesInterface& services, const Spell& spell)
{
	return GetSpellStrength(spell.chants, RulesOf(services, spell), services.CasterOf(spell));
}

float spells::CostToMaintain(const InfoConstants& info, const Spell& spell)
{
	const float plain = GetMagicEffectInfo(info, spell.magicType).costPerGameTurn;
	if (const auto* shield = GetMagicInfoAs<GMagicShieldInfo>(info, spell.magicType))
	{
		return ShieldCostToMaintain(plain, spell.magnitude, shield->radiusForNormalCost);
	}
	return plain;
}

bool spells::CanCastAt(SpellServicesInterface& services, MagicType type, PlayerNames player, glm::vec3 point,
                       bool ignoreInfluence)
{
	auto& world = services.World();
	if (!world.InBounds(point))
	{
		return false;
	}
	const auto& info = services.Info();
	const bool onLand = world.IsDryLand(point);
	const bool inInfluence = ignoreInfluence || world.InInfluence(player, point);
	switch (GetMagicInfo(info, type).castRuleType)
	{
	case CastRuleType::OnLand:
		if (!onLand)
		{
			return false;
		}
		break;
	case CastRuleType::InInfluence:
		if (!inInfluence)
		{
			return false;
		}
		break;
	case CastRuleType::OnLandInInfluence:
		if (!onLand || !inInfluence)
		{
			return false;
		}
		break;
	case CastRuleType::Anywhere:
		break;
	}
	switch (ClassOf(type))
	{
	case SpellClass::Heal:
	{
		const auto reach = HealReachOf(info, type, 1.0f);
		return reach.has_value() && !world.HealTargets(point, reach->radius, reach->maximum).empty();
	}
	case SpellClass::Resource:
		return onLand;
	case SpellClass::Creature:
		return false;
	default:
		return true;
	}
}

void spells::Prepare(SpellServicesInterface& services, Spell& spell)
{
	if (const auto* shield = GetMagicInfoAs<GMagicShieldInfo>(services.Info(), spell.magicType))
	{
		spell.magnitude = ClampShieldRadius(*shield, spell.magnitude);
	}
	spell.resourceFirstDone = false;
	spell.lastRipple = 0.0f;
}

bool spells::Start(SpellServicesInterface& services, Spell& spell)
{
	if (spell.spellClass == SpellClass::Heal)
	{
		// The people round the point are given to its chakras; a heal with nobody to heal still plays
		if (const auto reach = HealReachOf(services.Info(), spell.magicType, services.TribalPower(spell)))
		{
			for (const auto target : services.World().HealTargets(spell.castPosition, reach->radius, reach->maximum))
			{
				services.AddEffectTarget(spell, target);
			}
		}
	}
	return true;
}

bool spells::OnEvent(SpellServicesInterface& services, Spell& spell, const SpellEventInfo& event)
{
	if (event.type == SpellEventInfo::Type::Started)
	{
		return true;
	}
	switch (spell.spellClass)
	{
	case SpellClass::Resource:
		return ApplyResourceEvent(services, spell, event);
	case SpellClass::General:
		if (IsFireball(spell.magicType) && event.type == SpellEventInfo::Type::Point)
		{
			const bool acted = ApplyDefaultEffect(services, spell, event);
			if (acted)
			{
				const auto& effect = GetMagicEffectInfo(services.Info(), spell.magicType);
				services.World().Heat(event.position, effect.radius,
				                      FireballTemperature(services.Info(), spell.magicType) * StrengthOf(services, spell));
			}
			return acted;
		}
		break;
	default:
		break;
	}
	return ApplyDefaultEffect(services, spell, event);
}

bool spells::ApplyDefaultEffect(SpellServicesInterface& services, Spell& spell, const SpellEventInfo& event)
{
	if (spell.closedDown)
	{
		return false;
	}
	spell.position = event.position;
	auto values = EffectValues::From(GetMagicEffectInfo(services.Info(), spell.magicType));
	const float strength = PayEvent(services, spell);
	if (!(strength > 0.0f))
	{
		return false;
	}
	// The strength already carries the tribal power
	values.Scale(strength * event.strength);
	auto& world = services.World();
	if (event.type == SpellEventInfo::Type::HitSpell)
	{
		auto* other = event.target != entt::null ? services.FindSpell(event.target) : nullptr;
		if (other != nullptr && other != &spell && !StrikeSpell(services, spell, *other))
		{
			return false;
		}
	}
	else
	{
		if (event.checkShields)
		{
			if (const auto shield = services.ShieldAt(event.position, values.radius))
			{
				auto* other = services.FindSpell(*shield);
				if (other != nullptr && other != &spell && !StrikeSpell(services, spell, *other))
				{
					return false;
				}
				services.StrikeShield(*shield, event.position);
			}
		}
		if (HasAnyEffect(values))
		{
			if (event.target != entt::null && world.PositionOf(event.target).has_value())
			{
				world.ApplyEffect(event.target, values);
			}
			else
			{
				world.ApplyEffectAt(event.position, values);
			}
		}
	}
	spell.movement = event.velocity;
	return true;
}

bool spells::StrikeSpell(SpellServicesInterface& services, Spell& spell, Spell& other)
{
	if (!(StrengthOf(services, other) > 0.0f))
	{
		return true;
	}
	const float cost = StrengthOf(services, spell) * GetMagicEffectInfo(services.Info(), spell.magicType).costPerShieldCollide;
	magic::PayFor(other.chants, RulesOf(services, other), services.CasterOf(other), cost, Refill::Whole);
	PayEvent(services, spell);
	return !(StrengthOf(services, other) > 0.0f) && StrengthOf(services, spell) > 0.0f;
}

void spells::ProcessTurn(SpellServicesInterface& services, Spell& spell)
{
	if (spell.spellClass != SpellClass::Water || spell.closedDown)
	{
		return;
	}
	// One drop a turn at a random point round where it is cast
	auto& world = services.World();
	const float distance = DropDistance(services.GameRandom(RainRadius(spell.magicType)));
	const float angle = services.GameRandom(k_TwoPi);
	glm::vec3 drop(spell.castPosition.x + distance * std::cos(angle), 0.0f, spell.castPosition.z + distance * std::sin(angle));
	drop.y = world.LandHeight({drop.x, drop.z}) + k_DropHeight;
	ApplyDefaultEffect(services, spell,
	                   {.type = SpellEventInfo::Type::Point,
	                    .position = drop,
	                    .velocity = glm::vec3(0.0f),
	                    .strength = 1.0f,
	                    .checkShields = false,
	                    .target = entt::null});
	std::optional<float> ring;
	if (spell.age - spell.lastRipple > k_RippleEvery)
	{
		spell.lastRipple = spell.age;
		ring = RippleGrowth(spell.magicType);
	}
	world.Water(drop, k_WaterReach, ring);
}

bool spells::HasEnoughForRecast(const InfoConstants& info, const Spell& spell)
{
	if (const auto* resource = GetMagicInfoAs<GMagicResourceInfo>(info, spell.magicType))
	{
		return HasEnoughChantsForResourceRecast(*resource, spell.chants.chants);
	}
	return true;
}

float spells::FireballTemperature(const InfoConstants& info, MagicType type)
{
	// The plain fireball and its second power-up burn as the first row, the first power-up as the second
	const size_t row = GetPowerUpLevel(info, type) == 0 ? 1 : 0;
	return info.magicFireBall.at(row).initialTemperature;
}
