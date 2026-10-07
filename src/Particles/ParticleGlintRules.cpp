/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Glints that sparkle on the points of an object's model, as on a frozen-creature phial shown in a globe: for each
// object the effect is given, a parent particle whose group holds the glints. Glints come at the most allowed over the
// age at which they vanish, each on a point of the model picked once, following it as the model moves. A glint grows to
// full size at one age and shrinks to nothing at another, its size pulsing, at a fixed alpha.

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>

#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
class GlintsOnTarget final: public Modifier
{
public:
	explicit GlintsOnTarget(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , glintCreator(object.String("GlintCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , maxAtoms(object.Int("MaxAtoms", 20))
	    , maxAlpha(object.Int("MaxAlpha", 40))
	    , glintGroup(object.Int("GlintGroup", -1))
	    , pulseMagnitude(object.Float("PulseMagnitude", 1.0f))
	    , pulseSpeed(object.Float("PulseSpeed", 1.0f))
	    , ageMaxSize(object.Float("AtomAgeMaxSize", 0.25f))
	    , ageZeroSize(object.Float("AtomAgeZeroSize", 2.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* parentCreator = effect.FindCreator(creator);
		if (parentCreator == nullptr)
		{
			return false;
		}
		// Each object given becomes a parent particle holding a group of glints
		if (effect.TargetCount() > 0)
		{
			if (const auto target = effect.TakeTarget())
			{
				const std::array groups {glintGroup};
				auto& parent = effect.NewAtom(collection, parentCreator, groups);
				parent.data[this].object = *target;
			}
		}
		auto& world = effect.Services().world;
		std::erase_if(collection.atoms, [&](const auto& parent) {
			auto found = parent->data.find(this);
			if (found == parent->data.end())
			{
				return false;
			}
			// The parent goes with its object
			if (!world.ObjectPosition(found->second.object).has_value())
			{
				return true;
			}
			for (auto& glints : parent->subCollections)
			{
				Glints(effect, found->second, *glints);
			}
			return false;
		});
		return true;
	}

	void Glints(Effect& effect, AtomRuleData& parent, Collection& glints) const
	{
		const auto* made = effect.FindCreator(glintCreator);
		if (made == nullptr)
		{
			return;
		}
		const float rate = static_cast<float>(maxAtoms) / ageZeroSize;
		if (!(rate > 0.0f))
		{
			return;
		}
		auto& world = effect.Services().world;
		const auto object = parent.object;
		auto& due = parent.a.x;
		auto& count = parent.a.y;
		due += effect.GetDt() * rate;
		const auto points = world.TargetPointCount(object);
		if (points > 0)
		{
			while (count < due && glints.atoms.size() < static_cast<size_t>(std::max(0, maxAtoms)))
			{
				count += 1.0f;
				auto& glint = effect.NewAtom(glints, made, nextGroups);
				glint.baseScale *= world.TargetScale(object);
				// The point of the model it sparkles on, for its whole life
				glint.data[this].a.x = static_cast<float>(effect.Rand(static_cast<int32_t>(points)));
			}
		}
		std::erase_if(glints.atoms, [&](const auto& glint) {
			const auto& data = glint->data[this];
			if (const auto point = world.TargetPoint(object, static_cast<uint32_t>(data.a.x)))
			{
				glint->position = effect.GlobalToLocal(glints, *point);
			}
			const float age = effect.AtomAge(*glint);
			const float pulse =
			    std::max(std::cos(age * pulseSpeed * 2.0f * std::numbers::pi_v<float>) * pulseMagnitude * 0.5f + 1.0f, 0.0f);
			const float size = std::clamp(
			    age < ageMaxSize ? age / ageMaxSize : 1.0f - (age - ageMaxSize) / (ageZeroSize - ageMaxSize), 0.0f, 1.0f);
			glint->ruleScale = size * pulse;
			glint->rgba[3] = static_cast<uint8_t>(maxAlpha);
			return age > ageZeroSize;
		});
	}

	std::string creator;
	std::string glintCreator;
	std::vector<int> nextGroups;
	int maxAtoms;
	int maxAlpha;
	int glintGroup;
	float pulseMagnitude;
	float pulseSpeed;
	float ageMaxSize;
	float ageZeroSize;
};
} // namespace

void openblack::particles::RegisterGlintRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("ER_GlintsOnTarget", ParticleClassRegistry::Make<GlintsOnTarget>);
}
