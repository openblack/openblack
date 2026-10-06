/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The shield miracle's rules: the sphere it raises, the sparks where something strikes it, the spin and surface of the
// dome, its collections' alpha, and particles of other effects turned away by it

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleMiracleMaths.h"
#include "ParticleShields.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The shortest step the rules divide a move by
constexpr float k_MinimumStep = 1e-4f;
/// A move smaller than this along every axis doesn't turn the dome's surface
constexpr float k_StillMove = 1e-4f;
/// A spark's arc wiggles between two and five times across
constexpr float k_SparkWigglesMin = 2.0f;
constexpr float k_SparkWigglesRange = 4.0f;
/// The spark arc's joints scale with the shield, pulsing
constexpr float k_SparkScale = 0.04f;
constexpr float k_SparkPulseBase = 1.5f;
constexpr float k_SparkPulse = 0.4f;
constexpr float k_SparkPulseSpeed = 6.0f;
/// A spark fades out over its life, from twice full
constexpr float k_SparkAlphaScale = 510.0f;
/// The dome's surface fades in by this much a second
constexpr float k_VapourFadeIn = 30.0f;

/// The rotation that turns the vertical towards a point from the centre
glm::mat3 OrientAlong(const glm::vec3& p, const glm::mat3& start)
{
	const float yaw = std::atan2(p.z, p.x);
	const float colatitude = std::numbers::pi_v<float> / 2.0f - std::atan2(p.y, std::sqrt(p.z * p.z + p.x * p.x));
	glm::mat3 m = start;
	const float cc = std::cos(colatitude);
	const float sc = std::sin(colatitude);
	const float cy = std::cos(yaw);
	const float sy = std::sin(yaw);
	for (int i = 0; i < 3; ++i)
	{
		const float x = m[i].x;
		const float y = m[i].y;
		m[i].x = cc * x + sc * y;
		m[i].y = cc * y - sc * x;
		const float x2 = m[i].x;
		const float z = m[i].z;
		m[i].x = cy * x2 - sy * z;
		m[i].z = cy * z + sy * x2;
	}
	return m;
}

/// The first time its collection runs, the shield's sphere at the parent atom (or the origin), as big as the radius
/// provider says; after that its radius follows the provider and its centre stays
class AddDefensiveSphere final: public Modifier
{
public:
	explicit AddDefensiveSphere(const ParticleObject& object)
	    : radius(object.String("SphereRadius"))
	{
	}

	[[nodiscard]] bool KeepsAlive() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (slot.first)
		{
			slot.first = false;
			auto shield = std::make_shared<ShieldSphere>();
			shield->centre = collection.parent != nullptr ? effect.GlobalPosition(*collection.parent) : effect.GetOrigin();
			shield->radius = effect.FloatProvider(radius, 0.0f);
			shield->spell = effect.GetSink() != nullptr ? effect.GetSink()->Spell() : entt::null;
			shield->owner = &effect;
			effect.Services().world.AddShield(shield);
			slot.data = shield;
			return true;
		}
		if (auto* shield = std::any_cast<std::shared_ptr<ShieldSphere>>(&slot.data); shield != nullptr && *shield)
		{
			(*shield)->radius = effect.FloatProvider(radius, (*shield)->radius);
		}
		return true;
	}

	std::string radius;
};

/// An atom not yet turned away that is inside a shield strikes it; unless its miracle gets through it bounces off,
/// taking the turned-away flag and closing its effect down when asked
class CheckShieldDeflections final: public Modifier
{
public:
	explicit CheckShieldDeflections(const ParticleObject& object)
	    : closeDownIfDeflected(object.Bool("CloseDownSpellIfDeflected", false))
	    , setDeflected(object.Bool("SetDeflectedWhenDeflected", false))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (atom.deflected)
		{
			return true;
		}
		const auto global = effect.GlobalPosition(atom);
		const auto shield = effect.Services().world.FindShield(global, 0.0f);
		if (!shield || shield->owner == &effect)
		{
			return true;
		}
		StrikeShield(*shield, global);
		const bool through = effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
		                                            .position = global,
		                                            .velocity = atom.velocity * effect.GetDt(),
		                                            .strength = 1.0f,
		                                            .checkShields = false,
		                                            .target = shield->spell});
		if (through)
		{
			return true;
		}
		atom.velocity = maths::DeflectOffSphere(global, shield->centre, atom.velocity);
		atom.deflected = atom.deflected || setDeflected;
		if (closeDownIfDeflected)
		{
			effect.CloseDown();
		}
		return true;
	}

	bool closeDownIfDeflected;
	bool setDeflected;
};

