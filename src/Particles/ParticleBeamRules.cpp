/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The simple beam: wiggling ribbons of light from the effect's origin to each object or point it is given, as a
// creature's hands pour a miracle onto what is below them. Each target the effect is given while it is not closing down
// becomes an atom of its own carrying a few beams, each a collection of ribbon joints. Every step each beam is laid
// afresh from the origin, wherever it is now, to its target, an object being followed for as long as it is there: a few
// key points along the way are pushed aside by noise drifting along the beam, most at its middle, and kept above the
// land, then the joints are spread evenly along a smooth curve through them, thickest at the middle. The ribbon's
// texture slides along it.

#include <cstddef>

#include <algorithm>
#include <limits>
#include <ranges>
#include <string>
#include <vector>

#include <ParticleFile.h>

#include "BeamMaths.h"
#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
class SimpleBeam final: public Modifier
{
public:
	// The game leaves the joints and the minimum height unset when a file doesn't give them; every file does
	explicit SimpleBeam(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , beamGroup(object.Int("BeamGroup", -1))
	    , joints(std::max(0, object.Int("MaxJointsPerFork", 1)))
	    // A curve needs two key points; the files give six
	    , keyPoints(std::max(2, object.Int("NumSplinePoints", 5)))
	    , beams(std::max(0, object.Int("NumBeams", 3)))
	    , wiggle({
	          .frequency = object.Float("WiggleFreq", 4.0f),
	          .speed = object.Float("WiggleSpeed", 1.0f),
	          .amount = object.Float("RandomFrac", 0.1f),
	          .minHeight = object.Float("MinHeight", std::numeric_limits<float>::lowest()),
	      })
	    , speedV(object.Float("SpeedV", 1.0f))
	    , forkScaleMin(object.Float("ForkScaleMin", 1.0f))
	    , forkScaleMax(object.Float("ForkScaleMax", 1.0f))
	{
	}

	/// Waits for targets until the effect closes down, and makes nothing after
	[[nodiscard]] bool KeepsAlive() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		// Where the beams start this step: the parent atom, or the effect's origin
		const auto origin = collection.parent != nullptr ? collection.parent->position : effect.GetOrigin();
		if (!effect.Closing())
		{
			while (effect.TargetCount() + effect.TargetPositionCount() > 0)
			{
				Begin(effect, collection, origin);
			}
		}
		for (auto& atom : collection.atoms)
		{
			const auto found = atom->data.find(this);
			if (found == atom->data.end())
			{
				continue;
			}
			auto& data = found->second;
			Follow(effect, data);
			Lay(effect, *atom, origin, glm::vec3(data.a));
		}
		return true;
	}

private:
	/// A new atom for the next target, objects before points, carrying its beams and their joints
	void Begin(Effect& effect, Collection& collection, const glm::vec3& origin) const
	{
		const std::vector<int> groups(static_cast<size_t>(beams), beamGroup);
		// The atom itself is never drawn: it only carries the beams
		auto& atom = effect.NewAtom(collection, nullptr, groups);
		atom.position = origin;
		auto& data = atom.data[this];
		// Until an object's place is known the beam has no length
		data.a = glm::vec4(origin, 0.0f);
		if (effect.TargetCount() > 0)
		{
			data.object = effect.TakeTarget().value_or(entt::null);
		}
		else if (const auto position = effect.TakeTargetPosition())
		{
			data.a = glm::vec4(*position, 0.0f);
		}
		const auto* jointCreator = effect.FindCreator(creator);
		const bool chain = jointCreator != nullptr && jointCreator->kind == Creator::Kind::Chain;
		// The newest beam first, as the game keeps its lists
		for (auto& beam : std::ranges::reverse_view(atom.subCollections))
		{
			if (chain)
			{
				beam->textureSpeed = speedV;
			}
			for (int i = 0; i < joints; ++i)
			{
				effect.NewAtom(*beam, jointCreator, {});
			}
		}
	}

	/// An object's beam ends at its middle for as long as it is there, and stays where it last was after
	void Follow(const Effect& effect, AtomRuleData& data) const
	{
		if (data.object == entt::null)
		{
			return;
		}
		// TODO: the game ends a beam on a creature at one of its bones rather than at its middle
		if (const auto info = effect.Services().world.Target(data.object, true))
		{
			data.a = glm::vec4(info->position, 0.0f);
		}
		else
		{
			data.object = entt::null;
		}
	}

	/// Each beam laid from the origin to the target, numbered from the newest so that no two wiggle alike, its joints
	/// from the newest at the origin to the first made at the target
	void Lay(Effect& effect, Atom& atom, const glm::vec3& origin, const glm::vec3& target) const
	{
		const auto& services = effect.Services();
		const auto noise = [&services](float x) { return services.noise.Smooth(x); };
		const auto land = [&services](glm::vec2 xz) { return services.world.LandHeight(xz); };
		const float magnitude = effect.GetMagnitude();
		const auto count = atom.subCollections.size();
		for (size_t s = 0; s < count; ++s)
		{
			auto& beam = *atom.subCollections[s];
			const auto number = static_cast<int>(count - 1 - s);
			const auto keys =
			    maths::BeamKeyPoints(origin, target, keyPoints, wiggle, effect.CollectionAge(beam), number, noise, land);
			const auto laid = maths::BeamJoints(keys, beam.atoms.size(), magnitude * forkScaleMin, magnitude * forkScaleMax);
			const auto n = std::min(laid.size(), beam.atoms.size());
			for (size_t j = 0; j < n; ++j)
			{
				auto& joint = *beam.atoms[beam.atoms.size() - 1 - j];
				joint.position = laid[j].position;
				joint.ruleScale = laid[j].scale;
			}
		}
	}

	std::string creator;
	int beamGroup;
	int joints;
	int keyPoints;
	int beams;
	maths::BeamWiggle wiggle;
	/// How fast a chain's texture slides along it, in sheet heights a second
	float speedV;
	float forkScaleMin;
	float forkScaleMax;
};
} // namespace

void openblack::particles::RegisterBeamRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_SimpleBeam", ParticleClassRegistry::Make<SimpleBeam>);
}
