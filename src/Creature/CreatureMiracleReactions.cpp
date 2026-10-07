/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMiracleReactions.h"

using namespace openblack::creature_mind;

namespace
{
Step PointAt(glm::vec2 point)
{
	return {.kind = Step::Kind::Object,
	        .order = {.kind = ObjectOrder::Kind::PointAt, .point = point},
	        .face = openblack::creature_face::Cue::Amazed};
}

Step TurnToFace(glm::vec2 point)
{
	return {.kind = Step::Kind::Move,
	        .movement = {.kind = Movement::Kind::TurnToFace, .point = point},
	        .face = openblack::creature_face::Cue::Amazed};
}
} // namespace

std::vector<Step> openblack::creature_mind::RunAwayFromMiracle(glm::vec2 point, float height, const Random& random)
{
	std::vector<Step> agenda;
	if (random(2) == 0)
	{
		agenda.push_back({.kind = Step::Kind::Action, .animation = k_FrightenedAnimation, .face = creature_face::Cue::Amazed});
	}
	agenda.push_back(
	    {.kind = Step::Kind::Move,
	     .movement = {
	         .kind = Movement::Kind::FleeFrom, .point = point, .run = true, .maxDistance = k_MiracleApproachHeights * height}});
	return agenda;
}

std::vector<Step> openblack::creature_mind::ExamineMiracle(glm::vec2 point, float height, const Random& random)
{
	std::vector<Step> agenda;
	const bool first = random(2) == 0;
	if (first)
	{
		agenda.push_back(TurnToFace(point));
		agenda.push_back(PointAt(point));
	}
	agenda.push_back({.kind = Step::Kind::Move,
	                  .movement = {.kind = Movement::Kind::ToPoint,
	                               .point = point,
	                               .minDistance = 0.0f,
	                               .maxDistance = k_MiracleApproachHeights * height}});
	agenda.push_back(TurnToFace(point));
	if (first)
	{
		agenda.push_back({.kind = Step::Kind::Wait, .seconds = k_PuzzledSeconds, .face = creature_face::Cue::Puzzled});
	}
	else
	{
		agenda.push_back(PointAt(point));
	}
	return agenda;
}
