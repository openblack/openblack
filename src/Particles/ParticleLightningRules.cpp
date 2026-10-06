/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The lightning bolt's rules. A bolt looks for things to strike in a cone in front of the hand (or round its parent
// atom, or among the objects its miracle gives it), and makes up the rest with points on the ground. Each step it
// strikes some of them: a trunk of ribbon joints from the hand forks in two, then in two again, until each branch ends
// on one target, where a light flashes on the land and the miracle is told it struck. The land and shields cut forks
// short. A strike from the sky is one atom with its bolt under it.

#include <cmath>

#include <algorithm>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleMiracleMaths.h"
#include "ParticleShields.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// The margin a fork's split point is tested against shields with
constexpr float k_ShieldMargin = 2.5f;
/// Ground points are put this high above the land
constexpr float k_GroundPointHeight = 2.0f;
/// Ground points are found within this share of the search radius, and a quarter turn either side of the heading
constexpr float k_GroundPointReach = 0.6f;
constexpr float k_GroundPointSpread = std::numbers::pi_v<float> / 4.0f;
/// A fork splits between these shares of the way to its targets' middle
constexpr float k_SplitShareMin = 0.2f;
constexpr float k_SplitShareRange = 0.4f;
/// A fork's joints are half to fully opaque
constexpr int32_t k_JointAlphaRange = 0x7F;
constexpr uint8_t k_JointAlphaMin = 0x80;

/// One thing a bolt may strike
struct BoltTarget
{
	entt::entity object {entt::null};
	/// A ground point, or where the object stood when found
	glm::vec3 ground {0.0f};
	float height {0.0f};
};

/// What a bolt keeps between steps
struct Bolt
{
	glm::vec3 origin {0.0f};
	glm::vec3 searchOrigin {0.0f};
	glm::vec3 centroid {0.0f};
	float heading {0.0f};
	bool built {false};
	/// Seconds since the targets were last looked for
	float life {0.0f};
	float centroidDistance {1.0f};
	float forkScale {1.0f};
	std::vector<BoltTarget> targets;
	/// The single atom whose collections are the forks
	const Atom* root {nullptr};
};

/// Where a target's bolt ends: an object's top, as it stands now, or the ground point
glm::vec3 TipOf(const Effect& effect, const BoltTarget& target)
{
	if (target.object == entt::null)
	{
		return target.ground;
	}
	if (const auto info = effect.Services().world.Target(target.object, false))
	{
		return info->position + glm::vec3(0.0f, info->height, 0.0f);
	}
	return target.ground + glm::vec3(0.0f, target.height, 0.0f);
}