/// A spark for each place the shield was struck, while there are fewer than the most allowed: an arc of its atoms from
/// the centre to the strike, wiggling, pulsing and fading over its life, with its sound
class ShieldSpark final: public Modifier
{
public:
	explicit ShieldSpark(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , radius(object.String("SphereRadius"))
	    , maxAtoms(object.Int("MaxNumAtomsForCollection", -1))
	    , sparkLife(object.Float("SparkLife", 1.0f))
	    , wiggle(object.Float("WiggleAmpl", 0.0f))
	    , sound({.action = object.Sound("SoundSpark")})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* sparkCreator = effect.FindCreator(creator);
		if (sparkCreator == nullptr)
		{
			return false;
		}
		if (const auto shield = effect.Services().world.ShieldOf(effect))
		{
			for (const auto& impact : shield->impacts)
			{
				if (maxAtoms != -1 && static_cast<int>(collection.atoms.size()) >= maxAtoms)
				{
					break;
				}
				auto& atom = effect.NewAtom(collection, sparkCreator, nextGroups);
				atom.position = effect.GlobalToLocal(collection, impact);
				StartAtomSound(effect, atom, sound);
				// Three angles of its own and how many times it wiggles across
				atom.data[this].a = glm::vec4(effect.Random(k_TwoPi), effect.Random(k_TwoPi), effect.Random(k_TwoPi),
				                              std::floor(effect.Random(k_SparkWigglesRange) + k_SparkWigglesMin));
			}
			shield->impacts.clear();
		}
		std::erase_if(collection.atoms, [&](const auto& atom) { return effect.AtomAge(*atom) > sparkLife; });
		for (auto& atom : collection.atoms)
		{
			if (!atom->subCollections.empty())
			{
				LayArc(effect, *atom, *atom->subCollections.front());
			}
		}
		return true;
	}

	void LayArc(Effect& effect, const Atom& spark, Collection& arc) const
	{
		const auto found = spark.data.find(this);
		if (radius.empty() || arc.atoms.empty() || found == spark.data.end())
		{
			return;
		}
		const auto& data = found->second.a;
		const float phase = data.z;
		const int wiggles = static_cast<int>(data.w);
		const float r = effect.FloatProvider(radius, 0.0f);
		const auto n = static_cast<float>(arc.atoms.size());
		const float t = effect.AtomAge(spark);
		const float scale =
		    ((std::sin(phase - k_SparkPulseSpeed * t) + 1.0f) * k_SparkPulse + k_SparkPulseBase) * k_SparkScale * r;
		const float alpha = std::clamp((1.0f - t / sparkLife) * k_SparkAlphaScale, 0.0f, 255.0f);
		const auto m = OrientAlong(spark.position, glm::mat3(1.0f));
		const float pi = std::numbers::pi_v<float>;
		float index = n;
		for (auto& joint : arc.atoms)
		{
			const float u = index / n;
			const float across = std::sin(static_cast<float>(wiggles) * u * pi) * std::sin(4.0f * t + phase) * wiggle;
			const float along = std::sin(static_cast<float>(wiggles + 1) * u * pi) * std::sin(2.0f * t + phase) * wiggle;
			const auto p = m[0] * (r * across) + m[1] * (u * r) + m[2] * (r * along);
			joint->ruleScale = scale;
			joint->velocity = (p - joint->position) / std::max(effect.GetDt(), k_MinimumStep);
			joint->position = p;
			joint->rgba[3] = static_cast<uint8_t>(alpha);
			index -= 1.0f;
		}
	}

	std::string creator;
	std::vector<int> nextGroups;
	std::string radius;
	int maxAtoms;
	float sparkLife;
	float wiggle;
	ParticleSound sound;
};

/// Once the effect has a player, the atom spins about its own vertical by the caster's spin, slowing to a stop over a
/// time
class InitialSpin final: public Modifier
{
public:
	explicit InitialSpin(const ParticleObject& object)
	    : scale(object.Float("ScaleAngularVelocity", 1.0f))
	    , maxSpeed(object.Float("MaxAngularVelocity", 1.0f))
	    , timeToFade(object.Float("TimeToFade", 2.0f))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started)
		{
			if (effect.GetPlayer() < 0)
			{
				return true;
			}
			data.a.x = std::clamp(effect.GetProcessInfo().spin * scale, -maxSpeed, maxSpeed);
			data.started = true;
		}
		const float fade = std::clamp(effect.AtomAge(atom) / timeToFade, 0.0f, 1.0f);
		const float angle = data.a.x * (1.0f - fade) * effect.GetDt();
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		auto& m = atom.rotation;
		const auto r0 = m[0];
		const auto r2 = m[2];
		m[0] = c * r0 + s * r2;
		m[2] = c * r2 - s * r0;
		return true;
	}

	float scale;
	float maxSpeed;
	float timeToFade;
};

