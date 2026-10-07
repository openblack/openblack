/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that make atoms: once, a ball of them at once, emitters of several shapes over time, and a trail behind a
// moving parent

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// A conical emitter's atoms leave at between these fractions of its speed
constexpr float k_ConicalSpeedMinimum = 0.66f;
constexpr float k_ConicalSpeedRange = 0.33f;
/// The sound radii a single atom's sound is sized by, when the file gives none
constexpr float k_DefaultSmallSoundRadius = 200.0f;
constexpr float k_DefaultMediumSoundRadius = 500.0f;

/// What every rule that makes atoms has: the creator of its atoms and the groups made under each
class CreateRule: public Modifier
{
public:
	explicit CreateRule(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	{
	}
	[[nodiscard]] bool Creates() const override { return true; }

	std::string creator;
	std::vector<int> nextGroups;
};

/// One atom at the collection's spawn point and an offset, then the rule is done. Its sound is sized by how far the
/// effect reaches, when the file says how to measure it.
class CreateRuleAnAtom final: public CreateRule
{
public:
	explicit CreateRuleAnAtom(const ParticleObject& object)
	    : CreateRule(object)
	    , offset(object.Float("OffsetX", 0.0f), object.Float("OffsetY", 0.0f), object.Float("OffsetZ", 0.0f))
	    , scale(object.String("InitScaleFP"))
	    , sound({.action = object.Sound("SoundOfCreate")})
	    , soundRadius(object.String("SoundRadiusFP"))
	    , soundSmall(object.Float("SoundRadiusSmall", k_DefaultSmallSoundRadius))
	    , soundMedium(object.Float("SoundRadiusMedium", k_DefaultMediumSoundRadius))
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* atomCreator = effect.FindCreator(creator);
		if (atomCreator == nullptr)
		{
			return false;
		}
		auto& atom = effect.NewAtom(collection, atomCreator, nextGroups);
		atom.baseScale *= effect.FloatProvider(scale, 1.0f);
		atom.position += offset;
		auto played = sound;
		if (!soundRadius.empty())
		{
			played.size = maths::SoundSizeFromRadius(effect.FloatProvider(soundRadius, 0.0f), soundSmall, soundMedium);
		}
		StartAtomSound(effect, atom, played);
		return false;
	}

	glm::vec3 offset;
	std::string scale;
	ParticleSound sound;
	std::string soundRadius;
	float soundSmall;
	float soundMedium;
};

/// A number of atoms at once, each somewhere in a ball round the spawn point, then the rule is done. They may start on
/// frames counting down by their order, and the first one plays the sound.
class CreateRuleSphere final: public CreateRule
{
public:
	explicit CreateRuleSphere(const ParticleObject& object)
	    : CreateRule(object)
	    , count(object.Int("NumAtoms", 1))
	    , radius(object.Float("Radius", 1.0f))
	    , radiusScale(object.String("RadiusScaleFP"))
	    , scale(object.String("InitScaleFP"))
	    , offset(object.Float("OffsetX", 0.0f), object.Float("OffsetY", 0.0f), object.Float("OffsetZ", 0.0f))
	    , frameFromIndex(object.Bool("SetInitFrameFromIndex", false))
	    , sound({.action = object.Sound("SoundOfCreate")})
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* atomCreator = effect.FindCreator(creator);
		if (atomCreator == nullptr)
		{
			return false;
		}
		const float r = radius * effect.FloatProvider(radiusScale, 1.0f);
		for (int i = 0; i < count; ++i)
		{
			auto& atom = effect.NewAtom(collection, atomCreator, nextGroups);
			atom.baseScale *= effect.FloatProvider(scale, 1.0f);
			if (frameFromIndex && atomCreator->numFrames > 0)
			{
				atom.frame = static_cast<float>((count - i - 1) % atomCreator->numFrames);
			}
			glm::vec3 place(0.0f);
			if (r != 0.0f)
			{
				place = effect.RandomInBall() * r;
			}
			atom.position += place + offset;
			if (i == 0)
			{
				StartAtomSound(effect, atom, sound);
			}
		}
		return false;
	}

	int count;
	float radius;
	std::string radiusScale;
	std::string scale;
	glm::vec3 offset;
	bool frameFromIndex;
	ParticleSound sound;
};