class Lightning final: public Modifier
{
public:
	explicit Lightning(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , lightMapCreator(object.String("PCreatorLightMapAtom"))
	    , forkGroup(object.Int("ForkGroup", -1))
	    , lightMapGroup(object.Int("LightMapGroup", -1))
	    , maxObjects(std::max(1, object.Int("MaxLightningObjects", 50)))
	    , minObjects(std::max(1, object.Int("MinLightningObjects", 3)))
	    , atOnce(std::max(1, object.Int("MaxLightningObjectsAtOnce", 10)))
	    , maxJoints(std::max(2, object.Int("MaxJointsPerFork", 10)))
	    , splitAngle(object.Float("SplitAngle", std::numbers::pi_v<float> / 2.0f))
	    , randomFrac(object.Float("RandomFrac", 0.1f))
	    , forkScale(object.Float("ForkScale", 1.0f))
	    , forkScaleProvider(object.String("FP_ForkScale"))
	    , searchRadius(object.String("SearchRadius"))
	    , defaultSearchRadius(object.Float("DefaultSearchRadius", 20.0f))
	    , castingFromHand(object.Bool("CastingFromHand", true))
	    , takeTargetsFromSpell(object.Bool("TakeTargetsFromManager", false))
	    , renewTargetsOnMove(object.Bool("RenewTargetsOnMove", false))
	    , renewTargetsOnMoveFrac(object.Float("RenewTargetsOnMoveFrac", 0.5f))
	    , renewSearchEvery(object.Float("RenewSearchEvery", 1.0f))
	    , texturesToTile(object.Int("NumTexturesToTile", -1))
	    , sound({.action = object.Sound("SoundLightning")})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (!slot.data.has_value())
		{
			slot.data = Bolt {};
		}
		auto& bolt = *std::any_cast<Bolt>(&slot.data);
		bolt.life += effect.GetDt();
		const auto& info = effect.GetProcessInfo();
		bolt.origin = castingFromHand                ? info.handPosition
		              : collection.parent != nullptr ? effect.GlobalPosition(*collection.parent)
		                                             : effect.GetOrigin();
		// The cone points the way the camera looks
		if (info.cameraForward.x != 0.0f || info.cameraForward.z != 0.0f)
		{
			bolt.heading = std::atan2(info.cameraForward.z, info.cameraForward.x);
		}
		bolt.forkScale = forkScale * (forkScaleProvider.empty() ? 1.0f : effect.FloatProvider(forkScaleProvider, 1.0f));
		if (effect.Closing())
		{
			collection.atoms.clear();
			bolt.root = nullptr;
			bolt.built = false;
			return true;
		}
		if (!bolt.built || !Valid(collection, bolt))
		{
			FindTargets(effect, bolt);
			Build(effect, collection, bolt);
			bolt.built = true;
		}
		else if (Renew(effect, bolt))
		{
			FindTargets(effect, bolt);
		}
		auto& root = *collection.atoms.front();
		// The thunder plays while the miracle is being cast
		if (!sound.Silent())
		{
			auto* playing = FindAtomSound(root, sound.action.sound);
			if (info.enabled && playing == nullptr)
			{
				StartAtomSound(effect, root, sound);
			}
			else if (!info.enabled && playing != nullptr)
			{
				StopAtomSound(root, *playing);
			}
		}
		// Each step only the forks the strike lays out are shown
		for (auto& fork : root.subCollections)
		{
			for (auto& joint : fork->atoms)
			{
				joint->visible = false;
			}
		}
		if (info.enabled)
		{
			Strike(effect, root, bolt);
		}
		return true;
	}

private:
	[[nodiscard]] static bool Valid(const Collection& collection, const Bolt& bolt)
	{
		return bolt.root != nullptr && !collection.atoms.empty() && collection.atoms.front().get() == bolt.root;
	}

	[[nodiscard]] float SearchRadius(const Effect& effect) const
	{
		return searchRadius.empty() ? defaultSearchRadius : effect.FloatProvider(searchRadius, defaultSearchRadius);
	}

	/// Looked for again every so often, or once the origin moved far enough from where they were looked for
	[[nodiscard]] bool Renew(const Effect& effect, const Bolt& bolt) const
	{
		if (renewSearchEvery > 0.0f && bolt.life > renewSearchEvery)
		{
			return true;
		}
		if (!renewTargetsOnMove)
		{
			return false;
		}
		const auto moved = bolt.searchOrigin - bolt.origin;
		const float limit = SearchRadius(effect) * renewTargetsOnMoveFrac;
		return glm::dot(moved, moved) > limit * limit;
	}

	void FindTargets(Effect& effect, Bolt& bolt) const
	{
		bolt.targets.clear();
		bolt.life = 0.0f;
		bolt.searchOrigin = bolt.origin;
		const float radius = SearchRadius(effect);
		auto& world = effect.Services().world;
		if (castingFromHand || !takeTargetsFromSpell)
		{
			const float cosHalfAngle = std::cos(splitAngle);
			for (const auto& candidate : world.StrikeCandidates(bolt.origin, radius))
			{
				if (static_cast<int>(bolt.targets.size()) >= maxObjects)
				{
					break;
				}
				if (castingFromHand && !maths::InStrikeCone(bolt.origin, bolt.heading, cosHalfAngle, candidate.position))
				{
					continue;
				}
				bolt.targets.push_back({candidate.object, candidate.position, candidate.height});
			}
		}
		else
		{
			for (auto target = effect.TakeTarget(); target.has_value(); target = effect.TakeTarget())
			{
				if (static_cast<int>(bolt.targets.size()) < maxObjects)
				{
					if (const auto info = world.Target(*target, false))
					{
						bolt.targets.push_back({*target, info->position, info->height});
					}
				}
			}
		}
		// The rest are points on the ground
		const bool aimed = castingFromHand || takeTargetsFromSpell;
		while (static_cast<int>(bolt.targets.size()) < minObjects)
		{
			const float angle =
			    aimed ? bolt.heading + effect.Random(k_GroundPointSpread) : effect.Random(2.0f * std::numbers::pi_v<float>);
			const float distance = effect.Random(k_GroundPointReach * radius);
			glm::vec3 point(bolt.origin.x + distance * std::cos(angle), 0.0f, bolt.origin.z + distance * std::sin(angle));
			point.y = world.LandHeight({point.x, point.z}) + k_GroundPointHeight;
			bolt.targets.push_back({entt::null, point, 0.0f});
		}
		glm::vec3 sum(0.0f);
		for (const auto& target : bolt.targets)
		{
			sum += TipOf(effect, target);
		}
		bolt.centroid = sum / static_cast<float>(bolt.targets.size());
		bolt.centroidDistance = glm::length(bolt.centroid - bolt.origin);
	}

