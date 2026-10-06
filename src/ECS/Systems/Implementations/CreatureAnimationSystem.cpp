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

#include <glm/gtx/transform.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Creature/CreatureAnimation.h"
#include "Creature/CreatureEyes.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureRig.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
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

CreatureRig::Mesh WeakStrongMesh(float value)
{
	return value < 0.0f ? CreatureRig::Mesh::Weak : CreatureRig::Mesh::Strong;
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

glm::mat4 ModelMatrix(const Transform& transform)
{
	// As the renderer places the creature
	auto model = glm::mat4(transform.rotation);
	model = glm::translate(model, transform.position * transform.rotation);
	return glm::scale(model, transform.scale);
}

/// An eye's point on the body as drawn: each vertex blended as the body is, placed by its bone, in the world
std::optional<creature_eyes::SurfacePoint> EyePointOf(const CreatureRig::EyePoint& point, const creature_morph::Morph& morph,
                                                      const std::vector<glm::mat4>& bones, const glm::mat4& model)
{
	if (!point.enabled)
	{
		return std::nullopt;
	}
	std::array<glm::vec3, 3> world {};
	for (size_t i = 0; i < 3; ++i)
	{
		const auto vertexOf = [&point, i](CreatureRig::Mesh mesh) {
			return point.vertices.at(static_cast<size_t>(mesh)).at(i);
		};
		const auto local =
		    creature_morph::Blend(vertexOf(CreatureRig::Mesh::Base), vertexOf(EvilGoodMesh(morph.evilGood)),
		                          vertexOf(ThinFatMesh(morph.thinFat)), vertexOf(WeakStrongMesh(morph.weakStrong)), morph);
		const auto bone = point.bones.at(i);
		const auto& boneMatrix = bone < bones.size() ? bones[bone] : glm::mat4(1.0f);
		world.at(i) = glm::vec3(model * boneMatrix * glm::vec4(local, 1.0f));
	}
	return creature_eyes::PointOnTriangle(world[0], world[1], world[2], point.u, point.v);
}

void PlaceEyes(CreatureEyes& eyes, const CreatureRig::Eyes& rig, const creature_morph::Morph& morph,
               const std::vector<glm::mat4>& bones, const Transform& transform, float size, float seconds)
{
	const auto model = ModelMatrix(transform);
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
} // namespace

void CreatureAnimationSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const Creature, CreatureMorph>([](const Creature& creature, CreatureMorph& morph) {
		morph.shownFatness = creature_morph::EaseFatness(morph.shownFatness, creature.fatness);
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
		    }

		    // Standing, the creature breathes in and out over its stand animation
		    const auto targetPeriod = creature_animation::BreathPeriod(creature.size);
		    animation.breathPeriod = creature_animation::EaseBreathPeriod(animation.breathPeriod, targetPeriod, seconds);
		    animation.breathPhase = creature_animation::AdvanceBreath(animation.breathPhase, seconds, animation.breathPeriod);

		    if (rig != nullptr)
		    {
			    auto stand = animation.animations.find(creature_animation::k_StandAnimation);
			    if (stand == animation.animations.end())
			    {
				    if (auto blended = BlendAnimation(*rig, morph.drawn, creature_animation::k_StandAnimation))
				    {
					    stand = animation.animations.emplace(creature_animation::k_StandAnimation, std::move(*blended)).first;
				    }
			    }
			    if (stand != animation.animations.end() && !stand->second.frames.empty())
			    {
				    const auto& standAnimation = stand->second;
				    const auto time = creature_animation::BreathTime(animation.breathPhase, standAnimation.duration);
				    const auto poses =
				        skeletal_animation::SampleCycle(standAnimation, standAnimation, time, animation.skeleton);
				    animation.boneMatrices = skeletal_animation::ComposeBoneMatrices(poses, animation.skeleton.parents);
			    }
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
