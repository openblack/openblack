/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ShieldRules.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

#include "InfoConstants.h"
#include "VillagerReactionRules.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The animations a villager amazed by a shield plays: pointing up into it, looking at its hand, standing
constexpr uint32_t k_IntoPointing = 286;
constexpr uint32_t k_LookAtHand = 311;
constexpr uint32_t k_Stand = 385;
/// The sheltering villager's turn either side of its own side of the shield
constexpr float k_ShelterTurnHalf = shield::k_ShelterTurnRange * 0.5f;

float Wrap(float angle)
{
	angle = std::fmod(angle, k_TwoPi);
	return angle < 0.0f ? angle + k_TwoPi : angle;
}
} // namespace

float shield::SphereRadius(float radius)
{
	return std::clamp(radius * k_SphereRadiusScale, 0.0f, k_MaxSphereRadius);
}

shield::DomeShape shield::MakeDomeShape(const GMagicShieldInfo& info, float radius, float castSpin)
{
	const float finalScale = k_DomeScalePerRadius * radius;
	const float startSpin = std::clamp(castSpin, -k_DomeMaxSpin, k_DomeMaxSpin);
	return {
	    .finalScale = finalScale,
	    .startScale = finalScale * k_DomeStartFraction,
	    .startSpin = startSpin,
	    .endSpin = startSpin < 0.0f ? -k_DomeEndSpin : k_DomeEndSpin,
	    .raiseWithScale = info.raiseWithScale,
	    .shieldHeight = info.shieldHeight,
	    .bobMagnitude = info.bobMagnitude,
	};
}

shield::DomePose shield::StepDome(const DomeShape& shape, DomeState& state, float age, float dt)
{
	DomePose pose;
	float amount = 1.0f;
	if (age < k_DomeHiddenSeconds)
	{
		// Hidden, not yet turning
		amount = 1.0f - (age / k_DomeHiddenSeconds);
	}
	else
	{
		const float shown = age - k_DomeHiddenSeconds;
		amount = shown < k_DomeGrowSeconds ? Ease(shown / k_DomeGrowSeconds) : 1.0f;
		// The spin eases from the hand's to the slow end spin; past the ease's end it stays there
		const float slowing = Ease(std::min(shown / k_DomeSpinDownSeconds, 1.0f));
		state.angle = Wrap(state.angle + (shape.startSpin + (shape.endSpin - shape.startSpin) * slowing) * dt);
		pose.drawn = true;
	}
	if (state.dying)
	{
		state.dieTime += dt;
		if (state.dieTime > k_DomeGoneSeconds)
		{
			pose.gone = true;
		}
		else
		{
			amount -= amount * std::clamp(state.dieTime / k_DomeFadeSeconds, 0.0f, 1.0f);
		}
	}
	pose.scale = shape.startScale + (shape.finalScale - shape.startScale) * amount;
	state.bob = Wrap(state.bob + k_DomeBobSpeed * dt);
	pose.height = (shape.raiseWithScale * shape.finalScale) + shape.shieldHeight +
	              ((std::sin(state.bob) + 1.0f) * shape.bobMagnitude * pose.scale * 0.5f);
	pose.angle = state.angle;
	return pose;
}

float shield::DomeAlpha(float strength)
{
	const auto byte = static_cast<uint8_t>(std::clamp(std::min(strength, 1.0f) * 255.0f, 0.0f, 255.0f));
	return std::max(k_DomeLeastAlpha, static_cast<float>(byte));
}

float shield::ThrownMass(float weight, float scale)
{
	return std::max(scale * scale * scale * weight, 0.01f);
}

float shield::DyingEffectAlpha(float alpha, float dieTime)
{
	return dieTime > k_DomeFadeSeconds ? 0.0f : alpha;
}

bool shield::NeedsRescale(float drawnScale, float solidScale)
{
	return std::abs(drawnScale - solidScale) > k_DomeRescaleStep;
}

shield::DomeVolume shield::VolumeOf(glm::vec3 meshHalfExtent, float scale)
{
	return {.radius = std::max(meshHalfExtent.x, meshHalfExtent.z) * scale, .height = 2.0f * meshHalfExtent.y * scale};
}

bool shield::WithinSpiritualShield(glm::vec3 shieldOnLand, float radius, glm::vec3 pointOnLand)
{
	const auto d = pointOnLand - shieldOnLand;
	return radius * radius > glm::dot(d, d);
}

bool shield::WithinPhysicalShield(const DomeVolume& volume, float distanceAcross, float heightAboveLand)
{
	if (!(distanceAcross * distanceAcross < volume.radius * volume.radius))
	{
		return false;
	}
	return volume.height * (1.0f - distanceAcross / volume.radius) > heightAboveLand;
}

bool shield::InsideDome(const DomeVolume& volume, float distanceAcross, float heightAboveLand)
{
	if (!(volume.radius > 0.0f) || distanceAcross >= volume.radius)
	{
		return false;
	}
	return heightAboveLand <= volume.height - (volume.height * distanceAcross / volume.radius);
}