/// What the emitters share: their schedule (emitters make atoms over time, at a frequency, up to limits)
class Emitter: public CreateRule
{
public:
	explicit Emitter(const ParticleObject& object)
	    : CreateRule(object)
	    , limits({
	          .frequency = object.Float("EmissionFreq", 1.0f),
	          .maxAlive = object.Int("MaxAtoms", -1),
	          .maxTotal = object.Int("MaxTotalAtomsToEmit", -1),
	          .randomise = object.Bool("Randomise", true),
	      })
	    , initiallyVisible(object.Bool("InitiallyVisible", true))
	    , multiple(object.Bool("AllowMultipleEmits", false))
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* atomCreator = effect.FindCreator(creator);
		if (atomCreator == nullptr)
		{
			return true;
		}
		if (!AsksAgain())
		{
			if (Due(effect, collection, slot))
			{
				Emit(effect, effect.NewAtom(collection, atomCreator, nextGroups), collection);
			}
			return true;
		}
		// It asks again after each atom whether another is due, which moves its schedule on, but makes another only
		// when it may emit several in a step: otherwise an atom due a second time in the step is lost
		bool another = true;
		while (Due(effect, collection, slot) && another)
		{
			another = multiple;
			Emit(effect, effect.NewAtom(collection, atomCreator, nextGroups), collection);
		}
		return true;
	}

protected:
	/// The emitter's shape: where its new atom goes and how fast
	virtual void Emit(Effect& effect, Atom& atom, const Collection& collection) const = 0;
	/// Whether it asks after each atom whether another is due, emitting as many in a step as AllowMultipleEmits lets it;
	/// otherwise it makes at most one atom a step
	[[nodiscard]] virtual bool AsksAgain() const { return false; }

	maths::EmitterLimits limits;
	bool initiallyVisible;
	bool multiple;

private:
	bool Due(Effect& effect, const Collection& collection, Collection::Slot& slot) const
	{
		return maths::ShouldEmit(slot.emitter, limits, effect.CollectionAge(collection), effect.GetDt(),
		                         static_cast<int>(collection.atoms.size()), [&effect] { return effect.Random(0.5f); });
	}
};

/// Atoms that leave in any direction at up to its speed, turned as their parent is when asked
class EmitterRuleSimple final: public Emitter
{
public:
	explicit EmitterRuleSimple(const ParticleObject& object)
	    : Emitter(object)
	    , speed(object.Float("Speed", 0.0f))
	    , orientWithParent(object.Bool("OrientWithParent", false))
	{
	}

protected:
	void Emit(Effect& effect, Atom& atom, const Collection& collection) const override
	{
		if (speed != 0.0f)
		{
			glm::vec3 direction = effect.RandomInBall();
			const float s = effect.Random(speed);
			if (direction != glm::vec3(0.0f))
			{
				direction = glm::normalize(direction);
			}
			atom.velocity = direction * s;
		}
		if (orientWithParent && collection.parent != nullptr)
		{
			atom.rotation = collection.parent->rotation;
		}
		atom.visible = initiallyVisible;
	}

	float speed;
	bool orientWithParent;
};

/// Atoms placed round the spawn point on a flat disk at a height: a direction across the ground, and a distance up to
/// the radius along it
class DiskEmitter final: public Emitter
{
public:
	explicit DiskEmitter(const ParticleObject& object)
	    : Emitter(object)
	    , radius(object.Float("Radius", 0.0f))
	    , height(object.Float("Height", 0.0f))
	{
	}

protected:
	void Emit(Effect& effect, Atom& atom, const Collection& /*collection*/) const override
	{
		// A point in the unit disk, drawn again until it is in it
		glm::vec2 direction;
		do
		{
			direction.x = effect.Random(2.0f) - 1.0f;
			direction.y = effect.Random(2.0f) - 1.0f;
		} while (glm::dot(direction, direction) > 1.0f);
		const float distance = effect.Random(radius);
		if (direction != glm::vec2(0.0f))
		{
			direction = glm::normalize(direction);
		}
		atom.position += glm::vec3(direction.x * distance, height, direction.y * distance);
	}

	float radius;
	float height;
};

/// Atoms placed on a ring between two radii, at a height: one a step, or as many as are due when it may emit several
class SpreadingDiskEmitter final: public Emitter
{
public:
	explicit SpreadingDiskEmitter(const ParticleObject& object)
	    : Emitter(object)
	    , startRadius(object.Float("StartRadius", 0.0f))
	    , stopRadius(object.Float("StopRadius", 0.0f))
	    , height(object.Float("Height", 0.0f))
	{
	}

protected:
	void Emit(Effect& effect, Atom& atom, const Collection& /*collection*/) const override
	{
		const float r = effect.Random(startRadius, stopRadius);
		const float angle = effect.Random(k_TwoPi);
		atom.position += glm::vec3(std::cos(angle) * r, height, std::sin(angle) * r);
		atom.visible = initiallyVisible;
	}
	[[nodiscard]] bool AsksAgain() const override { return true; }

