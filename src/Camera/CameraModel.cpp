/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraModel.h"

#include "OldCameraModel.h"

using namespace openblack;

CameraModel::~CameraModel() = default;

std::unique_ptr<CameraModel> CameraModel::CreateModel(CameraModel::Model model)
{
	switch (model)
	{
	case CameraModel::Model::Old:
		return std::unique_ptr<CameraModel>(new OldCameraModel());
	default:
		assert(false);
		return nullptr;
	}
}
