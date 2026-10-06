/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that follow key point curves over an atom's life, and the one that lines sprites up with their movement

#include <cmath>

#include <array>

#include "KeyPointSpline.h"
#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// A curve rule's window and curve when the file gives none: five seconds, flat
constexpr float k_DefaultStopTime = 5.0f;
constexpr std::array<float, 4> k_FlatAtOne {0.0f, 1.0f, k_DefaultStopTime, 1.0f};
constexpr std::array<float, 4> k_FlatAtZero {0.0f, 0.0f, k_DefaultStopTime, 0.0f};
/// How fast the smoothing of a sprite's velocity follows: -10 * ln(1 - smoothing) a second
constexpr float k_SmoothingRateScale = -10.0f;

std::span<const float> CurveOf(const ParticleObject& object, std::string_view key, std::span<const float> fallback)
{
	return object.Has(key) ? object.FloatArray(key) : fallback;
}

/// The age a curve is read at: inside the window, the step that runs past its end read at the end. nullopt outside it.
std::optional<float> CurveAge(float age, float dt, float start, float stop)
{
	if (age < start || age > stop)
	{
		return std::nullopt;
	}
	return dt + age > stop ? stop : age;
}

/// An atom's stretch follows the curve over its age
class KPStretchHeight final: public Modifier
{
public:
	explicit KPStretchHeight(const ParticleObject& object)
	    : startTime(object.Float("StartTime", 0.0f))
	    , stopTime(object.Float("StopTime", k_DefaultStopTime))
	    , curve(CurveOf(object, "KeyPoints", k_FlatAtOne))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		if (const auto t = CurveAge(effect.AtomAge(atom), effect.GetDt(), startTime, stopTime))
		{
			atom.stretch = curve.Evaluate(*t, atom.stretch);
		}
		return true;
	}
	float startTime, stopTime;
	KeyPointSpline curve;
};

/// Each atom sits above its parent (or the origin) at the curve's height for its age; with MovePropAtomIndex the newest
/// atom takes the whole height and the oldest none, the others in between
class KPMoveAtoms final: public Modifier
{
public:
	explicit KPMoveAtoms(const ParticleObject& object)
	    : startTime(object.Float("StartTime", 0.0f))
	    , stopTime(object.Float("StopTime", k_DefaultStopTime))
	    , proportional(object.Bool("MovePropAtomIndex", false))
	    , curve(CurveOf(object, "KeyPointsY", k_FlatAtZero))
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const size_t count = collection.atoms.size();
		const float inverse = count > 1 ? 1.0f / static_cast<float>(count - 1) : 0.0f;
		const glm::vec3 parent = collection.parent != nullptr ? collection.parent->position : effect.GetOrigin();
		for (size_t n = 0; n < count; ++n)
		{
			auto& atom = *collection.atoms[n];
			const auto t = CurveAge(effect.AtomAge(atom), effect.GetDt(), startTime, stopTime);
			if (!t.has_value())
			{
				continue;
			}
			glm::vec3 offset(0.0f, curve.Evaluate(*t, 0.0f), 0.0f);
			if (proportional)
			{
				// The atoms are kept oldest first
				offset *= static_cast<float>(count - 1 - n) * inverse;
			}
			atom.position = parent + offset;
		}
		return true;
	}
	float startTime, stopTime;
	bool proportional;
	KeyPointSpline curve;
};

/// A sprite rolls to line up with its smoothed velocity as the camera sees it, lifted by a default upward part (the
/// flames of a fireball in the hand)
class OrientSpriteWithVelocity final: public Modifier
{
public:
	explicit OrientSpriteWithVelocity(const ParticleObject& object)
	    : smoothFactor(object.Float("SmoothFactor", 0.0f))
	    , proportionDefault(object.Float("ProportionDefault", 0.0f))
	{
	}
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		// a: the smoothed velocity and how fast it follows
		auto& data = atom.data[this];
		if (!data.started)
		{
			data.started = true;
			data.a = glm::vec4(atom.velocity, std::log(1.0f - smoothFactor) * k_SmoothingRateScale);
		}
		glm::vec3 smoothed(data.a);
		smoothed += (atom.velocity - smoothed) * (1.0f - std::exp(-effect.GetDt() * data.a.w));
		data.a = glm::vec4(smoothed, data.a.w);
		const glm::vec3 seen(-smoothed.x, -smoothed.y + proportionDefault, -smoothed.z);
		const auto& world = effect.Services().world;
		atom.rotation = maths::AngleY(maths::ScreenVelocityAngle(seen, world.CameraRight(), world.CameraUp()));
		return true;
	}
	float smoothFactor, proportionDefault;
};
} // namespace

void openblack::particles::RegisterCurveRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_KPStretchHeight", ParticleClassRegistry::Make<KPStretchHeight>);
	registry.AddModifier("UR_KPMoveAtoms", ParticleClassRegistry::Make<KPMoveAtoms>);
	registry.AddModifier("UR_OrientSpriteWithVelocity", ParticleClassRegistry::Make<OrientSpriteWithVelocity>);
}
