/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that remove atoms, change how they look, and move and turn them

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// A rule's own stopwatch never divides by less than this
constexpr float k_MinimumDuration = 1e-4f;

// Removing atoms

/// An atom goes once older than its die age
class RemoveRuleOldAgeOnly final: public Modifier
{
public:
	explicit RemoveRuleOldAgeOnly(const ParticleObject& object)
	    : dieAge(object.Float("DieAge", 1.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		return effect.AtomAge(atom) <= dieAge;
	}
	float dieAge;
};

/// Every atom goes a delay after the effect closes down
class RemoveRuleAfterCloseDown final: public Modifier
{
public:
	explicit RemoveRuleAfterCloseDown(const ParticleObject& object)
	    : delay(object.Float("Delay", 0.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& /*atom*/, Collection::Slot& /*slot*/) const override
	{
		return !(effect.Closing() && effect.GetAge() - effect.GetCloseAge() > delay);
	}
	float delay;
};

/// An atom goes a delay after its removal condition first holds (at once without one). Approximated: the delay is kept
/// from the first step the condition held.
class RemoveRuleAfterConditionTrue final: public Modifier
{
public:
	explicit RemoveRuleAfterConditionTrue(const ParticleObject& object)
	    : delay(object.Float("Delay", 0.0f))
	    , conditionForRemove(object.String("ConditionForRemove"))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started && (conditionForRemove.empty() || effect.ConditionForAtom(conditionForRemove, atom)))
		{
			data.started = true;
			data.a.x = effect.GetAge();
		}
		return !data.started || effect.GetAge() - data.a.x <= delay;
	}
	float delay;
	std::string conditionForRemove;
};

/// While the collection has more than a minimum of atoms, each goes with a chance of its frequency a second
class RemoveRuleProb final: public Modifier
{
public:
	explicit RemoveRuleProb(const ParticleObject& object)
	    : frequency(object.Float("RemoveFreq", 0.0f))
	    , minAtoms(object.Int("MinAtoms", 0))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		return !(static_cast<int>(atom.collection->atoms.size()) > minAtoms &&
		         effect.Random(1.0f) < effect.GetDt() * frequency);
	}
	float frequency;
	int minAtoms;
};

/// An atom under the land goes, first telling the miracle it landed where it is, with its movement over the step
class LandscapeCollide final: public Modifier
{
public:
	explicit LandscapeCollide(const ParticleObject& object)
	    : sendEvent(object.Bool("SendEvent", false))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const auto p = effect.GlobalPosition(atom);
		if (p.y >= effect.Services().world.LandHeight({p.x, p.z}))
		{
			return true;
		}
		if (sendEvent)
		{
			effect.SendSpellEvent({
			    .type = SpellEventInfo::Type::Landed,
			    .position = p,
			    .velocity = atom.velocity * effect.GetDt(),
			});
		}
		return false;
	}
	bool sendEvent;
};

// How atoms look

/// An atom's alpha over a window of its life
class FadeAlpha final: public Modifier
{
public:
	explicit FadeAlpha(const ParticleObject& object)
	    : start(object.Float("StartTime", 0.0f))
	    , stop(object.Float("StopTime", 1.0f))
	    , from(static_cast<float>(object.Int("StartAlpha", 255)))
	    , to(static_cast<float>(object.Int("StopAlpha", 0)))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (const auto alpha = maths::TimedValue(effect.AtomAge(atom), effect.GetDt(), start, stop, from, to))
		{
			atom.rgba[3] = maths::TruncateToByte(*alpha);
		}
		return true;
	}
	float start, stop, from, to;
};

/// The whole collection's alpha over a window of its life, or of the time since the effect closed down; with
/// SetAlphaAfterStopTime it stays at the end's alpha after the window
class FadeCollectionAlpha final: public Modifier
{
public:
	explicit FadeCollectionAlpha(const ParticleObject& object)
	    : start(object.Float("StartTime", 0.0f))
	    , stop(object.Float("StopTime", 1.0f))
	    , from(static_cast<float>(object.Int("StartAlpha", 255)))
	    , to(static_cast<float>(object.Int("StopAlpha", 0)))
	    , afterCloseDown(object.Bool("TimesAreAfterCloseDown", false))
	    , holdAfterStop(object.Bool("SetAlphaAfterStopTime", false))
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		float age = effect.CollectionAge(collection);
		if (afterCloseDown)
		{
			if (!effect.Closing())
			{
				return true;
			}
			age = std::max(0.0f, effect.GetAge() - effect.GetCloseAge());
		}
		auto alpha = maths::TimedValue(age, effect.GetDt(), start, stop, from, to);
		if (!alpha.has_value() && holdAfterStop && age > stop)
		{
			alpha = to;
		}
		if (alpha.has_value())
		{
			collection.alpha = std::clamp(std::trunc(*alpha), 0.0f, 255.0f);
		}
		return true;
	}
	float start, stop, from, to;
	bool afterCloseDown, holdAfterStop;
};

