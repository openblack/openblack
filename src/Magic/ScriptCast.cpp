/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScriptCast.h"

#include "3D/MapCoords.h"

using namespace openblack;

magic::ScriptCast magic::MakeScriptCast(float initialChants, glm::vec3 target, glm::vec3 from, float radius, float duration,
                                        float curl)
{
	const auto quantise = [](glm::vec3 point) {
		return glm::vec3(map_coords::Quantise(point.x), point.y, map_coords::Quantise(point.z));
	};
	const auto at = quantise(target);
	const auto start = quantise(from);
	return {.point = at,
	        .cast = {.magnitude = radius, .chants = initialChants, .duration = duration, .maxObjectsToCreate = -1},
	        .info = {.handPosition = start,
	                 .cameraForward = at - start,
	                 .direction = glm::vec3(0.0f),
	                 .power = 1.0f,
	                 .enabled = true,
	                 .spin = k_ScriptCurlToSpin * curl}};
}