std::vector<std::array<glm::vec3, 3>> shield::DomeHull(std::span<const std::array<glm::vec3, 3>> modelTriangles,
                                                       glm::vec3 ground, const DomeShape& shape)
{
	// The game works out the height while the dome's scale is still 1
	const glm::vec3 origin(ground.x, ground.y + shape.shieldHeight + shape.raiseWithScale, ground.z);
	std::vector<std::array<glm::vec3, 3>> hull;
	hull.reserve(modelTriangles.size());
	for (const auto& triangle : modelTriangles)
	{
		hull.push_back({origin + (triangle[0] * shape.finalScale), origin + (triangle[1] * shape.finalScale),
		                origin + (triangle[2] * shape.finalScale)});
	}
	return hull;
}

std::optional<shield::HullHit> shield::CrossHull(std::span<const std::array<glm::vec3, 3>> hull, glm::vec3 from, glm::vec3 to)
{
	constexpr float k_Facing = -1e-4f;
	const auto move = to - from;
	std::optional<HullHit> best;
	float bestT = 2.0f;
	for (const auto& [v0, v1, v2] : hull)
	{
		const auto cross = glm::cross(v1 - v0, v2 - v0);
		const float length = glm::length(cross);
		if (!(length > 0.0f))
		{
			continue;
		}
		const auto n = cross / length;
		const float d = glm::dot(n, move);
		if (!(d < k_Facing))
		{
			continue;
		}
		const float t = -glm::dot(from - v0, n) / d;
		if (t < 0.0f || t > 1.0f || t >= bestT)
		{
			continue;
		}
		const auto q = from + move * t;
		if (glm::dot(glm::cross(v1 - v0, q - v0), n) > 0.0f && glm::dot(glm::cross(v2 - v1, q - v1), n) > 0.0f &&
		    glm::dot(glm::cross(v0 - v2, q - v2), n) > 0.0f)
		{
			bestT = t;
			best = HullHit {.point = q, .normal = n};
		}
	}
	return best;
}

bool shield::IsUnder(glm::vec2 point, glm::vec2 centre, float radius, float margin)
{
	return glm::distance(point, centre) < radius - margin;
}

bool shield::NeedsShelter(float distance, float radius)
{
	return !(distance < radius - (radius * (1.0f - k_ShelterInside)));
}

shield::Shelter shield::ShelterAt(glm::vec2 centre, glm::vec2 villager, float radius, float turn, float u, float faceTurn)
{
	const auto away = villager - centre;
	const float side = std::atan2(away.y, away.x);
	const float angle = side + turn - k_ShelterTurnHalf;
	const float inner = k_ShelterInside * radius;
	const float distance = inner - (u * u * u * inner);
	const glm::vec2 point = centre + glm::vec2(std::cos(angle), std::sin(angle)) * distance;
	const float faceAngle = side + faceTurn - k_ShelterTurnHalf;
	const glm::vec2 face = centre + glm::vec2(std::cos(faceAngle), std::sin(faceAngle)) * (distance + 1.0f);
	return {.point = point, .faceTowards = face};
}

uint32_t shield::AmazedAnimation(uint32_t roll)
{
	if (roll == 0)
	{
		return k_IntoPointing;
	}
	return roll <= 2 ? k_LookAtHand : k_Stand;
}

uint8_t shield::VillagerPriority(uint8_t priority, bool hasTown, float protectionSignificance, uint32_t sinceAttacked,
                                 uint32_t interestedFor)
{
	if (!hasTown)
	{
		return priority;
	}
	if (protectionSignificance == 0.0f)
	{
		return 0;
	}
	return sinceAttacked <= interestedFor ? priority : uint8_t {0};
}

uint32_t shield::VillagerReactTurns(bool hasTown, uint32_t roll4, uint32_t roll50, uint32_t sinceAttacked,
                                    uint32_t interestedFor, uint32_t standardTurns)
{
	if (hasTown && roll4 != 0 && sinceAttacked + roll50 < interestedFor)
	{
		return magic::villager_reaction::k_Forever;
	}
	return standardTurns;
}

uint32_t shield::VillagerAgainTurns(bool hasTown, uint32_t sinceAttacked, uint32_t interestedFor, bool goalUnder,
                                    uint32_t standardTurns)
{
	if (hasTown && sinceAttacked > interestedFor)
	{
		return magic::villager_reaction::k_Forever;
	}
	return goalUnder ? standardTurns : 0;
}

glm::vec2 shield::LookOut(glm::vec2 centre, glm::vec2 villager, float turn)
{
	const auto away = villager - centre;
	const float angle = std::atan2(away.y, away.x) + turn - k_ShelterTurnHalf;
	return villager + glm::vec2(std::cos(angle), std::sin(angle));
}

float shield::ImpressiveMultiplier(Reaction reaction, bool townAttackedByShieldPlayer, bool physical)
{
	if (townAttackedByShieldPlayer &&
	    (reaction == Reaction::ReactToMagicShield || (!physical && reaction == Reaction::ReactToMagicShieldStruck)))
	{
		return 0.0f;
	}
	return !physical && reaction == Reaction::ReactToMagicShieldDestroyed ? k_DestroyedImpressiveness : 1.0f;
}