	/// One root atom, carrying two forks for each target, each fork a ribbon of joints drawn as the step leaves them
	void Build(Effect& effect, Collection& collection, Bolt& bolt) const
	{
		collection.atoms.clear();
		const auto* jointCreator = effect.FindCreator(creator);
		auto& root = effect.NewAtom(collection, jointCreator, {});
		root.visible = false;
		const std::vector<int> groups(2 * bolt.targets.size(), forkGroup);
		effect.AddSubCollections(root, groups);
		for (auto& fork : root.subCollections)
		{
			fork->interpolated = false;
			for (int i = 0; i < maxJoints; ++i)
			{
				effect.NewAtom(*fork, jointCreator, {});
			}
		}
		bolt.root = &root;
	}

	/// Some of the targets are picked, each with an even chance, and the forks are laid out to them from the origin
	void Strike(Effect& effect, Atom& root, Bolt& bolt) const
	{
		const auto total = static_cast<int>(bolt.targets.size());
		const int limit = maths::StrikesAtOnce(atOnce, total);
		std::vector<size_t> striking;
		if (total > 0)
		{
			const float chance = static_cast<float>(limit) / static_cast<float>(total);
			for (int i = 0; i < total && static_cast<int>(striking.size()) < limit; ++i)
			{
				if (effect.Random(1.0f) < chance)
				{
					striking.push_back(static_cast<size_t>(i));
				}
			}
		}
		if (root.subCollections.empty() || striking.empty())
		{
			return;
		}
		size_t nextFork = 1;
		Fork(effect, root, bolt, 0, *root.subCollections.front(), bolt.origin, striking, 1.0f, nextFork);
	}

