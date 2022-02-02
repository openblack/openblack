/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Camera.h"
#include "3D/LandIsland.h"
#include "ECS/Registry.h"
#include "Game.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

using namespace openblack;

Camera::Camera(glm::vec3 position, glm::vec3 rotation)
    : _position(position)
    , _rotation(glm::radians(rotation))
    , _dv(0.0f, 0.0f, 0.0f)
    , _dwv(0.0f, 0.0f, 0.0f)
    , _dsv(0.0f, 0.0f, 0.0f)
    , _ddv(0.0f, 0.0f, 0.0f)
    , _duv(0.0f, 0.0f, 0.0f)
    , _drv(0.0f, 0.0f, 0.0f)
    , _viewCenterLand(_position)
    , _projectionMatrix(1.0f)
    , _velocity(0.0f, 0.0f, 0.0f)
    , _rotVelocity(0.0f, 0.0f, 0.0f)
    , _groundHeight(0.0f)
    , _groundHeightStart(0.0f)
    , _accelFactor(0.001f)
    , _movementSpeed(4.0f)
    , _maxMovementSpeed(0.005f)
    , _maxRotationSpeed(0.005f)
{
}

glm::mat4 Camera::GetRotationMatrix() const
{
	return glm::eulerAngleZXY(_rotation.z, _rotation.x, _rotation.y);
}

glm::mat4 Camera::GetViewMatrix() const
{
	return GetRotationMatrix() * glm::translate(glm::mat4(1.0f), -_position);
}

glm::mat4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

glm::vec3& Camera::GetViewCenterLand()
{
	// get the _viewCenterLand by raycasting to the land down the camera ray
	float intersectDistance = 0.0f;
	glm::vec3 viewVec = GetForward();
	if (glm::intersectRayPlane(_position, viewVec, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), intersectDistance))
	{
		_viewCenterLand = _position + viewVec * intersectDistance;
		_viewCenterLand.y = Game::instance()->GetLandIsland().GetHeightAt(glm::vec2(_viewCenterLand.x, _viewCenterLand.z));
	}
	else if (_viewCenterLand == glm::vec3(0.0f))
	{
		_viewCenterLand = _position + viewVec * 100.f;
	}
	return _viewCenterLand;
}

void Camera::SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip)
{
	float yFov = (glm::atan(glm::tan(glm::radians(xFov) / 2.0f)) / aspect) * 2.0f;
	_projectionMatrix = glm::perspective(yFov, aspect, nearClip, farClip);
}

glm::vec3 Camera::GetForward() const
{
	// Forward is +1 in openblack but is -1 in OpenGL
	glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
	return mRotation * glm::vec3(0, 0, 1);
}

glm::vec3 Camera::GetRight() const
{
	glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
	return mRotation * glm::vec3(1, 0, 0);
}

glm::vec3 Camera::GetUp() const
{
	glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
	return mRotation * glm::vec3(0, 1, 0);
}

std::unique_ptr<Camera> Camera::Reflect(const glm::vec4& relectionPlane) const
{
	auto reflectionCamera = std::make_unique<ReflectionCamera>(_position, glm::degrees(_rotation), relectionPlane);
	reflectionCamera->SetProjectionMatrix(_projectionMatrix);

	return reflectionCamera;
}

void Camera::DeprojectScreenToWorld(const glm::ivec2 screenPosition, const glm::ivec2 screenSize, glm::vec3& out_worldOrigin,
                                    glm::vec3& out_worldDirection)
{
	const float normalizedX = (float)screenPosition.x / (float)screenSize.x;
	const float normalizedY = (float)screenPosition.y / (float)screenSize.y;

	const float screenSpaceX = (normalizedX - 0.5f) * 2.0f;
	const float screenSpaceY = ((1.0f - normalizedY) - 0.5f) * 2.0f;

	// The start of the ray trace is defined to be at mousex,mousey,1 in
	// projection space (z=0 is near, z=1 is far - this gives us better
	// precision) To get the direction of the ray trace we need to use any z
	// between the near and the far plane, so let's use (mousex, mousey, 0.5)
	const glm::vec4 rayStartProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.0f, 1.0f);
	const glm::vec4 rayEndProjectionSpace = glm::vec4(screenSpaceX, screenSpaceY, 0.5f, 1.0f);

	// Calculate our inverse view projection matrix
	glm::mat4 inverseViewProj = glm::inverse(GetViewProjectionMatrix());

	// Get our homogeneous coordinates for our start and end ray positions
	const glm::vec4 hgRayStartWorldSpace = inverseViewProj * rayStartProjectionSpace;
	const glm::vec4 hgRayEndWorldSpace = inverseViewProj * rayEndProjectionSpace;

	glm::vec3 rayStartWorldSpace(hgRayStartWorldSpace.x, hgRayStartWorldSpace.y, hgRayStartWorldSpace.z);
	glm::vec3 rayEndWorldSpace(hgRayEndWorldSpace.x, hgRayEndWorldSpace.y, hgRayEndWorldSpace.z);

	// divide vectors by W to undo any projection and get the 3-space coord
	if (hgRayStartWorldSpace.w != 0.0f)
		rayStartWorldSpace /= hgRayStartWorldSpace.w;

	if (hgRayEndWorldSpace.w != 0.0f)
		rayEndWorldSpace /= hgRayEndWorldSpace.w;

	const glm::vec3 rayDirWorldSpace = glm::normalize(rayEndWorldSpace - rayStartWorldSpace);

	// finally, store the results in the outputs
	out_worldOrigin = rayStartWorldSpace;
	out_worldDirection = rayDirWorldSpace;
}

