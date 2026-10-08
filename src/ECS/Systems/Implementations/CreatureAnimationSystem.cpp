/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureAnimationSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>

#include <glm/gtx/transform.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Creature/CreatureAnimation.h"
#include "Creature/CreatureEyes.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLook.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureRig.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::creature::CreatureRig;
using openblack::skeletal_animation::Animation;

namespace
{
/// The meshes look back along their z axis, so the eyes look ahead along -z when the creature looks nowhere in
/// particular
constexpr glm::vec3 k_MeshBack {0.0f, 0.0f, 1.0f};
/// How fast the head speeds up turning to look, in radians a second each second. The game reads its rate from the
/// species' tables; this stands in until that field is known.
constexpr float k_HeadAcceleration = 4.0f;
/// The head is turned when more than this far round, in radians
constexpr float k_LookSettled = 1e-4f;

creature_morph::Morph TargetMorph(const Creature& creature, const CreatureMorph& morph)
{
	return creature_morph::FromAttributes(creature.alignment, morph.shownFatness, creature.strength,
	                                      ecs::archetypes::CreatureArchetype::SpeciesStrength(creature.species));
}

CreatureRig::Mesh EvilGoodMesh(float value)
{
	return value < 0.0f ? CreatureRig::Mesh::Evil : CreatureRig::Mesh::Good;
}

CreatureRig::Mesh ThinFatMesh(float value)
{
	return value < 0.0f ? CreatureRig::Mesh::Thin : CreatureRig::Mesh::Fat;
}

/// A variant's animation, the base's adjusted to the variant's stand where the variant has none of its own, and the
/// variant's stand
struct VariantAnimation
{
	std::optional<Animation> adjusted;
	const Animation* animation {nullptr};
	const Animation* stand {nullptr};
};

VariantAnimation VariantOf(const CreatureRig& rig, CreatureRig::Mesh mesh, size_t index)
{
	VariantAnimation variant;
	const auto slot = static_cast<size_t>(mesh);
	if (!rig.hasMesh.at(slot) || slot >= rig.animations.size())
	{
		return variant;
	}
	const auto* base = rig.GetAnimation(CreatureRig::Mesh::Base, index);
	const auto* baseStand = rig.GetAnimation(CreatureRig::Mesh::Base, creature_animation::k_StandAnimation);
	variant.stand = rig.GetAnimation(mesh, creature_animation::k_StandAnimation);
	const auto& own = rig.animations.at(slot);
	if (index < own.size() && own[index].has_value())
	{
		variant.animation = &*own[index];
	}
	else if (index == creature_animation::k_StandAnimation)
	{
		variant.animation = variant.stand;
	}
	else if (base != nullptr && baseStand != nullptr && variant.stand != nullptr)
	{
		variant.adjusted = creature_animation::AdjustFromStand(*base, *baseStand, *variant.stand);
		variant.animation = &*variant.adjusted;
	}
	return variant;
}

/// The species' animation blended as the body is drawn
std::optional<Animation> BlendAnimation(const CreatureRig& rig, const creature_morph::Morph& morph, size_t index)
{
	const auto* base = rig.GetAnimation(CreatureRig::Mesh::Base, index);
	if (base == nullptr)
	{
		return std::nullopt;
	}
	const auto evilGood = VariantOf(rig, EvilGoodMesh(morph.evilGood), index);
	const auto thinFat = VariantOf(rig, ThinFatMesh(morph.thinFat), index);
	return creature_animation::Blend(*base,
	                                 {.animation = evilGood.animation,
	                                  .stand = evilGood.stand,
	                                  .weight = evilGood.animation != nullptr ? std::abs(morph.evilGood) : 0.0f},
	                                 {.animation = thinFat.animation,
	                                  .stand = thinFat.stand,
	                                  .weight = thinFat.animation != nullptr ? std::abs(morph.thinFat) : 0.0f});
}

/// An eye's point on the body as drawn: each vertex blended as the body is, placed by its bone, in the world
std::optional<creature_eyes::SurfacePoint> EyePointOf(const CreatureRig::EyePoint& point, const creature_morph::Morph& morph,
                                                      const std::vector<glm::mat4>& bones, const glm::mat4& model)
{
	if (!point.enabled)
	{
		return std::nullopt;
	}
	const auto world = creature::PosedTriangle(point.vertices, point.bones, morph, bones, model);
	return creature_eyes::PointOnTriangle(world[0], world[1], world[2], point.u, point.v);
}

void PlaceEyes(CreatureEyes& eyes, const CreatureRig::Eyes& rig, const creature_morph::Morph& morph,
               const std::vector<glm::mat4>& bones, const Transform& transform, float size, float seconds)
{
	const auto model = creature::PlacementMatrix(transform.position, transform.rotation, transform.scale);
	const auto eyeSize = creature_eyes::EyeSize(size, rig.scale, morph.evilGood, eyes.mode);
	const auto ahead = glm::normalize(transform.rotation * k_MeshBack);
	bool lidColourSet = false;
	for (size_t i = 0; i < eyes.drawn.size(); ++i)
	{
		auto& drawn = eyes.drawn.at(i);
		drawn = {};
		const auto point = EyePointOf(rig.points.at(i), morph, bones, model);
		if (!point || glm::length(point->normal) <= 0.0f)
		{
			continue;
		}
		const auto normal = glm::normalize(point->normal);
		const auto centre = point->position - (rig.points.at(i).depth * normal);

		const bool looksAtPoint =
		    eyes.lookAt.has_value() && eyes.mode != creature_eyes::Mode::Ahead && glm::length(centre - *eyes.lookAt) > 0.0f;
		const auto away = creature_eyes::ClampLook(looksAtPoint ? glm::normalize(centre - *eyes.lookAt) : ahead, normal);
		auto& look = eyes.look.at(i);
		if (!eyes.lookStarted.at(i))
		{
			look.Reset(away);
			eyes.lookStarted.at(i) = true;
		}
		look.SetDestination(away, creature_eyes::LookSeconds(eyes.openness, eyes.mode));
		look.Update(seconds);
		const auto eyeball = creature_eyes::EyeballFrame(centre, look.GetValue());
		drawn.eyeball = creature_eyes::ToMatrix(eyeball, eyeSize);

		const auto anchor = EyePointOf(rig.points.at(i + 2), morph, bones, model);
		if (!anchor)
		{
			continue;
		}
		// Both lids take the skin's colour under the first one drawn
		if (!lidColourSet)
		{
			eyes.lidColour = rig.points.at(i + 2).skinColour;
			lidColourSet = true;
		}
		const auto lid = creature_eyes::EyelidFrame(centre, normal, anchor->position, i == 1);
		const auto angle = creature_eyes::EyelidAngle(eyes.mode, rig.lidAngles, eyes.openness,
		                                              creature_eyes::LidPitch(lid, eyeball), eyes.blink);
		drawn.eyelid = creature_eyes::ToMatrix(creature_eyes::Turned(lid, angle), eyeSize);
	}
}
/// A blended animation of the creature's, blended the first time it is wanted, or nothing when the species has none
const Animation* AnimationOf(CreatureAnimation& animation, const CreatureRig& rig, const creature_morph::Morph& morph,
                             size_t index)
{
	auto found = animation.animations.find(index);
	if (found == animation.animations.end())
	{
		// A species without the animation keeps an empty one, so as not to look for it again
		auto blended = BlendAnimation(rig, morph, index).value_or(Animation {});
		found = animation.animations.emplace(index, std::move(blended)).first;
	}
	return found->second.frames.empty() ? nullptr : &found->second;
}

std::optional<uint32_t> DurationOf(const Animation* animation)
{
	return animation != nullptr ? std::optional(animation->duration) : std::nullopt;
}

/// Plays a head turning animation on top of the pose for how far the head is turned, unless it looks straight on
void AddLook(std::vector<skeletal_animation::Pose>& poses, const Animation* look, float angle, float limit,
             const CreatureAnimation& animation, std::span<const uint32_t> mirror)
{
	if (look == nullptr || std::abs(angle) <= k_LookSettled)
	{
		return;
	}
	const auto reference = (look->frames.size() - 1) / 2;
	skeletal_animation::AddLayer(poses, *look, creature_layers::LookTime(angle, limit, look->duration), reference,
	                             animation.skeleton, mirror);
}

/// The body posed for this frame: its action or breathing, or its slots blended, then the head turned, the face and
/// any gesture on top
void PoseBody(CreatureAnimation& animation, const CreatureRig& rig, const creature_morph::Morph& morph,
              const Transform& transform, float size, float milliseconds, float seconds)
{
	const auto* stand = AnimationOf(animation, rig, morph, creature_layers::animations::k_Stand);
	if (stand == nullptr)
	{
		return;
	}
	const auto playbackMs = milliseconds * creature_layers::PlaybackRate(size);
	const auto animationOf = [&](size_t index) { return AnimationOf(animation, rig, morph, index); };

	animation.body = creature_layers::AdvanceBody(animation.body, playbackMs,
	                                              DurationOf(animationOf(creature_layers::CurrentAnimation(animation.body))));
	animation.face = creature_layers::AdvanceFace(
	    animation.face, milliseconds,
	    DurationOf(animation.face.current.has_value() ? animationOf(*animation.face.current) : nullptr));
	animation.gesture = creature_layers::AdvanceGesture(
	    animation.gesture, playbackMs,
	    DurationOf(animation.gesture.animation.has_value() ? animationOf(*animation.gesture.animation) : nullptr));
	animation.wobble = creature_layers::AdvanceGesture(
	    animation.wobble, playbackMs,
	    DurationOf(animation.wobble.animation.has_value() ? animationOf(*animation.wobble.animation) : nullptr));

	// The head turns towards where the creature looks, from its eyes' height
	const auto ahead = -(transform.rotation * k_MeshBack);
	const auto head = transform.position + glm::vec3(0.0f, creature_look::k_HeadHeight * size, 0.0f);
	const auto angles = animation.lookAt.has_value() ? creature_layers::AnglesTowards(head, ahead, *animation.lookAt)
	                                                 : creature_layers::LookAngles {.yaw = 0.0f, .pitch = 0.0f};
	animation.yaw =
	    creature_layers::TurnHead(animation.yaw, angles.yaw, k_HeadAcceleration, seconds, creature_layers::k_YawLimit);
	animation.pitch =
	    creature_layers::TurnHead(animation.pitch, angles.pitch, k_HeadAcceleration, seconds, creature_layers::k_PitchLimit);

	const std::span<const uint32_t> mirror = animation.body.mirrored ? animation.mirror : std::span<const uint32_t> {};
	std::vector<skeletal_animation::Pose> poses;
	if (!animation.slots.empty())
	{
		std::vector<std::vector<skeletal_animation::Pose>> sampled;
		std::vector<float> weights;
		for (const auto& slot : animation.slots)
		{
			const auto* played = animationOf(slot.animation);
			sampled.push_back(skeletal_animation::SampleCycle(
			    played != nullptr ? *played : *stand, *stand, static_cast<uint32_t>(std::max(slot.timeMs, 0.0f)),
			    animation.skeleton, slot.mirrored ? animation.mirror : std::span<const uint32_t> {}));
			weights.push_back(slot.weight);
		}
		poses = skeletal_animation::WeightedSum(sampled, weights);
	}
	else if (const auto* played = creature_layers::IsPlaying(animation.body)
	                                  ? animationOf(creature_layers::CurrentAnimation(animation.body))
	                                  : nullptr;
	         played != nullptr)
	{
		poses = skeletal_animation::SampleCycle(*played, *stand, static_cast<uint32_t>(std::max(animation.body.timeMs, 0.0f)),
		                                        animation.skeleton, mirror);
	}
	else
	{
		const auto time = creature_animation::BreathTime(animation.breathPhase, stand->duration);
		poses = skeletal_animation::SampleCycle(*stand, *stand, time, animation.skeleton);
	}

	// Sitting, the head turns by the sitting versions. The head turns the other way when the body plays mirrored.
	const bool sitting = creature_layers::CurrentAnimation(animation.body) == creature_layers::animations::k_Sit;
	AddLook(poses,
	        animationOf(sitting ? creature_layers::animations::k_SitLookDownUp : creature_layers::animations::k_LookDownUp),
	        animation.pitch.angle, creature_layers::k_PitchLimit, animation, mirror);
	AddLook(
	    poses,
	    animationOf(sitting ? creature_layers::animations::k_SitLookRightLeft : creature_layers::animations::k_LookRightLeft),
	    mirror.empty() ? animation.yaw.angle : -animation.yaw.angle, creature_layers::k_YawLimit, animation, mirror);
	// The face and gestures, relative to their first frames
	if (animation.face.current.has_value())
	{
		if (const auto* face = animationOf(*animation.face.current))
		{
			skeletal_animation::AddLayer(poses, *face, static_cast<uint32_t>(std::max(animation.face.timeMs, 0.0f)), 0,
			                             animation.skeleton);
		}
	}
	if (animation.gesture.animation.has_value())
	{
		if (const auto* gesture = animationOf(*animation.gesture.animation))
		{
			skeletal_animation::AddLayer(poses, *gesture, static_cast<uint32_t>(std::max(animation.gesture.timeMs, 0.0f)), 0,
			                             animation.skeleton);
		}
	}
	// A blow's wobble, relative to its first frame, the way the body plays
	if (animation.wobble.animation.has_value())
	{
		if (const auto* wobble = animationOf(*animation.wobble.animation))
		{
			skeletal_animation::AddLayer(poses, *wobble, static_cast<uint32_t>(std::max(animation.wobble.timeMs, 0.0f)), 0,
			                             animation.skeleton, mirror);
		}
	}
	animation.boneMatrices = skeletal_animation::ComposeBoneMatrices(poses, animation.skeleton.parents);
}
} // namespace

void CreatureAnimationSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const Creature, CreatureMorph>([](const Creature& creature, CreatureMorph& morph) {
		morph.shownFatness = creature_morph::EaseFatness(morph.shownFatness, creature.fatness);
	});
	// Breathing settles towards the period it should be at a step a turn; standing, that is its resting period
	constexpr auto k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
	registry.Each<const Creature, CreatureAnimation>([](const Creature& creature, CreatureAnimation& animation) {
		const auto resting = creature_animation::BreathPeriod(creature.size);
		animation.breathPeriod = creature_animation::EaseBreathPeriod(animation.breathPeriod, resting, resting, k_TurnSeconds);
	});
}

void CreatureAnimationSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& resources = Locator::resources::value();
	const auto& meshes = resources.GetMeshes();
	const auto& rigs = resources.GetCreatureRigs();
	const auto seconds = std::chrono::duration<float>(gameTime).count();
	const auto milliseconds = static_cast<int32_t>(std::lround(gameTime.count()));

	registry.Each<const Creature, CreatureMorph, CreatureAnimation, const Transform>(
	    [&](entt::entity entity, const Creature& creature, CreatureMorph& morph, CreatureAnimation& animation,
	        const Transform& transform) {
		    const auto refresh = creature_morph::RefreshDrawn(morph.drawn, TargetMorph(creature, morph));
		    morph.drawn = refresh.drawn;
		    if (refresh.animations)
		    {
			    ++morph.revision;
		    }

		    const auto bodyMeshes = creature_morph::MeshesOf(creature.species, morph.drawn,
		                                                     [&meshes](entt::id_type id) { return meshes.Contains(id); });
		    if (!meshes.Contains(bodyMeshes.base))
		    {
			    return;
		    }
		    const auto base = meshes.Handle(bodyMeshes.base);
		    const auto rigId = creature::GetRigId(creature.species);
		    const CreatureRig* rig = rigs.Contains(rigId) ? &*rigs.Handle(rigId) : nullptr;

		    // The rest pose and the animations follow the evil to good and thin to fat axes
		    std::vector<glm::mat4> rest;
		    if (animation.builtRevision != morph.revision || animation.skeleton.Empty())
		    {
			    const auto evilGood = meshes.Handle(bodyMeshes.evilGood);
			    const auto thinFat = meshes.Handle(bodyMeshes.thinFat);
			    rest = creature_animation::BlendRest(base->GetBoneMatrices(), evilGood->GetBoneMatrices(),
			                                         std::abs(morph.drawn.evilGood), thinFat->GetBoneMatrices(),
			                                         std::abs(morph.drawn.thinFat));
			    animation.skeleton = skeletal_animation::Skeleton::FromRestMatrices(base->GetBoneParents(), rest);
			    animation.animations.clear();
			    animation.builtRevision = morph.revision;
			    animation.boneMatrices = rest;
			    // Each bone's mirror as the species' file has it
			    auto mirror = rig != nullptr ? rig->MirrorBones(rest.size()) : std::nullopt;
			    animation.mirror = mirror.has_value() ? std::move(*mirror) : skeletal_animation::MirrorJoints(rest);
		    }

		    // Standing, the creature breathes in and out over its stand animation, from its first frame at its resting
		    // period
		    if (animation.breathPeriod <= 0.0f)
		    {
			    animation.breathPeriod = creature_animation::BreathPeriod(creature.size);
		    }
		    // A frozen creature plays slower, down to not at all
		    const float scale = animation.playbackScale;
		    animation.breathPhase =
		        creature_animation::AdvanceBreath(animation.breathPhase, seconds * scale, animation.breathPeriod);

		    if (rig != nullptr)
		    {
			    PoseBody(animation, *rig, morph.drawn, transform, creature.size, gameTime.count() * scale, seconds * scale);
		    }
		    if (animation.boneMatrices.size() != base->GetBoneMatrices().size())
		    {
			    animation.boneMatrices = base->GetBoneMatrices();
		    }

		    auto* eyes = registry.TryGet<CreatureEyes>(entity);
		    if (eyes == nullptr)
		    {
			    return;
		    }
		    eyes->blink = creature_eyes::AdvanceBlink(eyes->blink, milliseconds, [this](uint32_t range) {
			    return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
		    });
		    eyes->drawn = {};
		    if (rig != nullptr && rig->eyes.has_value())
		    {
			    PlaceEyes(*eyes, *rig->eyes, morph.drawn, animation.boneMatrices, transform, creature.size, seconds);
		    }
	    });
}