/// Once its condition holds (at once without one), an atom fades out and or shrinks away from how it was then
class FadeOutOnceConditionTrue final: public Modifier
{
public:
	explicit FadeOutOnceConditionTrue(const ParticleObject& object)
	    : time(object.Float("TimeToFadeOut", 1.0f))
	    , fadeAlpha(object.Bool("FadeAlpha", true))
	    , shrink(object.Bool("ShrinkScale", false))
	    , conditionStartFadeOut(object.String("ConditionStartFadeOut"))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		// a: the age, alpha and scale when it started
		auto& data = atom.data[this];
		if (!data.started && (conditionStartFadeOut.empty() || effect.ConditionForAtom(conditionStartFadeOut, atom)))
		{
			data.started = true;
			data.a = glm::vec4(effect.AtomAge(atom), atom.rgba[3], atom.ruleScale, 0.0f);
		}
		if (!data.started || !(fadeAlpha || shrink))
		{
			return true;
		}
		const float f = std::clamp((effect.AtomAge(atom) - data.a.x) / std::max(time, k_MinimumDuration), 0.0f, 1.0f);
		if (fadeAlpha)
		{
			atom.rgba[3] = static_cast<uint8_t>(data.a.y * (1.0f - f));
		}
		if (shrink)
		{
			atom.ruleScale = data.a.z * (1.0f - f);
		}
		return true;
	}
	float time;
	bool fadeAlpha, shrink;
	std::string conditionStartFadeOut;
};

/// An atom's scale over a window of its life
class ChangeScale final: public Modifier
{
public:
	explicit ChangeScale(const ParticleObject& object)
	    : start(object.Float("StartTime", 0.0f))
	    , stop(object.Float("StopTime", 1.0f))
	    , from(object.Float("StartScale", 1.0f))
	    , to(object.Float("StopScale", 1.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (const auto scale = maths::TimedValue(effect.AtomAge(atom), effect.GetDt(), start, stop, from, to))
		{
			atom.ruleScale = *scale;
		}
		return true;
	}
	float start, stop, from, to;
};

/// An atom's scale from a float provider, every step
class SetScale final: public Modifier
{
public:
	explicit SetScale(const ParticleObject& object)
	    : provider(object.String("Scale"))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.ruleScale = effect.FloatProvider(provider, 1.0f);
		return true;
	}
	std::string provider;
};

/// An atom's alpha from a float provider, every step
class SetAtomAlpha final: public Modifier
{
public:
	explicit SetAtomAlpha(const ParticleObject& object)
	    : provider(object.String("Alpha"))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.rgba[3] = static_cast<uint8_t>(std::clamp(effect.FloatProvider(provider, 255.0f), 0.0f, 255.0f));
		return true;
	}
	std::string provider;
};

// Moving and turning atoms

/// Gravity pulls an atom down, less as it nears its top falling speed, after its velocity is damped when asked; then it
/// moves by its velocity
class Gravity final: public Modifier
{
public:
	explicit Gravity(const ParticleObject& object)
	    : gravity(object.Float("Gravity", 10.0f))
	    , maxSpeed(object.Float("MaxSpeed", 100.0f))
	    , damping(object.Bool("UseDamping", false) ? object.Float("Damping", 0.0f) : 0.0f)
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const float dt = effect.GetDt();
		atom.velocity *= 1.0f - dt * damping;
		atom.velocity.y -= std::clamp(atom.velocity.y + maxSpeed, 0.0f, 1.0f) * gravity * atom.gravity * dt;
		atom.position += atom.velocity * dt;
		return true;
	}
	float gravity, maxSpeed, damping;
};

/// An atom moves by its velocity
class PositionFromVelocity final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.position += atom.velocity * effect.GetDt();
		return true;
	}
};

