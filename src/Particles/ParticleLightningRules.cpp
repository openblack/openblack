/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The lightning bolt's rules. A bolt from a hand looks for things to strike in a cone in front of the hand, through the
// land's cells in a spiral out from the hand's, and makes up the rest with points on the ground ahead; a creature in the
// cone takes every fork. A bolt from a cloud or a parent atom looks round itself instead, through a quarter as many
// cells, and one its miracle gives targets strikes where they stand; both make up the rest with points on the ground
// anywhere round them, and neither is drawn to creatures. Each step it strikes some of them: a
// trunk of ribbon joints from the hand forks in two, then in two again, until each branch ends on one target, where a
// light flashes on the land and the miracle is told it struck there, hitting everything within reach of the point. What
// it strikes for the first time since it last looked for targets is left with electric arcs crawling over it. The land and
// shields cut forks short. Two players' bolts from their hands that point the same way may clash: both trunks meet at a point,
// where a glow shows, and the older bolt carries on from there, thicker, striking twice as hard for both. The start of a bolt
// from this computer's hand keeps up with the hand between steps. A strike from the sky is one atom with its bolt under it.

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "3D/MapCoords.h"
#include "LightningMaths.h"
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
/// Ground points are found within this share of the search radius: from a hand up to an eighth of a turn round from the
/// heading, from a cloud, a parent or a miracle's targets anywhere round
constexpr float k_GroundPointReach = 0.6f;
constexpr float k_GroundPointSpread = std::numbers::pi_v<float> / 4.0f;
/// A fork splits between these shares of the way to its targets' middle
constexpr float k_SplitShareMin = 0.2f;
constexpr float k_SplitShareRange = 0.4f;
/// A fork's joints are half to fully opaque, and fully so on a fork thicker than plain
constexpr int32_t k_JointAlphaRange = 0x7F;
constexpr uint8_t k_JointAlphaMin = 0x80;
constexpr uint8_t k_JointAlphaThick = 0xFF;
/// What a bolt strikes for the first time since it last looked for targets, once that is this long ago, is left with arcs
/// crawling over it
constexpr float k_ArcsAfterSeconds = 0.2f;

