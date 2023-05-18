/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ReflectionCamera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace openblack;

glm::mat4 ReflectionCamera::GetViewMatrix() const
{
	auto mRotation = GetRotationMatrix();
	auto mView = mRotation * glm::translate(glm::mat4(1.0f), -_position);

	// M''camera = M_reflection * M_camera * M_flip
	glm::mat4 reflectionMatrix;
	ReflectMatrix(reflectionMatrix, _reflectionPlane);

	return mView * reflectionMatrix;
}

/*
               | 1-2Nx2   -2NxNy  -2NxNz  -2NxD |
M_reflection = |  -2NxNy 1-2Ny2   -2NyNz  -2NyD |
               |  -2NxNz  -2NyNz 1-2Nz2   -2NzD |
               |    0       0       0       1   |
*/
void ReflectionCamera::ReflectMatrix(glm::mat4& m, const glm::vec4& plane) const
{
	m[0][0] = (1.0f - 2.0f * plane[0] * plane[0]);
	m[1][0] = (-2.0f * plane[0] * plane[1]);
	m[2][0] = (-2.0f * plane[0] * plane[2]);
	m[3][0] = (-2.0f * plane[3] * plane[0]);

	m[0][1] = (-2.0f * plane[1] * plane[0]);
	m[1][1] = (1.0f - 2.0f * plane[1] * plane[1]);
	m[2][1] = (-2.0f * plane[1] * plane[2]);
	m[3][1] = (-2.0f * plane[3] * plane[1]);

	m[0][2] = (-2.0f * plane[2] * plane[0]);
	m[1][2] = (-2.0f * plane[2] * plane[1]);
	m[2][2] = (1.0f - 2.0f * plane[2] * plane[2]);
	m[3][2] = (-2.0f * plane[3] * plane[2]);

	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][3] = 1.0f;
}
