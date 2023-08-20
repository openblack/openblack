/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>

#include "3D/LandIsland.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "Game.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "ReflectionXZCamera.h"

using namespace openblack;

Camera::Camera()
    : _model(CameraModel::CreateModel(CameraModel::Model::Old))
{
}

float Camera::GetHorizontalFieldOfView() const
{
	return _xFov;
}

glm::mat4 Camera::GetRotationMatrix() const
{
	return static_cast<glm::mat3>(glm::transpose(GetViewMatrix()));
}

glm::mat4 Camera::GetViewMatrix() const
{
	// Invert the camera's rotation (transposed) and position (negated) to get the view matrix.
	return glm::lookAt(_position, _focus, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

std::optional<ecs::components::Transform> Camera::RaycastMouseToLand() const
{
	// get the hit by raycasting to the land down via the mouse
	const auto mousePosition = Locator::gameActionSystem::value().GetMousePosition();
	const auto screenSize = Locator::windowing::value().GetSize();
	return RaycastScreenCoordToLand(static_cast<glm::vec2>(mousePosition) / static_cast<glm::vec2>(screenSize));
}

std::optional<ecs::components::Transform> Camera::RaycastScreenCoordToLand(glm::vec2 screenCoord) const
{
	// get the hit by raycasting to the land down via the pixel coordinate
	ecs::components::Transform intersectionTransform;
	float intersectDistance = 0.0f;
	glm::vec3 rayOrigin;
	glm::vec3 rayDirection;
	DeprojectScreenToWorld(screenCoord, rayOrigin, rayDirection);
	const auto& dynamicsSystem = Locator::dynamicsSystem::value();
	if (auto hit = dynamicsSystem.RayCastClosestHit(rayOrigin, rayDirection, 1e10f))
	{
		intersectionTransform = hit->first;
		return std::make_optional(intersectionTransform);
	}
	if (glm::intersectRayPlane(rayOrigin, rayDirection, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
	                           intersectDistance))
	{
		intersectionTransform.position = rayOrigin + rayDirection * intersectDistance;
		intersectionTransform.rotation = glm::mat3(1.0f);
		return std::make_optional(intersectionTransform);
	}
	return std::nullopt;
}

Camera& Camera::SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip)
{
	_xFov = glm::radians(xFov);
	const float yFov = (glm::atan(glm::tan(_xFov / 2.0f)) / aspect) * 2.0f;
	const float h = 1.0f / glm::tan(yFov * 0.5f);
	const float w = h / aspect;
	const float a = nearClip / (farClip - nearClip);
	const float b = (nearClip * farClip) / (farClip - nearClip);
	_projectionMatrix = glm::mat4x4(w, 0.f, 0.f, 0.f, 0.f, h, 0.f, 0.f, 0.f, 0.f, a, 1.f, 0.f, 0.f, b, 0.f);

	return *this;
}

glm::vec3 Camera::GetForward() const
{
	// Forward is +1 in openblack but is -1 in OpenGL
	return static_cast<glm::mat3>(GetRotationMatrix()) * glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::vec3 Camera::GetRight() const
{
	return static_cast<glm::mat3>(GetRotationMatrix()) * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 Camera::GetUp() const
{
	return static_cast<glm::mat3>(GetRotationMatrix()) * glm::vec3(0.0f, 1.0f, 0.0f);
}

std::unique_ptr<Camera> Camera::Reflect() const
{
	// TODO(bwrsandman): The copy to reflection camera has way too much of Camera including model which is useless
	//                   This also touches on other cameras such as the citadel camera which use a different kind of model
	auto reflectionCamera = std::make_unique<ReflectionXZCamera>();
	(*reflectionCamera).SetPosition(_position).SetFocus(_focus).SetProjectionMatrix(_projectionMatrix);
	return reflectionCamera;
}

void Camera::DeprojectScreenToWorld(glm::vec2 screenCoord, glm::vec3& outWorldOrigin, glm::vec3& outWorldDirection) const
{
	const float screenSpaceX = (screenCoord.x - 0.5f) * 2.0f;
	const float screenSpaceY = ((1.0f - screenCoord.y) - 0.5f) * 2.0f;

	// The start of the ray trace is defined to be at mousex,mousey,1 in
	// projection space (z=0 is near, z=1 is far - this gives us better
	// precision) To get the direction of the ray trace we need to use any z
	// between the near and the far plane, so let's use (mousex, mousey, 0.5)
	const glm::vec4 rayStartProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.0f, 1.0f);
	const glm::vec4 rayEndProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.5f, 1.0f);

	// Calculate our inverse view projection matrix
	auto inverseViewProj = glm::inverse(GetViewProjectionMatrix());

	// Get our homogeneous coordinates for our start and end ray positions
	const glm::vec4 hgRayStartWorldSpace = inverseViewProj * rayStartProjectionSpace;
	const glm::vec4 hgRayEndWorldSpace = inverseViewProj * rayEndProjectionSpace;

	glm::vec3 rayStartWorldSpace(hgRayStartWorldSpace.x, hgRayStartWorldSpace.y, hgRayStartWorldSpace.z);
	glm::vec3 rayEndWorldSpace(hgRayEndWorldSpace.x, hgRayEndWorldSpace.y, hgRayEndWorldSpace.z);

	// divide vectors by W to undo any projection and get the 3-space coord
	if (hgRayStartWorldSpace.w != 0.0f)
	{
		rayStartWorldSpace /= hgRayStartWorldSpace.w;
	}

	if (hgRayEndWorldSpace.w != 0.0f)
	{
		rayEndWorldSpace /= hgRayEndWorldSpace.w;
	}

	const glm::vec3 rayDirWorldSpace = glm::normalize(rayEndWorldSpace - rayStartWorldSpace);

	// finally, store the results in the outputs
	outWorldOrigin = rayStartWorldSpace;
	outWorldDirection = rayDirWorldSpace;
}

bool Camera::ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition) const
{
	outScreenPosition = glm::project(worldPosition, GetViewMatrix(), GetProjectionMatrix(), viewport);
	if (outScreenPosition.x < viewport.x || outScreenPosition.y < viewport.y || outScreenPosition.x > viewport.z ||
	    outScreenPosition.y > viewport.w)
	{
		return false; // Outside viewport bounds
	}
	if (outScreenPosition.z > 1.0f)
	{
		return false; // Behind Camera
	}
	if (outScreenPosition.z < 0.0f)
	{
		return false; // Clipped
	}
	outScreenPosition.y = viewport.w - (outScreenPosition.y - viewport.y) + viewport.y;
	return true;
}