/// A gusting wind across the ground from the particles' noise at the atom's place: it pushes the atom along, or with
/// SimWind draws its velocity across the ground towards the wind, leaving its rise and fall alone; then it moves
class GustyWind final: public Modifier
{
public:
	explicit GustyWind(const ParticleObject& object)
	    : frequency(object.Float("NoiseFreq", 1.0f))
	    , speed(object.Float("WindSpeed", 1.0f))
	    , damping(object.Float("Damping", 0.0f))
	    , simulate(object.Bool("SimWind", false))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const float dt = effect.GetDt();
		const auto noise = effect.Services().noise.Wind(atom.position * frequency);
		const glm::vec3 wind(noise.x * speed, 0.0f, noise.y * speed);
		if (simulate)
		{
			const glm::vec3 change = (wind - atom.velocity) * damping * dt;
			atom.velocity.x += change.x;
			atom.velocity.z += change.z;
		}
		else
		{
			atom.velocity += wind * dt;
		}
		atom.position += atom.velocity * dt;
		return true;
	}
	float frequency, speed, damping;
	bool simulate;
};

/// An atom spins about one of the world's axes at an angular speed
class RotatePrincipalAxis final: public Modifier
{
public:
	explicit RotatePrincipalAxis(const ParticleObject& object)
	    : axis(std::clamp(object.Int("AxisChosen", 1), 0, 2))
	    , speed(object.Float("AngularVel", 0.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.rotation = maths::TurnAboutAxis(atom.rotation, axis, effect.GetDt() * speed);
		return true;
	}
	int axis;
	float speed;
};

/// An atom stays at the effect's origin
class FollowOrigin final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.position = atom.collection->hierarchy ? glm::vec3(0.0f) : effect.GetOrigin();
		return true;
	}
};

/// An atom stays with its parent atom and moves as it does
class FollowParent final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (const auto* parent = atom.collection->parent; parent != nullptr)
		{
			atom.position = effect.SpawnPosition(*atom.collection);
			atom.velocity = parent->velocity;
		}
		return true;
	}
};

/// An atom held at a height above the land, or at an altitude above the sea
class ForceHeight final: public Modifier
{
public:
	ForceHeight(const ParticleObject& object, bool aboveLand)
	    : value(object.Float(aboveLand ? "Height" : "Altitude", 0.0f))
	    , aboveLand(aboveLand)
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (atom.collection->hierarchy)
		{
			return true;
		}
		const float ground = aboveLand ? effect.Services().world.LandHeight({atom.position.x, atom.position.z}) : 0.0f;
		atom.position.y = ground + value;
		return true;
	}
	float value;
	bool aboveLand;
};

/// An atom runs over the surface of a sphere (stretched along each axis) round its spawn point, at its own angular
/// speeds from a random start, its alpha set as it goes; turned to face out of the sphere when asked, so patches tile it
class SphereSurfaceTracer final: public Modifier
{
public:
	explicit SphereSurfaceTracer(const ParticleObject& object)
	    : radius(object.Float("SphereRadius", 1.0f))
	    , radiusScale(object.String("ScaleSphereRadius"))
	    , alphaScale(object.String("ScaleAlpha"))
	    , alpha(object.Int("Alpha", 255))
	    , thetaSpeed(object.Float("ThetaSpeed", 1.0f))
	    , phiSpeed(object.Float("PhiSpeed", 1.0f))
	    , scale(object.Float("ScaleX", 1.0f), object.Float("ScaleY", 1.0f), object.Float("ScaleZ", 1.0f))
	    , orientToSurface(object.Bool("OrientToSurface", false))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started)
		{
			data.started = true;
			data.a.x = effect.Random(k_TwoPi);
			data.a.y = effect.Random(k_TwoPi);
		}
		const float age = effect.AtomAge(atom);
		const float theta = std::fmod(age * thetaSpeed + data.a.x, k_TwoPi);
		const float phi = std::fmod(age * phiSpeed + data.a.y, k_TwoPi);
		const float r = radius * effect.FloatProvider(radiusScale, 1.0f);
		glm::vec3 p = r * glm::vec3(scale.x * std::cos(theta) * std::cos(phi), scale.y * std::sin(phi),
		                            scale.z * std::sin(theta) * std::cos(phi));
		if (!atom.collection->hierarchy)
		{
			p += effect.SpawnPosition(*atom.collection);
		}
		atom.velocity = (p - atom.position) / std::max(effect.GetDt(), k_MinimumDuration);
		atom.position = p;
		const float a = alphaScale.empty()
		                    ? static_cast<float>(alpha)
		                    : std::clamp(static_cast<float>(alpha) * effect.FloatProvider(alphaScale, 1.0f), 0.0f, 255.0f);
		atom.rgba[3] = maths::TruncateToByte(a);
		if (orientToSurface)
		{
			// Tilted up to the latitude, then turned round to the longitude
			atom.rotation = maths::TurnAboutAxis(maths::AngleXYZ(0.0f, 0.0f, std::numbers::pi_v<float> * 0.5f - phi), 1, theta);
		}
		return true;
	}
	float radius;
	std::string radiusScale;
	std::string alphaScale;
	int alpha;
	float thetaSpeed, phiSpeed;
	glm::vec3 scale;
	bool orientToSurface;
};