/// One thing a bolt may strike
struct BoltTarget
{
	entt::entity object {entt::null};
	/// A ground point, or where the object stood when found
	glm::vec3 ground {0.0f};
	float height {0.0f};
	/// A creature not faded away, which draws the bolt
	bool drawsBolt {false};
	/// Arcs may crawl over it, and have since the targets were looked for
	bool arcs {false};
	bool arced {false};
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
	/// The first search's count, which later searches keep to
	std::optional<size_t> firstCount;
	/// The single atom whose collections are the forks
	const Atom* root {nullptr};
	/// The joint the clash's glow shows on
	Atom* glowJoint {nullptr};
	/// How it shows itself to other bolts from hands, to clash
	std::shared_ptr<BoltShare> share;
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
	    , commonGlowGroup(object.Int("CommonGlowGroup", -1))
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
			Unlink(bolt);
			bolt.share.reset();
			collection.atoms.clear();
			bolt.root = nullptr;
			bolt.glowJoint = nullptr;
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
		Clash(effect, bolt);
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
		// Each search starts its targets afresh, none of them yet with arcs
		bolt.targets.clear();
		bolt.life = 0.0f;
		bolt.searchOrigin = bolt.origin;
		const float radius = SearchRadius(effect);
		auto& world = effect.Services().world;
		// Later searches find no more than the first did
		const auto most = static_cast<size_t>(bolt.firstCount.value_or(static_cast<size_t>(maxObjects)));
		const auto take = [&bolt](const StrikeCandidate& candidate) {
			bolt.targets.push_back({.object = candidate.object,
			                        .ground = candidate.position,
			                        .height = candidate.height,
			                        .drawsBolt = candidate.drawsBolt,
			                        .arcs = candidate.arcs});
		};
		const auto within = [&bolt, radius](glm::vec3 point) {
			const float dx = point.x - bolt.origin.x;
			const float dz = point.z - bolt.origin.z;
			return dx * dx + dz * dz < radius * radius;
		};
		if (castingFromHand)
		{
			// In the cone in front of the hand
			const float cosHalfAngle = std::cos(splitAngle);
			for (const auto& candidate : world.StrikeCandidates(bolt.origin, maths::StrikeSearchCells(radius)))
			{
				if (bolt.targets.size() >= most)
				{
					break;
				}
				if (maths::InStrikeCone(bolt.origin, bolt.heading, cosHalfAngle, candidate.position))
				{
					take(candidate);
				}
			}
		}
		else if (!takeTargetsFromSpell)
		{
			// Anything round it, within the radius across the land
			for (const auto& candidate : world.StrikeCandidates(bolt.origin, maths::CloudSearchCells(radius)))
			{
				if (bolt.targets.size() >= most)
				{
					break;
				}
				if (within(candidate.position))
				{
					take(candidate);
				}
			}
		}
		else
		{
			// Where each of its miracle's targets stands, as a point rather than the object, those within the radius
			for (auto target = effect.TakeTarget(); target.has_value(); target = effect.TakeTarget())
			{
				if (const auto info = world.Target(*target, false))
				{
					const glm::vec3 point(openblack::map_coords::Quantise(info->position.x), info->position.y,
					                      openblack::map_coords::Quantise(info->position.z));
					if (within(point))
					{
						bolt.targets.push_back({.object = entt::null, .ground = point});
					}
				}
			}
		}
		// The rest are points on the ground: ahead of a hand, anywhere round otherwise
		while (static_cast<int>(bolt.targets.size()) < minObjects)
		{
			const float angle = castingFromHand ? bolt.heading + effect.Random(k_GroundPointSpread)
			                                    : effect.Random(2.0f * std::numbers::pi_v<float>);
			const float distance = effect.Random(k_GroundPointReach * radius);
			glm::vec3 point(bolt.origin.x + distance * std::cos(angle), 0.0f, bolt.origin.z + distance * std::sin(angle));
			point.y = world.LandHeight({point.x, point.z}) + k_GroundPointHeight;
			bolt.targets.push_back({.object = entt::null, .ground = point});
		}
		// A creature among a hand's targets takes every fork
		if (castingFromHand)
		{
			std::vector<bool> drawsBolt;
			drawsBolt.reserve(bolt.targets.size());
			for (const auto& target : bolt.targets)
			{
				drawsBolt.push_back(target.drawsBolt);
			}
			const auto pointed = maths::PreferCreatures(drawsBolt);
			const auto found = bolt.targets;
			for (size_t i = 0; i < bolt.targets.size(); ++i)
			{
				bolt.targets[i] = found[pointed[i]];
			}
		}
		if (!bolt.firstCount.has_value())
		{
			bolt.firstCount = bolt.targets.size();
		}
		glm::vec3 sum(0.0f);
		for (const auto& target : bolt.targets)
		{
			sum += TipOf(effect, target);
		}
		bolt.centroid = sum / static_cast<float>(bolt.targets.size());
		bolt.centroidDistance = glm::length(bolt.centroid - bolt.origin);
	}

	/// One root atom, carrying two forks for each target it may strike at once and two more, each fork a ribbon of joints
	/// drawn as the step leaves them
	void Build(Effect& effect, Collection& collection, Bolt& bolt) const
	{
		collection.atoms.clear();
		bolt.glowJoint = nullptr;
		const auto* jointCreator = effect.FindCreator(creator);
		auto& root = effect.NewAtom(collection, jointCreator, {});
		root.visible = false;
		const std::vector<int> groups(maths::ForkCount(atOnce, bolt.targets.size()), forkGroup);
		effect.AddSubCollections(root, groups);
		for (auto& fork : root.subCollections)
		{
			fork->interpolated = false;
			for (int i = 0; i < maxJoints; ++i)
			{
				auto& joint = effect.NewAtom(*fork, jointCreator, {});
				// Only the trunk's start keeps up with the hand between steps
				joint.drawWeight = 0.0f;
			}
		}
		bolt.root = &root;
		// A bolt from a hand shows itself to the others, which it may clash with
		if (castingFromHand && !bolt.share)
		{
			auto& world = effect.Services().world;
			bolt.share = std::make_shared<BoltShare>();
			bolt.share->owner = &effect;
			bolt.share->order = world.NextBoltOrder();
			world.AddBolt(bolt.share);
		}
	}

	/// The bolt's pose for the others, and whether it meets an older bolt from a hand: the newer one looks
	void Clash(Effect& effect, Bolt& bolt) const
	{
		if (!bolt.share)
		{
			return;
		}
		auto& share = *bolt.share;
		share.origin = bolt.origin;
		share.centroid = bolt.centroid;
		share.heading = bolt.heading;
		share.hasTargets = !bolt.targets.empty();
		// An older bolt whose newer one has gone shows no glow
		if (share.linkedTo.expired() && bolt.glowJoint != nullptr)
		{
			bolt.glowJoint->subCollections.clear();
			bolt.glowJoint = nullptr;
		}
		// One linked to a newer bolt doesn't look for another
		if (!share.linkedTo.expired())
		{
			return;
		}
		share.meeting.reset();
		const float reach = SearchRadius(effect);
		for (const auto& other : effect.Services().world.Bolts())
		{
			if (other.get() == &share || !other->hasTargets || other->order > share.order)
			{
				continue;
			}
			const auto otherLink = other->linkedTo.lock();
			if (otherLink != nullptr && otherLink.get() != &share)
			{
				continue;
			}
			const auto meeting =
			    maths::ClashPoint({.origin = share.origin, .centroid = share.centroid, .heading = share.heading},
			                      {.origin = other->origin, .centroid = other->centroid, .heading = other->heading}, reach);
			if (meeting.has_value())
			{
				share.meeting = meeting;
				other->linkedTo = bolt.share;
				share.linkedFrom = other;
				return;
			}
			if (otherLink != nullptr)
			{
				other->linkedTo.reset();
				share.linkedFrom.reset();
			}
		}
	}

	/// A bolt going lets go of the bolts it clashed with
	static void Unlink(Bolt& bolt)
	{
		if (!bolt.share)
		{
			return;
		}
		if (const auto older = bolt.share->linkedFrom.lock())
		{
			older->linkedTo.reset();
		}
		bolt.share->linkedTo.reset();
		bolt.share->linkedFrom.reset();
	}

	/// Some of the targets are picked, each with an even chance, and the forks are laid out to them from the origin. In a
	/// clash the trunk ends where the bolts meet: the newer one stops there, and the older one strikes a single target
	/// from there, thicker and twice as hard, for both.
	void Strike(Effect& effect, Atom& root, Bolt& bolt) const
	{
		const auto total = static_cast<int>(bolt.targets.size());
		if (root.subCollections.empty() || total == 0)
		{
			return;
		}
		size_t nextFork = 1;
		if (bolt.share && bolt.share->meeting.has_value())
		{
			// The newer bolt of a clash: only its trunk, to where they meet
			LayJoints(effect, bolt, *root.subCollections.front(), 0, bolt.origin, *bolt.share->meeting, 1.0f);
			return;
		}
		if (const auto newer = bolt.share ? bolt.share->linkedTo.lock() : nullptr; newer && newer->meeting.has_value())
		{
			const auto meeting = *newer->meeting;
			auto& trunk = *root.subCollections.front();
			LayJoints(effect, bolt, trunk, 0, bolt.origin, meeting, 1.0f);
			// The glow where they meet, on the trunk's last joint
			auto* last = trunk.atoms.back().get();
			if (bolt.glowJoint != last && commonGlowGroup >= 0)
			{
				if (bolt.glowJoint != nullptr)
				{
					bolt.glowJoint->subCollections.clear();
				}
				const std::vector<int> glow {commonGlowGroup};
				effect.AddSubCollections(*last, glow);
				for (auto& collection : last->subCollections)
				{
					collection->interpolated = false;
				}
				bolt.glowJoint = last;
			}
			// A single target, from a random start
			const auto first = static_cast<size_t>(effect.Rand(total));
			const std::vector<size_t> single {first};
			if (nextFork < root.subCollections.size())
			{
				auto& fork = *root.subCollections[nextFork++];
				Fork(effect, root, bolt, 1, fork, meeting, single, maths::k_ClashForkScale, nextFork, newer->owner);
			}
			return;
		}
		const int limit = maths::StrikesAtOnce(atOnce, total);
		std::vector<size_t> striking;
		const float chance = static_cast<float>(limit) / static_cast<float>(total);
		for (int i = 0; i < total && static_cast<int>(striking.size()) < limit; ++i)
		{
			if (effect.Random(1.0f) < chance)
			{
				striking.push_back(static_cast<size_t>(i));
			}
		}
		if (striking.empty())
		{
			return;
		}
		Fork(effect, root, bolt, 0, *root.subCollections.front(), bolt.origin, striking, 1.0f, nextFork, nullptr);
	}

	/// One fork: from its origin to where its targets split, then a fork for each side from there, or the strike when it
	/// holds a single target. In a clash the strike is also sent to the other bolt's miracle.
	void Fork(Effect& effect, Atom& root, Bolt& bolt, int depth, Collection& fork, const glm::vec3& origin,
	          const std::vector<size_t>& targets, float scale, size_t& nextFork, const Effect* clashed) const
	{
		const auto joints = static_cast<int>(fork.atoms.size());
		if (targets.empty() || joints < 2)
		{
			return;
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
		const float strength = clashed != nullptr ? maths::k_ClashStrength : 1.0f;
		// A shield in the way stops the fork on its surface unless the miracle gets through
		bool stopped = false;
		if (const auto shield = world.FindShield(split, k_ShieldMargin); shield && shield->owner != &effect)
		{
			const auto hit = maths::SphereEntry(origin, split, shield->centre, shield->radius, k_ShieldMargin);
			const bool through = effect.SendSpellEvent({.type = SpellEventInfo::Type::HitSpell,
			                                            .position = hit,
			                                            .velocity = glm::vec3(0.0f),
			                                            .strength = strength,
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
				Fork(effect, root, bolt, depth + 1, child, end, list, scale, nextFork, clashed);
			}
			return;
		}
		Hit(effect, bolt, bolt.targets[targets.front()], centroid, strength, clashed);
	}

	/// The joints laid along the fork, the inner ones jittered across, thinning with depth and half to fully opaque (fully
	/// on a thick fork). The trunk of a bolt from this computer's hand keeps up with the hand between steps, all of it at
	/// the hand and less and less of it towards its end.
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
		const bool trunk = depth == 0 && &fork == bolt.root->subCollections.front().get();
		const bool glued = trunk && castingFromHand && effect.IsMyInterfaceCasting();
		const float step = 1.0f / static_cast<float>(joints - 1);
		for (int i = 0; i < joints; ++i)
		{
			auto& joint = *fork.atoms[static_cast<size_t>(i)];
			joint.visible = true;
			const float t = static_cast<float>(i) * step;
			auto position = origin + way * t;
			if (i != 0 && i != joints - 1)
			{
				position.x += (effect.Random(2.0f * randomFrac) - randomFrac) * length;
				position.z += (effect.Random(2.0f * randomFrac) - randomFrac) * length;
			}
			joint.position = position;
			joint.ruleScale = maths::ForkJointScale(scale * bolt.forkScale, depth, t);
			joint.rgba[3] =
			    scale > 1.0f ? k_JointAlphaThick : static_cast<uint8_t>(effect.Rand(k_JointAlphaRange) + k_JointAlphaMin);
			joint.drawWeight = glued ? 1.0f - t : 0.0f;
		}
	}

	/// The light on the land under the struck targets, and the miracle told where the bolt struck: everything within
	/// reach of the point takes the strike. What it strikes for the first time since it last looked for targets, once
	/// that is a moment ago, is left with arcs crawling over it.
	void Hit(Effect& effect, Bolt& bolt, BoltTarget& target, const glm::vec3& centroid, float strength,
	         const Effect* clashed) const
	{
		if (lightMapGroup >= 0)
		{
			if (auto* light = effect.NewAtomInGroup(lightMapGroup, effect.FindCreator(lightMapCreator)))
			{
				light->position =
				    glm::vec3(centroid.x, effect.Services().world.LandHeight({centroid.x, centroid.z}), centroid.z);
				light->drawWeight = 0.0f;
			}
		}
		const SpellEventInfo event {.type = SpellEventInfo::Type::Landed,
		                            .position = TipOf(effect, target),
		                            .velocity = glm::vec3(0.0f),
		                            .strength = strength,
		                            .checkShields = false,
		                            .target = entt::null};
		if (effect.GetSink() != nullptr)
		{
			effect.SendSpellEvent(event);
		}
		else
		{
			effect.Services().world.StrikeWithoutMiracle(event.position);
		}
		if (clashed != nullptr)
		{
			clashed->SendSpellEvent(event);
		}
		if (target.object != entt::null && target.arcs && !target.arced && bolt.life > k_ArcsAfterSeconds)
		{
			target.arced = true;
			effect.Services().world.QueueArcs(target.object);
		}
	}

	std::string creator;
	std::string lightMapCreator;
	int forkGroup;
	int commonGlowGroup;
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