bool Camera::ProjectWorldToScreen(const glm::vec3 worldPosition, const glm::vec4 viewport, glm::vec3& out_screenPosition) const
{
	out_screenPosition = glm::project(worldPosition, GetViewMatrix(), GetProjectionMatrix(), viewport);
	if (out_screenPosition.x < viewport.x || out_screenPosition.y < viewport.y || out_screenPosition.x > viewport.z ||
	    out_screenPosition.y > viewport.w)
	{
		return false;
	}
	if (out_screenPosition.z > 1.0f)
	{
		// Behind Camera
		return false;
	}

	if (out_screenPosition.z < 0.0f)
	{
		// Clipped
		return false;
	}

	return true;
}

void Camera::ProcessSDLEvent(const SDL_Event& e)
{
	switch (e.type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		handleKeyboardInput(e);
		break;
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEWHEEL:
		handleMouseInput(e);
		break;
	}
}

void Camera::handleKeyboardInput(const SDL_Event& e)
{
	// ignore all repeated keys
	if (e.key.repeat > 0)
		return;

	auto movementSpeed = _movementSpeed * std::max(_position.y * 0.01f, 0.0f) + 1.0f;

	switch (e.key.keysym.scancode)
	{
	case SDL_SCANCODE_W:
		if (e.type == SDL_KEYDOWN)
		{
			glm::vec3 temp = GetForward() * glm::vec3(1.f, 0.f, 1.f);
			glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
			temp = glm::normalize(temp) * mRotation * movementSpeed;
			_dv += temp;
			_dwv = temp;
		}
		else
		{
			_dv -= _dwv;
			_dwv = glm::vec3(0.0f, 0.0f, 0.0f);
		}
		break;
	case SDL_SCANCODE_S:
		if (e.type == SDL_KEYDOWN)
		{
			glm::vec3 temp = -GetForward() * glm::vec3(1.f, 0.f, 1.f);
			glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
			temp = glm::normalize(temp) * mRotation * movementSpeed;
			_dv += temp;
			_dsv = temp;
		}
		else
		{
			_dv -= _dsv;
			_dsv = glm::vec3(0.0f, 0.0f, 0.0f);
		}
		break;
	case SDL_SCANCODE_A:
		_dv.x += (e.type == SDL_KEYDOWN) ? -movementSpeed : -_dv.x;
		break;
	case SDL_SCANCODE_D:
		_dv.x += (e.type == SDL_KEYDOWN) ? movementSpeed : -_dv.x;
		break;
	case SDL_SCANCODE_LCTRL:
		if (e.type == SDL_KEYDOWN)
		{
			glm::vec3 temp = -glm::vec3(0.f, 1.f, 0.f);
			glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
			temp = glm::normalize(temp) * mRotation * movementSpeed;
			_dv += temp;
			_ddv = temp;
		}
		else
		{
			_dv -= _ddv;
			_ddv = glm::vec3(0.0f, 0.0f, 0.0f);
		}
		break;
	case SDL_SCANCODE_SPACE:
		if (e.type == SDL_KEYDOWN)
		{
			glm::vec3 temp = glm::vec3(0.f, 1.f, 0.f);
			glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
			temp = glm::normalize(temp) * mRotation * movementSpeed;
			_dv += temp;
			_duv = temp;
		}
		else
		{
			_dv -= _duv;
			_duv = glm::vec3(0.0f, 0.0f, 0.0f);
		}
		break;
	case SDL_SCANCODE_Q:
		_drv.y += (e.type == SDL_KEYDOWN) ? _movementSpeed : -_drv.y;
		break;
	case SDL_SCANCODE_E:
		_drv.y += (e.type == SDL_KEYDOWN) ? -_movementSpeed : -_drv.y;
		break;
	case SDL_SCANCODE_R:
		_drv.x += (e.type == SDL_KEYDOWN) ? _movementSpeed : -_drv.x;
		break;
	case SDL_SCANCODE_V:
		_drv.x += (e.type == SDL_KEYDOWN) ? -_movementSpeed : -_drv.x;
		break;
	}
}
void Camera::handleMouseInput(const SDL_Event& e)
{
	const auto& land = Game::instance()->GetLandIsland();
	// Holding down the middle mouse button enables free look.
	if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		int width, height;
		Game::instance()->GetWindow()->GetSize(width, height);
		float yaw = e.motion.xrel * (glm::two_pi<float>() / width);
		float pitch = e.motion.yrel * (glm::pi<float>() / height);

		glm::mat4x4 rotationMatrixX(1.0f);
		rotationMatrixX = glm::rotate(rotationMatrixX, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		_position = (rotationMatrixX * glm::vec4(_position - _viewCenterLand, 0.0f)) + glm::vec4(_viewCenterLand, 0.0f);

		glm::mat4x4 rotationMatrixY(1.0f);
		rotationMatrixY = glm::rotate(rotationMatrixY, pitch, GetRight());
		_position = (rotationMatrixY * glm::vec4(_position - _viewCenterLand, 0.0f)) + glm::vec4(_viewCenterLand, 0.0f);

		_rotation.x -= pitch;
		_rotation.y -= yaw;
	}
	else if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		auto momentum = _position.y / 300;
		auto forward = GetForward() * glm::vec3(1.f, 0.f, 1.f) * static_cast<float>(e.motion.yrel * momentum);
		auto right = GetRight() * -static_cast<float>(e.motion.xrel * momentum);
		auto futurePosition = _position + forward + right;
		auto height = land.GetHeightAt(glm::vec2(futurePosition.x + 5, futurePosition.z + 5)) + _groundHeightStart;
		auto maxHandVel = std::max(log(_position.y) * log(_position.y), 0.0f) + 1.0f;
		futurePosition.y = std::max(height, _position.y);
		glm::vec3 vecTo = futurePosition - _position;
		vecTo.y = 0;

		if (glm::length(vecTo) > maxHandVel)
		{
			vecTo = glm::normalize(vecTo) * maxHandVel;
		}
		glm::mat3 mRotation = glm::transpose(GetRotationMatrix());
		_dv += vecTo * mRotation * 0.1f;
	}
	else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		_dv = glm::vec3(0.0f);
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		_groundHeightStart = land.GetHeightAt(glm::vec2(_position.x, _position.z));
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		GetViewCenterLand();
	}
	if (e.type == SDL_MOUSEWHEEL)
	{
		auto movementSpeed = _movementSpeed * std::max(log(_position.y), 0.0f) + 1.0f;
		if (e.wheel.y != 0) // scroll up or down
		{
			_velocity.z +=
			    (((movementSpeed * e.wheel.y * abs(e.wheel.y) * 5 * _maxMovementSpeed) - _velocity.z) * _accelFactor);
		}
		if (e.wheel.x != 0) // mouse wheel left or right
		{
			_rotation.z += (((movementSpeed * e.wheel.x * _maxMovementSpeed) - _velocity.z) * _accelFactor);
		}
	}
}

