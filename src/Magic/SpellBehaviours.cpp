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

#include "AreaEffect.h"
#include "ECS/Components/Spell.h"
#include "ForestRules.h"
#include "InfoConstants.h"
#include "MagicTables.h"
#include "MagicWorldInterface.h"
#include "Particles/StormMaths.h"
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

bool IsFlock(SpellClass spellClass)
{
	return spellClass == SpellClass::FlockFlying || spellClass == SpellClass::FlockGround;
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
	// The power-up's food makes a new pile sparkle: the people who take from it go faster
	const bool speedUp = resource->resourceType == ResourceType::Food && resource->speedUp == 1;
	world.AddResource(resource->resourceType, event.position, amount, speedUp, spell.caster.player);
	return true;
}
/// The forest: as its seed lands (its only event after starting) it pays for the event and, once, plants all its trees
/// round where it was cast
bool ApplyForestEvent(SpellServicesInterface& services, Spell& spell, const SpellEventInfo& event)
{
	if (spell.forestPlanted)
	{
		return false;
	}
	if (!spells::ApplyDefaultEffect(services, spell, event))
	{
		return false;
	}
	return services.PlantForest(spell);
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
	const auto& effect = GetMagicEffectInfo(info, spell.magicType);
	const float plain = effect.costPerGameTurn;
	if (spell.spellClass == SpellClass::Forest)
	{
		// Each of its trees costs as much as an event
		return forest::Upkeep(plain, effect.costPerEvent, spell.objectCount);
	}
	if (const auto* shield = GetMagicInfoAs<GMagicShieldInfo>(info, spell.magicType))
	{
		return ShieldCostToMaintain(plain, spell.magnitude, shield->radiusForNormalCost);
	}
	// A storm costs more the wider it is
	if (const auto* storm = GetMagicInfoAs<GMagicStormAndTornadoInfo>(info, spell.magicType))
	{
		return particles::storm::CostToMaintain(plain, spell.magnitude, storm->radiusForNormalCost);
	}
	return plain;
}

bool spells::MeetsCastRule(SpellServicesInterface& services, MagicType type, PlayerNames player, glm::vec3 point,
                           bool ignoreInfluence)
{
	auto& world = services.World();
	if (!world.InBounds(point))
	{
		return false;
	}
	const auto inInfluence = [&] { return ignoreInfluence || world.InInfluence(player, point); };
	switch (GetMagicInfo(services.Info(), type).castRuleType)
	{
	case CastRuleType::OnLand:
		return world.IsLand(point);
	case CastRuleType::InInfluence:
		return inInfluence();
	case CastRuleType::OnLandInInfluence:
		return world.IsLand(point) && inInfluence();
	case CastRuleType::Anywhere:
		break;
	}
	return true;
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
	// Land, for a cast rule and for the kinds that must go on land, is a cell without water in it
	const bool onLand = world.IsLand(point);
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
	case SpellClass::Teleport:
		// Not where something stands fixed on the land, a building, a field or another stone
		return services.CanPlaceTeleportStone(point);
	case SpellClass::Forest:
		return onLand && services.ForestCanGrowAt(point);
	case SpellClass::Creature:
		return false;
	default:
		return true;
	}
}

bool spells::CanCastAtPointItself(SpellServicesInterface& services, MagicType type, glm::vec3 point)
{
	if (ClassOf(type) != SpellClass::Heal)
	{
		return true;
	}
	const auto reach = HealReachOf(services.Info(), type, 1.0f);
	return reach.has_value() && !services.World().HealTargets(point, reach->radius, reach->maximum).empty();
}

void spells::Prepare(SpellServicesInterface& services, Spell& spell)
{
	if (const auto* shield = GetMagicInfoAs<GMagicShieldInfo>(services.Info(), spell.magicType))
	{
		spell.magnitude = ClampShieldRadius(*shield, spell.magnitude);
	}
	// A storm is as wide as the circle that cast it, within its limits
	if (const auto* storm = GetMagicInfoAs<GMagicStormAndTornadoInfo>(services.Info(), spell.magicType))
	{
		spell.magnitude = std::clamp(spell.magnitude, storm->minRadius, storm->maxRadius);
	}
	spell.resourceFirstDone = false;
	spell.lastRipple = 0.0f;
}