void Camera::Update(std::chrono::microseconds dt)
{
	const auto updateInfo = _model->Update(dt, *this);

	if (updateInfo)
	{
		SetPosition(updateInfo->origin);
		SetFocus(updateInfo->focus);
	}
}

void Camera::HandleActions(std::chrono::microseconds dt)
{
	_model->HandleActions(dt);
}

const glm::mat4& Camera::GetProjectionMatrix() const
{
	return _projectionMatrix;
}

glm::vec3 Camera::GetPosition() const
{
	return _position;
}

glm::vec3 Camera::GetFocus() const
{
	return _focus;
}


glm::vec3 Camera::GetVelocity() const
{
	// TODO
	return {0.0f, 0.0f, 0.0f};
}

glm::vec3 Camera::GetRotation() const
{
	// Extract the yaw, pitch, and roll angles from the rotation matrix
	float yaw;
	float pitch;
	float roll;

	// Use the GLM function `extractEulerAngleYXZ` to get the Euler angles
	glm::extractEulerAngleZYX(GetRotationMatrix(), roll, yaw, pitch);
	// Fix angles to make sure roll is 0, it tends to flip to 180 and -180
	if (roll > glm::radians(90.0f) || roll < glm::radians(-90.0f))
	{
		pitch -= glm::radians(180.0f) * glm::sign(pitch);
		yaw = glm::radians(180.0f) - yaw;
	}
	return {pitch, yaw, 0.0f};
}

Camera& Camera::SetPosition(const glm::vec3& position)
{
	_position = position;

	return *this;
}

Camera& Camera::SetFocus(const glm::vec3& position)
{
	_focus = position;

	return *this;
}

Camera& Camera::SetProjectionMatrix(const glm::mat4& projection)
{
	_projectionMatrix = projection;

	return *this;
}