	float startRadius;
	float stopRadius;
	float height;
};

/// Atoms that leave upwards within a cone, its half angle the spread, at two thirds to all of its speed, from anywhere
/// on a disk of its radius; each plays the emitter's sound
class EmitterRuleConical final: public Emitter
{
public:
	explicit EmitterRuleConical(const ParticleObject& object)
	    : Emitter(object)
	    , speed(object.Float("Speed", 1.0f))
	    , spread(object.Float("Spread", 0.0f))
	    , radius(object.Float("Radius", 0.0f))
	    , sound({.action = object.Sound("SoundEmission")})
	{
	}

protected:
	void Emit(Effect& effect, Atom& atom, const Collection& /*collection*/) const override
	{
		const float phi = effect.Random(spread);
		const float theta = effect.Random(k_TwoPi);
		const float extra = effect.Random(speed * k_ConicalSpeedRange);
		const float s = speed * k_ConicalSpeedMinimum + extra;
		if (radius != 0.0f)
		{
			const float angle = effect.Random(k_TwoPi);
			const float distance = effect.Random(radius);
			atom.position += glm::vec3(std::cos(angle) * distance, 0.0f, std::sin(angle) * distance);
		}
		const float across = std::sin(phi) * s;
		atom.velocity = {std::cos(theta) * across, std::cos(phi) * s, std::sin(theta) * across};
		atom.visible = initiallyVisible;
		StartAtomSound(effect, atom, sound);
	}
	[[nodiscard]] bool AsksAgain() const override { return true; }

	float speed;
	float spread;
	float radius;
	ParticleSound sound;
};

/// Atoms let out along the path its parent atom (or the effect's origin) moved over the step, as many as its rate and,
/// when asked, the distance moved call for; each starts aged by how far back along the path it was let out. MaxAtoms
/// sets the rate over the atoms' life rather than capping them. What a file leaves out takes the game's defaults: speed
/// 1 up to 100, random speed 1, no rate at all, atoms deleted once older than their die age, drawn on their first step.
class WillowWisp final: public CreateRule
{
public:
	explicit WillowWisp(const ParticleObject& object)
	    : CreateRule(object)
	    , maxAtoms(object.Int("MaxAtoms", -1))
	    , dieAge(object.Float("DieAge", 1.0f))
	    , speed(object.Float("Speed", 1.0f))
	    , maxSpeed(object.Float("MaxSpeed", 100.0f))
	    , randomSpeed(object.Float("RandomSpeed", 1.0f))
	    , randomRadiusMin(object.Float("RandomRadiusMin", 0.0f))
	    , randomRadiusMax(object.Float("RandomRadiusMax", 0.0f))
	    , deleteAtDieAge(object.Bool("DeleteAtomsAtDieAge", true))
	    , moving(object.Bool("EmitDueToMoving", false))
	    , movingDistance(object.Float("EmitDueToMovingDist", 10.0f))
	    , movingMaxRate(object.Float("EmitDueToMovingMaxRate", 0.0f))
	    , randomiseOrientation(object.Bool("RandomiseInitOrientation", false))
	    , useParentScale(object.Bool("UseParentScale", false))
	    , addCastVelocity(object.Bool("AddCastVelToInitPos", false))
	    , drawOnFirstUpdate(object.Bool("DrawOnFirstUpdate", true))
	    , drawOffsets(object.Bool("DoDrawOffsets", false))
	    , emitCondition(object.String("EmitConditionOfParent"))
	    , adjustScale(object.String("AdjustInitialScale"))
	    , adjustRandomVelocity(object.String("AdjustInitialRandomVel"))
	    , sound({.action = object.Sound("SoundEmission")})
	{
	}
	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (deleteAtDieAge)
		{
			std::erase_if(collection.atoms, [&](const auto& atom) { return effect.AtomAge(*atom) > dieAge; });
		}
		const glm::vec3 parent = collection.parent != nullptr ? effect.GlobalPosition(*collection.parent) : effect.GetOrigin();
		const glm::vec3 parentVelocity = collection.parent != nullptr ? collection.parent->velocity : glm::vec3(0.0f);
		glm::vec3 base = parentVelocity * speed;
		if (glm::dot(base, base) > maxSpeed * maxSpeed && base != glm::vec3(0.0f))
		{
			base *= maxSpeed / glm::length(base);
		}
		if (slot.first)
		{
			slot.state = glm::vec4(parent, 0.0f);
		}
		const glm::vec3 last(slot.state);
		const float dt = effect.GetDt();
		const float rate = static_cast<float>(maxAtoms) / dieAge;
		// Tested on the parent atom: without one nothing is let out
		const bool emit = emitCondition.empty() ||
		                  (collection.parent != nullptr && effect.ConditionForAtom(emitCondition, *collection.parent));
		const auto* atomCreator = effect.FindCreator(creator);
		if (emit && atomCreator != nullptr)
		{
			float amount = dt * rate;
			if (moving && movingDistance != 0.0f)
			{
				float moved = glm::distance(parent, last) * effect.FloatProvider(adjustScale, 1.0f) / movingDistance;
				if (movingMaxRate != 0.0f)
				{
					moved = std::min(moved / dt, movingMaxRate) * dt;
				}
				amount = std::max(amount, moved);
			}
			// slot.extra: x how much has been let out in all, y how many atoms that made
			const float before = slot.extra.x;
			slot.extra.x += amount;
			const float after = slot.extra.x;
			while (after - 1.0f > slot.extra.y)
			{
				slot.extra.y += 1.0f;
				auto& atom = effect.NewAtom(collection, atomCreator, nextGroups);
				Place(effect, collection, atom, parent, last, base, (slot.extra.y - before) / (after - before),
				      after != before);
			}
		}
		slot.first = false;
		slot.state = glm::vec4(parent, 0.0f);
		return true;
	}

