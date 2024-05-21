/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraModel.h"

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/LandIsland.h"
#include "DefaultWorldCameraModel.h"
#include "Locator.h"

using namespace openblack;

CameraModel::~CameraModel() = default;

std::unique_ptr<CameraModel> CameraModel::CreateModel(CameraModel::Model model)
{
	switch (model)
	{
	case CameraModel::Model::DefaultWorld:
		return std::unique_ptr<CameraModel>(new DefaultWorldCameraModel());
	default:
		assert(false);
		return nullptr;
	}
}

CameraModel::FlightPath CameraModel::CharterFlight(glm::vec3 destinationOrigin, glm::vec3 destinationFocus,
                                                   glm::vec3 currentOrigin, float heightFactor)
{
	const auto delta = destinationOrigin - currentOrigin;
	auto halfWayPoint = currentOrigin + delta / 2.0f;
	const auto halfWayAltitude = Locator::terrainSystem::value().GetHeightAt(glm::xz(halfWayPoint));
	halfWayPoint.y += glm::max(glm::length(glm::xz(delta)) * heightFactor, halfWayAltitude);

	return {destinationOrigin, destinationFocus, std::make_optional<glm::vec3>(halfWayPoint)};
}