bool spells::Start(SpellServicesInterface& services, Spell& spell)
{
	// A storm's swirl plays where it was cast
	if (spell.spellClass == SpellClass::StormAndTornado)
	{
		services.StartCastEffect(spell, ParticleType::StormCast);
	}
	if (IsFlock(spell.spellClass))
	{
		if (auto* flocks = services.Flocks())
		{
			flocks->Start(services, spell);
		}
	}
	if (spell.spellClass == SpellClass::Teleport)
	{
		// Its stone, which carries its pool; it has no effect of its own
		return services.CreateTeleportStone(spell) != entt::null;
	}
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
	// Starting, with or without an effect, does nothing more by itself
	if (event.type == SpellEventInfo::Type::Started || event.type == SpellEventInfo::Type::InitWithoutEffect)
	{
		return true;
	}
	switch (spell.spellClass)
	{
	case SpellClass::Resource:
		return ApplyResourceEvent(services, spell, event);
	case SpellClass::Forest:
		return ApplyForestEvent(services, spell, event);
	case SpellClass::Heal:
	{
		// The heal also cures whoever it reaches of poison
		const bool acted = ApplyDefaultEffect(services, spell, event);
		if (acted && event.type == SpellEventInfo::Type::Object && event.target != entt::null)
		{
			services.World().CurePoison(event.target);
		}
		return acted;
	}
	case SpellClass::General:
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
	auto& world = services.World();
	// The event's point is taken on the land under it
	const glm::vec3 point(event.position.x, world.LandHeight({event.position.x, event.position.z}), event.position.z);
	spell.position = point;
	auto values = EffectValues::From(GetMagicEffectInfo(services.Info(), spell.magicType));
	const float strength = PayEvent(services, spell);
	if (!(strength > 0.0f))
	{
		return false;
	}
	// The strength paid for already carries the caster's tribal power; it counts again, and the event's own strength
	values = EventEffectValues(values, strength, services.TribalPower(spell), event.strength);
	const auto source = SourceOf(spell);
	bool acted = true;
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
				// As in the game, the miracle strikes itself rather than the shield it found, paying for one more event
				// as it does: unless it has nothing left to pay with, it is stopped, the shield neither sparking nor
				// weakened. With nothing to pay the shield sparks and the event goes on.
				if (PayEvent(services, spell) > 0.0f)
				{
					StrikeSpell(services, spell, spell);
					return false;
				}
				services.StrikeShield(*shield, event.position);
			}
		}
		// Asked to pick something up, as a tornado does, the miracle only says whether it may destroy it
		if (event.type == SpellEventInfo::Type::Capture)
		{
			if (event.target != entt::null)
			{
				return world.CanBeDestroyedBySpell(event.target);
			}
			acted = false;
		}
		else
		{
			std::optional<entt::entity> reached;
			if (event.type == SpellEventInfo::Type::Object && event.target != entt::null &&
			    world.PositionOf(event.target).has_value())
			{
				if (world.ApplyEffect(event.target, values, source))
				{
					reached = event.target;
				}
			}
			else
			{
				// Everything in reach takes it, even an effect of no strength
				const auto all = world.ApplyEffectAt(point, values, source);
				if (!all.empty())
				{
					reached = all.back();
				}
			}
			// What a player's miracle (not a creature's) did to the last thing it reached, the player's creature may copy
			if (reached.has_value() && spell.caster.kind != ecs::components::SpellCaster::Kind::Creature)
			{
				world.PlayerAffected(*reached, spell.magicType, source.player);
			}
		}
	}
	if (GetMagicEffectInfo(services.Info(), spell.magicType).createReactionOnEvent != 0)
	{
		services.ReactToSpell(spell, false);
	}
	spell.movement = event.velocity;
	return acted;
}