namespace
{
/// A creature's animation blended as its body is drawn, if it has the animation and has been posed
const Animation* PosedAnimationOf(entt::entity creature, size_t index)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* morph = registry.TryGet<const CreatureMorph>(creature);
	auto* animation = registry.TryGet<CreatureAnimation>(creature);
	if (body == nullptr || morph == nullptr || animation == nullptr || animation->skeleton.Empty())
	{
		return nullptr;
	}
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto rigId = creature::GetRigId(body->species);
	if (!rigs.Contains(rigId))
	{
		return nullptr;
	}
	return AnimationOf(*animation, *rigs.Handle(rigId), morph->drawn, index);
}
} // namespace

std::optional<glm::vec3> CreatureAnimationSystem::BoneInAnimation(entt::entity creature, size_t animation, float timeMs,
                                                                  uint32_t bone, bool mirrored)
{
	const auto* played = PosedAnimationOf(creature, animation);
	const auto* stand = PosedAnimationOf(creature, creature_layers::animations::k_Stand);
	if (played == nullptr || stand == nullptr)
	{
		return std::nullopt;
	}
	const auto& body = Locator::entitiesRegistry::value().Get<CreatureAnimation>(creature);
	if (bone >= body.skeleton.parents.size())
	{
		return std::nullopt;
	}
	const auto time = static_cast<uint32_t>(std::clamp(timeMs, 0.0f, static_cast<float>(std::max(played->duration, 1u) - 1)));
	const auto poses = skeletal_animation::SampleCycle(
	    *played, *stand, time, body.skeleton, mirrored ? std::span<const uint32_t>(body.mirror) : std::span<const uint32_t> {});
	const auto matrices = skeletal_animation::ComposeBoneMatrices(poses, body.skeleton.parents);
	return bone < matrices.size() ? std::optional(glm::vec3(matrices[bone][3])) : std::nullopt;
}

std::optional<float> CreatureAnimationSystem::AnimationDuration(entt::entity creature, size_t animation)
{
	const auto* played = PosedAnimationOf(creature, animation);
	return played != nullptr ? std::optional(static_cast<float>(played->duration)) : std::nullopt;
}