/// The dome's surface patches: each sits on its parent, turned so its vertical points out from the centre and then by
/// as much as the parent moved round the centre; its scale from the provider, fading in
class VapourEndEffect final: public Modifier
{
public:
	explicit VapourEndEffect(const ParticleObject& object)
	    : scaleFactor(object.String("ScaleFactor"))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (scaleFactor.empty())
		{
			return true;
		}
		const auto* parent = atom.collection->parent;
		const auto p = parent != nullptr ? parent->position : effect.GetOrigin();
		auto& data = atom.data[this];
		auto m = atom.rotation;
		if (!data.started)
		{
			data.started = true;
			m = OrientAlong(p, m);
		}
		else
		{
			const auto last = p - (parent != nullptr ? parent->velocity : glm::vec3(0.0f)) * effect.GetDt();
			const auto moved = glm::abs(last - p);
			if (moved.x > k_StillMove || moved.y > k_StillMove || moved.z > k_StillMove)
			{
				const float lengths = glm::length(last) * glm::length(p);
				const auto axis = glm::cross(last, p);
				if (lengths > 0.0f && glm::dot(axis, axis) > 0.0f)
				{
					const float angle = std::acos(std::clamp(glm::dot(last, p) / lengths, -1.0f, 1.0f));
					const auto n = glm::normalize(axis);
					for (int i = 0; i < 3; ++i)
					{
						m[i] = glm::rotate(m[i], angle, n);
					}
				}
			}
		}
		atom.position = p;
		atom.rotation = m;
		atom.ruleScale = effect.FloatProvider(scaleFactor, 1.0f);
		atom.rgba[3] = static_cast<uint8_t>(std::clamp(effect.AtomAge(atom) * k_VapourFadeIn, 0.0f, 255.0f));
		return true;
	}

	std::string scaleFactor;
};

/// The collection's alpha is the provider's, between 0 and 255; without a provider the rule lets go
class SetCollectionAlpha final: public Modifier
{
public:
	explicit SetCollectionAlpha(const ParticleObject& object)
	    : alpha(object.String("Alpha"))
	{
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		if (alpha.empty())
		{
			return false;
		}
		collection.alpha = std::floor(std::clamp(effect.FloatProvider(alpha, 255.0f), 0.0f, 255.0f));
		return true;
	}

	std::string alpha;
};
} // namespace

void openblack::particles::StrikeShield(ShieldSphere& shield, const glm::vec3& point)
{
	shield.impacts.push_back(point);
}

bool openblack::particles::DeflectOffShields(Effect& effect, Atom& atom, const glm::vec3& previousGlobal)
{
	auto& world = effect.Services().world;
	const auto global = effect.GlobalPosition(atom);
	const float margin = k_ShieldParticleMargin * atom.baseScale * atom.ruleScale;
	const auto shield = world.FindShield(global, margin);
	if (!shield || shield->owner == &effect || maths::InsideSphere(previousGlobal, shield->centre, shield->radius, margin))
	{
		return false;
	}
	const auto hit = maths::SphereEntry(previousGlobal, global, shield->centre, shield->radius, margin);
	StrikeShield(*shield, hit);
	const bool through = effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
	                                            .position = hit,
	                                            .velocity = global - previousGlobal,
	                                            .strength = 1.0f,
	                                            .checkShields = false,
	                                            .target = shield->spell});
	if (through)
	{
		return false;
	}
	atom.position = atom.collection != nullptr ? effect.GlobalToLocal(*atom.collection, hit) : hit;
	atom.velocity = maths::DeflectOffSphere(hit, shield->centre, atom.velocity);
	return true;
}

void openblack::particles::RegisterShieldRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_AddDefensiveSphere", ParticleClassRegistry::Make<AddDefensiveSphere>);
	registry.AddModifier("CheckShieldDeflections", ParticleClassRegistry::Make<CheckShieldDeflections>);
	registry.AddModifier("UpdateRuleShieldSpark", ParticleClassRegistry::Make<ShieldSpark>);
	registry.AddModifier("UR_InitialSpin", ParticleClassRegistry::Make<InitialSpin>);
	registry.AddModifier("UR_VapourEndEffect", ParticleClassRegistry::Make<VapourEndEffect>);
	registry.AddModifier("SetCollectionAlpha", ParticleClassRegistry::Make<SetCollectionAlpha>);
}