bool spells::StrikeSpell(SpellServicesInterface& services, Spell& spell, Spell& other)
{
	if (!(StrengthOf(services, other) > 0.0f))
	{
		return true;
	}
	const float cost = StrengthOf(services, spell) * GetMagicEffectInfo(services.Info(), spell.magicType).costPerShieldCollide;
	// Paid as the game forces it: the caster is asked for the whole shortfall
	magic::PayFor(other.chants, RulesOf(services, other), services.CasterOf(other), cost, Refill::Whole);
	PayEvent(services, spell);
	const bool destroyed = !(StrengthOf(services, other) > 0.0f) && StrengthOf(services, spell) > 0.0f;
	if (&other != &spell && other.spellClass == SpellClass::Shield)
	{
		services.ShieldStruck(other, destroyed);
	}
	return destroyed;
}

void spells::ProcessTurn(SpellServicesInterface& services, Spell& spell)
{
	if (IsFlock(spell.spellClass))
	{
		if (auto* flocks = services.Flocks())
		{
			flocks->ProcessTurn(services, spell);
		}
		return;
	}
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
	world.Water({.position = drop,
	             .reach = k_WaterReach,
	             .ringGrowth = ring,
	             .extreme = spell.magicType == MagicType::WaterPowerUpOne,
	             .spell = services.SpellEntity(spell),
	             .source = SourceOf(spell)});
}

bool spells::KeptByKind(SpellServicesInterface& services, const Spell& spell)
{
	// A flock lives on while its animals fly or run, fading out
	if (IsFlock(spell.spellClass))
	{
		const auto* flocks = services.Flocks();
		return flocks != nullptr && flocks->AnimalsLeft(spell);
	}
	return (spell.spellClass == SpellClass::Shield || spell.spellClass == SpellClass::Forest) &&
	       services.HasWorldObjects(spell);
}

ParticleType spells::ParticleTypeOf(SpellServicesInterface& services, const Spell& spell)
{
	if (IsFlock(spell.spellClass))
	{
		if (const auto* flocks = services.Flocks())
		{
			return flocks->ParticleTypeOf(spell);
		}
	}
	return GetMagicInfo(services.Info(), spell.magicType).particleType;
}

bool spells::FollowsHand(SpellServicesInterface& services, const Spell& spell)
{
	const auto* flocks = IsFlock(spell.spellClass) ? services.Flocks() : nullptr;
	return flocks != nullptr && flocks->FollowsHand(services, spell);
}

EffectSource spells::SourceOf(const Spell& spell)
{
	return {.player = spell.caster.player,
	        .casterCreature =
	            spell.caster.kind == ecs::components::SpellCaster::Kind::Creature ? spell.caster.entity : entt::null};
}

bool spells::HasEnoughForRecast(const InfoConstants& info, const Spell& spell)
{
	if (const auto* resource = GetMagicInfoAs<GMagicResourceInfo>(info, spell.magicType))
	{
		return HasEnoughChantsForResourceRecast(*resource, spell.chants.chants);
	}
	if (const auto* trees = GetMagicInfoAs<GMagicForestInfo>(info, spell.magicType))
	{
		const auto standing =
		    spell.forestPlanted && spell.objectCount > 0 ? std::optional<uint32_t>(spell.objectCount) : std::nullopt;
		return forest::MaxObjectsToCreate(spell.maxObjectsToCreate, trees->finalNoTrees, spell.forestPlanted, standing) > 0;
	}
	return true;
}

float spells::FireballTemperature(const InfoConstants& info, MagicType /*type*/)
{
	// Every fireball burns as the first row of the fireball's table: the game picks the row by a power-up type that no
	// fireball has, so the power-ups throw more balls rather than hotter ones
	return info.magicFireBall.at(0).initialTemperature;
}
