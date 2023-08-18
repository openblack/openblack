/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraModel.h"

#include "DefaultWorldCameraModel.h"
#include "OldCameraModel.h"

using namespace openblack;

CameraModel::~CameraModel() = default;

std::unique_ptr<CameraModel> CameraModel::CreateModel(CameraModel::Model model, glm::vec3 origin, glm::vec3 focus)
{
	switch (model)
	{
	case CameraModel::Model::DefaultWorld:
		// TODO: Will the model still work if we don't set original origin and focus here?
		return std::unique_ptr<CameraModel>(new DefaultWorldCameraModel(origin, focus));
	case CameraModel::Model::Old:
		return std::unique_ptr<CameraModel>(new OldCameraModel());
	default:
		assert(false);
		return nullptr;
	}
}