	/// One fork: from its origin to where its targets split, then a fork for each side from there, or the strike when it
	/// holds a single target
	void Fork(Effect& effect, Atom& root, Bolt& bolt, int depth, Collection& fork, const glm::vec3& origin,
	          const std::vector<size_t>& targets, float scale, size_t& nextFork) const
	{
		const auto joints = static_cast<int>(fork.atoms.size());
		if (targets.empty() || joints < 2)
		{
			return;
		}
		for (auto& joint : fork.atoms)
		{
			joint->visible = true;
		}
		std::vector<glm::vec3> tips;
		tips.reserve(targets.size());
		glm::vec3 centroid(0.0f);
		for (const auto index : targets)
		{
			tips.push_back(TipOf(effect, bolt.targets[index]));
			centroid += tips.back();
		}
		centroid /= static_cast<float>(targets.size());
		auto split = centroid;
		if (targets.size() >= 2)
		{
			split = origin + (centroid - origin) * (k_SplitShareMin + effect.Random(k_SplitShareRange));
		}
		auto& world = effect.Services().world;
		// The land in the way ends the fork where the last step left it
		if (world.LandBlocks(origin, split))
		{
			return;
		}
		// A shield in the way stops the fork on its surface unless the miracle gets through
		bool stopped = false;
		if (const auto shield = world.FindShield(split, k_ShieldMargin); shield && shield->owner != &effect)
		{
			const auto hit = maths::SphereEntry(origin, split, shield->centre, shield->radius, k_ShieldMargin);
			const bool through = effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
			                                            .position = hit,
			                                            .velocity = glm::vec3(0.0f),
			                                            .strength = 1.0f,
			                                            .checkShields = false,
			                                            .target = shield->spell});
			if (!through)
			{
				split = hit;
				stopped = true;
			}
			StrikeShield(*shield, hit);
		}
		LayJoints(effect, bolt, fork, depth, origin, split, scale);
		if (stopped)
		{
			return;
		}
		const auto end = fork.atoms.back()->position;
		if (targets.size() >= 2)
		{
			const auto sides = maths::SplitTargets(tips, origin, centroid, split);
			for (const auto* side : {&sides.ahead, &sides.behind})
			{
				if (nextFork >= root.subCollections.size())
				{
					return;
				}
				std::vector<size_t> list;
				list.reserve(side->size());
				for (const auto i : *side)
				{
					list.push_back(targets[i]);
				}
				auto& child = *root.subCollections[nextFork++];
				Fork(effect, root, bolt, depth + 1, child, end, list, scale, nextFork);
			}
			return;
		}
		Hit(effect, bolt.targets[targets.front()], centroid);
	}

	/// The joints laid along the fork, the inner ones jittered across, thinning with depth and half to fully opaque
	void LayJoints(Effect& effect, const Bolt& bolt, Collection& fork, int depth, const glm::vec3& origin,
	               const glm::vec3& split, float scale) const
	{
		const auto joints = static_cast<int>(fork.atoms.size());
		const auto way = split - origin;
		const float length = glm::length(way);
		if (texturesToTile != -1 && bolt.centroidDistance > 0.0f)
		{
			fork.textureRepeats =
			    std::max(1, static_cast<int>(static_cast<float>(texturesToTile) * length / bolt.centroidDistance));
		}
		const float step = 1.0f / static_cast<float>(joints - 1);
		for (int i = 0; i < joints; ++i)
		{
			auto& joint = *fork.atoms[static_cast<size_t>(i)];
			const float t = static_cast<float>(i) * step;
			auto position = origin + way * t;
			if (i != 0 && i != joints - 1)
			{
				position.x += (effect.Random(2.0f * randomFrac) - randomFrac) * length;
				position.z += (effect.Random(2.0f * randomFrac) - randomFrac) * length;
			}
			joint.position = position;
			joint.ruleScale = maths::ForkJointScale(scale * bolt.forkScale, depth, t);
			joint.rgba[3] = static_cast<uint8_t>(effect.Rand(k_JointAlphaRange) + k_JointAlphaMin);
		}
	}

	/// The light on the land under the struck targets, and the miracle told where the bolt struck
	void Hit(Effect& effect, const BoltTarget& target, const glm::vec3& centroid) const
	{
		if (lightMapGroup >= 0)
		{
			if (auto* light = effect.NewAtomInGroup(lightMapGroup, effect.FindCreator(lightMapCreator)))
			{
				light->position =
				    glm::vec3(centroid.x, effect.Services().world.LandHeight({centroid.x, centroid.z}), centroid.z);
			}
		}
		effect.SendSpellEvent({.type = SpellEventInfo::Type::Landed,
		                       .position = TipOf(effect, target),
		                       .velocity = glm::vec3(0.0f),
		                       .strength = 1.0f,
		                       .checkShields = false,
		                       .target = target.object});
	}

	std::string creator;
	std::string lightMapCreator;
	int forkGroup;
	int lightMapGroup;
	int maxObjects;
	int minObjects;
	int atOnce;
	int maxJoints;
	float splitAngle;
	float randomFrac;
	float forkScale;
	std::string forkScaleProvider;
	std::string searchRadius;
	float defaultSearchRadius;
	bool castingFromHand;
	bool takeTargetsFromSpell;
	bool renewTargetsOnMove;
	float renewTargetsOnMoveFrac;
	float renewSearchEvery;
	int texturesToTile;
	ParticleSound sound;
};

/// A strike from the sky: once, one atom with its groups, where the bolt lives, and the thunder
class LightningStrike final: public Modifier
{
public:
	explicit LightningStrike(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , sound({.action = object.Sound("SoundLightning")})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		auto& atom = effect.NewAtom(collection, effect.FindCreator(creator), nextGroups);
		StartAtomSound(effect, atom, sound);
		return false;
	}

	std::string creator;
	std::vector<int> nextGroups;
	ParticleSound sound;
};
} // namespace

void openblack::particles::RegisterLightningRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_Lightning", ParticleClassRegistry::Make<Lightning>);
	registry.AddModifier("UR_LightningStrike", ParticleClassRegistry::Make<LightningStrike>);
}