private:
	void Place(Effect& effect, const Collection& collection, Atom& atom, const glm::vec3& parent, const glm::vec3& last,
	           const glm::vec3& base, float fraction, bool alongPath) const
	{
		const float dt = effect.GetDt();
		glm::vec3 position = parent;
		if (alongPath)
		{
			position = last + (parent - last) * fraction;
			atom.birth -= fraction * dt;
		}
		if (randomRadiusMax > 0.0f)
		{
			const float angle = effect.Random(k_TwoPi);
			const float r = randomRadiusMin + effect.Random(randomRadiusMax - randomRadiusMin);
			position += glm::vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
		}
		glm::vec3 random = effect.RandomInBall() * effect.Random(randomSpeed);
		random *= effect.FloatProvider(adjustRandomVelocity, 1.0f);
		atom.velocity = base + random;
		if (addCastVelocity)
		{
			position += atom.velocity * dt;
		}
		if (!collection.hierarchy)
		{
			atom.position = position;
		}
		// This computer's player sees it come out of their hand, sliding to where it is over two seconds
		if (drawOffsets && effect.IsMyInterfaceCasting())
		{
			atom.drawOffset = effect.GetProcessInfo().handPosition - position;
			atom.drawOffsetFrom = effect.GetAge();
		}
		if (randomiseOrientation)
		{
			// Drawn z first, then y, then x
			const float z = effect.Random(k_TwoPi);
			const float y = effect.Random(k_TwoPi);
			const float x = effect.Random(k_TwoPi);
			atom.rotation = maths::AngleXYZ(x, y, z);
		}
		atom.baseScale *= effect.FloatProvider(adjustScale, 1.0f);
		if (useParentScale && collection.parent != nullptr)
		{
			atom.baseScale *= collection.parent->ruleScale * collection.parent->baseScale;
		}
		atom.drawOnFirstUpdate = drawOnFirstUpdate;
		StartAtomSound(effect, atom, sound);
	}

	int maxAtoms;
	float dieAge;
	float speed;
	float maxSpeed;
	float randomSpeed;
	float randomRadiusMin;
	float randomRadiusMax;
	bool deleteAtDieAge;
	bool moving;
	float movingDistance;
	float movingMaxRate;
	bool randomiseOrientation;
	bool useParentScale;
	bool addCastVelocity;
	bool drawOnFirstUpdate;
	bool drawOffsets;
	std::string emitCondition;
	std::string adjustScale;
	std::string adjustRandomVelocity;
	ParticleSound sound;
};
} // namespace

void openblack::particles::RegisterCreateRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("CreateRuleAnAtom", ParticleClassRegistry::Make<CreateRuleAnAtom>);
	registry.AddModifier("CreateRuleSphere", ParticleClassRegistry::Make<CreateRuleSphere>);
	registry.AddModifier("EmitterRuleSimple", ParticleClassRegistry::Make<EmitterRuleSimple>);
	registry.AddModifier("DiskEmitter", ParticleClassRegistry::Make<DiskEmitter>);
	registry.AddModifier("SpreadingDiskEmitter", ParticleClassRegistry::Make<SpreadingDiskEmitter>);
	registry.AddModifier("EmitterRuleConical", ParticleClassRegistry::Make<EmitterRuleConical>);
	registry.AddModifier("UR_WillowWisp", ParticleClassRegistry::Make<WillowWisp>);
}
