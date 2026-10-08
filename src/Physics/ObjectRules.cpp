/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ObjectRules.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack::physics;
using namespace openblack::physics::objects;

TreeLanding objects::TreeLandingOf(bool putDownGently, bool forestMiracleTree, bool onLand, bool burning)
{
	if (putDownGently && forestMiracleTree)
	{
		return TreeLanding::Stays;
	}
	if (putDownGently && onLand && !burning)
	{
		return TreeLanding::Replanted;
	}
	return TreeLanding::Dies;
}

bool ForestSearch::Meet(const Thing& thing)
{
	if (thing.edgeDistance < k_ReplantNearEdge && thing.ofTown)
	{
		// Near a town the tree joins the town's forest if it has one, whatever trees stand nearer
		_nearTown = true;
		if (thing.townForest.has_value())
		{
			_best = 0.0f;
			_forest = thing.townForest;
		}
		return false;
	}
	if (thing.forest.has_value() && thing.edgeDistance < _best)
	{
		_best = thing.edgeDistance;
		_forest = thing.forest;
	}
	return true;
}

std::optional<float> objects::RockWear(float g, float height, bool struckByRock)
{
	if (!(g > k_RockWearG) || !(height > k_RockLeastHeight) || struckByRock)
	{
		return std::nullopt;
	}
	return (g - k_RockWearG) * k_RockWearPerG;
}

glm::mat3 objects::HeadingOnly(const glm::mat3& axes)
{
	glm::vec3 forward(axes[2].x, 0.0f, axes[2].z);
	const float length = glm::length(forward);
	forward = length > 0.0f ? forward / length : glm::vec3(0.0f, 0.0f, 1.0f);
	glm::mat3 upright;
	upright[1] = glm::vec3(0.0f, 1.0f, 0.0f);
	upright[2] = forward;
	upright[0] = glm::cross(upright[1], upright[2]);
	return upright;
}

bool objects::RockBreaksWhenTapped(float height)
{
	return height > k_RockLeastHeight;
}

float objects::RootsHeight(float startHeight, float restHeight, float seconds)
{
	const float falling = startHeight - k_RootsFall * seconds * seconds;
	return falling > restHeight ? falling : restHeight;
}

std::optional<uint8_t> objects::RootsAlpha(float seconds)
{
	if (!(seconds > k_RootsFadeStart))
	{
		return std::nullopt;
	}
	const float share = 1.0f - (seconds - k_RootsFadeStart) * k_RootsFadePerSecond;
	return static_cast<uint8_t>(static_cast<int>(std::clamp(share, 0.0f, 1.0f) * 255.0f));
}

bool objects::RootsGone(float seconds)
{
	return seconds > k_RootsLife;
}

Felling objects::FellingOf(float height, glm::vec2 fellerToTree)
{
	// The heading from the feller to the tree, none when they stand together
	const float heading =
	    glm::dot(fellerToTree, fellerToTree) > k_FellLeastSquaredDistance ? std::atan2(fellerToTree.x, -fellerToTree.y) : 0.0f;
	const float speed = k_FellSpeedPerHeight * height;
	return {
	    .velocity = {speed * std::sin(heading), 0.0f, -speed * std::cos(heading)},
	    .spin = {k_FellSpin * std::cos(heading), 0.0f, k_FellSpin * std::sin(heading)},
	};
}

bool objects::ArtefactWillImpress(float worth, bool ownTown)
{
	return worth > k_ArtefactImpressingWorth && !ownTown;
}