void Camera::Update(std::chrono::microseconds dt)
{
	auto airResistance = .92f; // reduced to make more floaty
	float fdt = float(dt.count());
	const auto& land = Game::instance()->GetLandIsland();
	glm::mat3 rotation = glm::transpose(GetViewMatrix());
	_velocity += (((_dv * _maxMovementSpeed) - _velocity) * _accelFactor);
	_position += rotation * _velocity * fdt;
	auto height = land.GetHeightAt(glm::vec2(_position.x + 5, _position.z + 5));
	_position.y =
	    (_position.y < height + 13.0f) ? height + 13.0f : _position.y; // stop the camera from going below ground level.

	_rotVelocity += (((_drv * _maxRotationSpeed) - _rotVelocity) * _accelFactor);
	glm::vec3 rot = GetRotation();
	rot += _rotVelocity * fdt;
	SetRotation(rot);

	_velocity *= airResistance;
	_rotVelocity *= airResistance;
}

glm::mat4 ReflectionCamera::GetViewMatrix() const
{
	glm::mat4 mRotation = GetRotationMatrix();
	glm::mat4 mView = mRotation * glm::translate(glm::mat4(1.0f), -_position);

	// M''camera = Mreflection * Mcamera * Mflip
	glm::mat4x4 reflectionMatrix;
	reflectMatrix(reflectionMatrix, _reflectionPlane);

	return mView * reflectionMatrix;
}

/*
              | 1-2Nx2   -2NxNy  -2NxNz  -2NxD |
Mreflection = |  -2NxNy 1-2Ny2   -2NyNz  -2NyD |
              |  -2NxNz  -2NyNz 1-2Nz2   -2NzD |
              |    0       0       0       1   |
*/
void ReflectionCamera::reflectMatrix(glm::mat4x4& m, const glm::vec4& plane) const
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
