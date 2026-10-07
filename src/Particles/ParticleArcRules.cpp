/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The electric arcs a lightning strike leaves crawling over what it hit. Each object a bolt strikes for the first time
// since it last looked for targets (once that search is a moment old, and only if its model doesn't move) gets an atom
// here, with five points of its model picked at random, unless a hundred such atoms already live. Every tick or two two of them
// are picked as the ends of an arc, and each step the arc's ribbon is laid along a curve from one to the other leaving along
// the model's normals, jittered afresh, so it writhes. Its group's other rules take the arcs away after a few seconds.

#include <algorithm>
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "LightningMaths.h"
#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// An arc picks its ends among this many points of the object's model
constexpr size_t k_ArcPoints = 5;

/// What an arc keeps
struct Arc
{
	std::vector<SurfacePoint> points;
	maths::ArcEnds ends;
	/// Ticks until its ends are picked again
	int32_t ticks {0};
};
/// Each arc by its atom
using Arcs = std::unordered_map<const Atom*, Arc>;

class ObjectArcer final: public Modifier
{
public:
	explicit ObjectArcer(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , chainCreator(object.String("ChainCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , averageTicks(std::max(1, object.Int("AverageTicksPerUpdate", 2)))
	    , joints(std::max(2, object.Int("JointsPerArc", 8)))
	    , randomFrac(object.Float("RandomFrac", 0.1f))
	    , randomTangents(object.Float("RandomTangents", 0.2f))
	    , scaleTangents(object.Float("ScaleTangents", 3.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (!slot.data.has_value())
		{
			slot.data = Arcs {};
		}
		auto& arcs = *std::any_cast<Arcs>(&slot.data);
		auto& world = effect.Services().world;
		const auto random = [&effect](int32_t n) { return n > 0 ? effect.Rand(n) : 0; };
		// The objects struck since the last step, each an atom with its points
		for (auto object = world.TakeArcs(); object.has_value(); object = world.TakeArcs())
		{
			auto points = world.SurfacePoints(*object, k_ArcPoints, random);
			if (points.empty())
			{
				continue;
			}
			auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
			atom.position = points.front().position;
			auto& data = atom.data[this];
			data.object = *object;
			// Counted among the live arcs for as long as its atom lasts
			data.held = world.HoldArc();
			arcs[&atom] = {.points = std::move(points)};
		}
		for (size_t i = 0; i < collection.atoms.size();)
		{
			auto& atom = *collection.atoms[i];
			const auto found = atom.data.find(this);
			if (found == atom.data.end())
			{
				++i;
				continue;
			}
			// Gone with its object
			const auto entry = arcs.find(&atom);
			if (!world.Target(found->second.object, false).has_value() || entry == arcs.end())
			{
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			auto& arc = entry->second;
			if (arc.points.empty())
			{
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			// Now and then new ends, picked from its points each on its own, so an arc may start and end together
			if (--arc.ticks < 0)
			{
				arc.ticks = effect.Rand(averageTicks);
				const auto& from = arc.points.at(static_cast<size_t>(effect.Rand(static_cast<int32_t>(arc.points.size()))));
				const auto& to = arc.points.at(static_cast<size_t>(effect.Rand(static_cast<int32_t>(arc.points.size()))));
				arc.ends = {.from = from.position, .to = to.position, .fromNormal = from.normal, .toNormal = to.normal};
			}
			Lay(effect, atom, arc);
			++i;
		}
		// Arcs whose atoms have gone are forgotten
		std::erase_if(arcs, [&collection](const auto& entry) {
			return std::ranges::none_of(collection.atoms, [&entry](const auto& atom) { return atom.get() == entry.first; });
		});
		return true;
	}

private:
	/// The ribbon laid along the arc, made the first time
	void Lay(Effect& effect, Atom& atom, const Arc& arc) const
	{
		if (atom.subCollections.empty())
		{
			return;
		}
		auto& chain = *atom.subCollections.front();
		if (chain.atoms.empty())
		{
			const auto* jointCreator = effect.FindCreator(chainCreator);
			for (int k = 0; k < joints; ++k)
			{
				effect.NewAtom(chain, jointCreator, {});
			}
		}
		const float length = glm::distance(arc.ends.from, arc.ends.to);
		const auto fromTangent =
		    maths::ArcTangent(arc.ends.fromNormal, effect.RandomInBall(), randomTangents, length, scaleTangents);
		const auto toTangent =
		    maths::ArcTangent(arc.ends.toNormal, effect.RandomInBall(), randomTangents, length, scaleTangents);
		std::vector<glm::vec3> jitters(chain.atoms.size());
		for (auto& jitter : jitters)
		{
			jitter = effect.RandomInBall();
		}
		const auto laid = maths::ArcJoints(arc.ends, fromTangent, toTangent, jitters, randomFrac);
		for (size_t k = 0; k < chain.atoms.size(); ++k)
		{
			chain.atoms[k]->position = laid[k];
		}
	}

	std::string creator;
	std::string chainCreator;
	std::vector<int> nextGroups;
	int32_t averageTicks;
	int joints;
	float randomFrac;
	float randomTangents;
	float scaleTangents;
};
} // namespace

void openblack::particles::RegisterArcRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_ObjectArcer", ParticleClassRegistry::Make<ObjectArcer>);
}