/// Each atom keeps a yaw of its own: the default angle, give or take a random part of the random angle
class OrientSpriteWithRandomAngle final: public Modifier
{
public:
	explicit OrientSpriteWithRandomAngle(const ParticleObject& object)
	    : angle(object.Float("DefaultAngle", 0.0f))
	    , range(object.Float("RandomAngle", 0.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started)
		{
			data.started = true;
			data.a.x = (1.0f - effect.Random(2.0f)) * range + angle;
		}
		atom.rotation = maths::AngleY(data.a.x);
		return true;
	}
	float angle, range;
};
} // namespace

void openblack::particles::RegisterUpdateRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("RemoveRuleOldAgeOnly", ParticleClassRegistry::Make<RemoveRuleOldAgeOnly>);
	registry.AddModifier("RemoveRuleAfterCloseDown", ParticleClassRegistry::Make<RemoveRuleAfterCloseDown>);
	registry.AddModifier("RemoveRuleAfterConditionTrue", ParticleClassRegistry::Make<RemoveRuleAfterConditionTrue>);
	registry.AddModifier("RemoveRuleProb", ParticleClassRegistry::Make<RemoveRuleProb>);
	registry.AddModifier("LandscapeCollide", ParticleClassRegistry::Make<LandscapeCollide>);
	registry.AddModifier("AR_FadeAlpha", ParticleClassRegistry::Make<FadeAlpha>);
	registry.AddModifier("AR_FadeCollectionAlpha", ParticleClassRegistry::Make<FadeCollectionAlpha>);
	registry.AddModifier("AR_FadeOutOnceConditionTrue", ParticleClassRegistry::Make<FadeOutOnceConditionTrue>);
	registry.AddModifier("UR_ChangeScale", ParticleClassRegistry::Make<ChangeScale>);
	registry.AddModifier("SetScale", ParticleClassRegistry::Make<SetScale>);
	registry.AddModifier("SetAtomAlpha", ParticleClassRegistry::Make<SetAtomAlpha>);
	registry.AddModifier("UpdateRuleGravity", ParticleClassRegistry::Make<Gravity>);
	registry.AddModifier("UR_UpdatePosnFromVelocity", ParticleClassRegistry::Make<PositionFromVelocity>);
	registry.AddModifier("UR_GustyWind", ParticleClassRegistry::Make<GustyWind>);
	registry.AddModifier("UpdateRuleRotatePrincipalAxis", ParticleClassRegistry::Make<RotatePrincipalAxis>);
	registry.AddModifier("FollowOrigin", ParticleClassRegistry::Make<FollowOrigin>);
	registry.AddModifier("UR_FollowParent", ParticleClassRegistry::Make<FollowParent>);
	registry.AddModifier("ForceConstantHeight", [](const ParticleObject& object) -> std::unique_ptr<Modifier> {
		return std::make_unique<ForceHeight>(object, true);
	});
	registry.AddModifier("ForceConstantAltitude", [](const ParticleObject& object) -> std::unique_ptr<Modifier> {
		return std::make_unique<ForceHeight>(object, false);
	});
	registry.AddModifier("UR_SphereSurfaceTracer", ParticleClassRegistry::Make<SphereSurfaceTracer>);
	registry.AddModifier("UR_OrientSpriteWithRandomAngle", ParticleClassRegistry::Make<OrientSpriteWithRandomAngle>);
}
