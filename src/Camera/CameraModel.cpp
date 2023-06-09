/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraModel.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/LandIslandInterface.h"
#include "Camera.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "Game.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;

CameraModel::CameraModel()
    : _dv(0.0f, 0.0f, 0.0f)
    , _dwv(0.0f, 0.0f, 0.0f)
    , _dsv(0.0f, 0.0f, 0.0f)
    , _ddv(0.0f, 0.0f, 0.0f)
    , _duv(0.0f, 0.0f, 0.0f)
    , _drv(0.0f, 0.0f, 0.0f)
    , _velocity(0.0f, 0.0f, 0.0f)
    , _hVelocity(0.0f, 0.0f, 0.0f)
    , _rotVelocity(0.0f, 0.0f, 0.0f)
    , _accelFactor(0.001f)
    , _movementSpeed(1.5f)
    , _maxMovementSpeed(0.005f)
    , _maxRotationSpeed(0.005f)
    , _lmouseIsDown(false)
    , _mmouseIsDown(false)
    , _mouseIsMoving(false)
    , _mouseFirstClick(0, 0)
    , _shiftHeld(false)
    , _handScreenVec(0)
    , _handDragMult(0.0f)
{
}

void CameraModel::FlyInit()
{
	// call to initialise a new flight. eventually may be refactored out to a separate file.
	_flyInProgress = false;
	_flyDist = 0.0f;
	_flyProgress = 1.0f;
	_flySpeed = 0.5f;
	_flyStartAngle = 10.0f;
	_flyEndAngle = 30.0f;
	_flyThreshold = 50.0f;
	_flyFromPos = glm::vec3(0.0f, 0.0f, 0.0f);
	_flyFromTan = glm::vec3(0.0f, 0.0f, 0.0f);
	_flyToPos = glm::vec3(0.0f, 0.0f, 0.0f);
	_flyToNorm = glm::vec3(0.0f, 0.0f, 0.0f);
	_flyToTan = glm::vec3(0.0f, 0.0f, 0.0f);
	_flyPrevPos = glm::vec3(0.0f, 0.0f, 0.0f);
}

void CameraModel::ResetVelocities()
{
	_dv = glm::vec3(0.0f, 0.0f, 0.0f);
	_drv = glm::vec3(0.0f, 0.0f, 0.0f);
	_dwv = glm::vec3(0.0f, 0.0f, 0.0f);
	_dsv = glm::vec3(0.0f, 0.0f, 0.0f);
	_ddv = glm::vec3(0.0f, 0.0f, 0.0f);
	_duv = glm::vec3(0.0f, 0.0f, 0.0f);
}

void CameraModel::HandleActions(const Camera& camera)
{
	const auto& actionSystem = Locator::gameActionSystem::value();

	_targetPosition = camera.GetPosition();
	_targetRotation = camera.GetRotation();

	if (actionSystem.Get(input::UnbindableActionMap::DOUBLE_CLICK))
	{
		const auto mousePosition = actionSystem.GetMousePosition();
		const auto dist = glm::distance(glm::vec2(mousePosition), glm::vec2(_mouseFirstClick));
		// fly to double-click location.
		if (dist < 10.0f)
		{
			if (auto hit = camera.RaycastMouseToLand())
			{
				// stop all current movements
				ResetVelocities();

				_flyToNorm = hit->rotation * glm::vec3(0.0f, 1.0f, 0.0f);
				auto normXZ = glm::normalize(_flyToNorm * glm::vec3(1.0f, 0.01f, 1.0f));
				_flyInProgress = true;
				_flyProgress = 0.0f;
				_flyFromPos = _targetPosition;
				_flyPrevPos = _flyFromPos;
				_flyDist = glm::length(hit->position - _flyFromPos);
				auto vecToCam = glm::normalize(_targetPosition - hit->position);
				_flyToPos = hit->position + (normXZ + vecToCam * 4.0f) / 5.0f * std::max(20.0f, _flyDist * 0.15f);
				_flyFromTan = glm::normalize(camera.GetForward() * glm::vec3(1.0f, 0.0f, 1.0f)) * _flyDist * 0.4f;
				_flyToTan =
				    glm::normalize(-(_flyToNorm * 9.0f + vecToCam) / 10.0f * glm::vec3(1.0f, 0.0f, 1.0f)) * _flyDist * 0.4f;
				if (_targetPosition.y <
				    _flyThreshold) // if the camera is low to the ground aim the path up before coming back down
				{
					_flyFromTan += glm::vec3(0.0f, 1.0f, 0.0f) * _flyDist * 0.4f;
					_flyToTan += glm::vec3(0.0f, -1.0f, 0.0f) * _flyDist * 0.4f;
				}
			}
		}
	}

	if (actionSystem.GetChanged(input::BindableActionMap::MOVE) && !actionSystem.Get(input::BindableActionMap::MOVE))
	{
		_lmouseIsDown = false;
		_mouseFirstClick = actionSystem.GetMousePosition();
	}
	else if (actionSystem.GetChanged(input::BindableActionMap::MOVE) && actionSystem.Get(input::BindableActionMap::MOVE))
	{
		if (!_lmouseIsDown)
		{
			ResetVelocities();
		}
		if (!_mmouseIsDown)
		{
			_lmouseIsDown = true;
		}
	}
	if (actionSystem.GetChanged(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON) &&
	    !actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON))
	{
		_mmouseIsDown = false;
	}
	else if (actionSystem.GetChanged(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON) &&
	         actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON))
	{
		_mmouseIsDown = true;
		_lmouseIsDown = false;
		if (!_mmouseIsDown)
		{
			ResetVelocities();
		}
	}

	if (actionSystem.GetAny(input::BindableActionMap::ZOOM_IN, input::BindableActionMap::ZOOM_OUT)) // scroll up or down
	{
		auto cameraPosition = _targetPosition;
		auto movementSpeed =
		    _movementSpeed * 4 * glm::smoothstep(0.1f, 1.0f, cameraPosition.y * 0.01f) * glm::log(cameraPosition.y + 1);
		_flyInProgress = false;
		const auto direction = actionSystem.Get(input::BindableActionMap::ZOOM_IN) ? 10.0f : -10.0f;
		auto dist = 9999.0f;
		const auto& dynamicsSystem = Locator::dynamicsSystem::value();
		if (auto hit = dynamicsSystem.RayCastClosestHit(cameraPosition, camera.GetForward(), 1e10f))
		{
			dist = glm::length(hit->first.position - cameraPosition);
		}
		auto amount = (((movementSpeed * direction * _maxMovementSpeed) - _velocity.z) * _accelFactor);
		if (actionSystem.Get(input::BindableActionMap::ZOOM_IN)) // scrolling in
		{
			if (dist > 40.0f) // if the cam is far from the ground
			{
				_velocity.z += amount;
			}
			else // if the cam is just over the ground
			{
				if (_targetRotation.x < glm::radians(60.0f)) // rotation greater than 60 degrees
				{
					_rotVelocity.x += ((direction * 4.0f * _maxRotationSpeed) + _rotVelocity.x) * _accelFactor;
				}
			}
		}
		else // scrolling out
		{
			if (dist <= 40.0f && _targetRotation.x > glm::radians(50.0f))
			{
				_rotVelocity.x += ((direction * 4.0f * _maxRotationSpeed) + _rotVelocity.x) * _accelFactor;
			}
			else
			{
				_velocity.z += amount;
			}
		}
	}

	if (actionSystem.GetMouseDelta() != glm::zero<glm::ivec2>())
	{
		if (actionSystem.Get(input::BindableActionMap::ROTATE_AROUND_MOUSE_ON))
		{
			const auto delta = actionSystem.GetMouseDelta();
			const auto handPositions = actionSystem.GetHandPositions();
			if (_shiftHeld) // Holding down the middle mouse button and shift enables FPV camera rotation.
			{
				glm::vec3 rot = _targetRotation;
				rot.y += delta.x * glm::radians(0.1f);
				rot.x += delta.y * glm::radians(0.1f);
				_targetRotation = rot;
			}
			// Holding down the middle mouse button without shift enables hand orbit camera rotation.
			else if (handPositions[0].has_value() || handPositions[1].has_value())
			{
				auto handPos = handPositions[0].value_or(handPositions[1].value_or(glm::zero<glm::vec3>()));

				auto handDist = glm::length2(handPos - _targetPosition);
				if (handDist > 250000.0f) // if hand is more than 500 away (500^2)
				{
					handPos = _targetPosition + camera.GetForward() * 500.0f; // orbit around a point 500 away from cam
				}
				const auto screenSize = Locator::windowing::value().GetSize();
				const float yaw = delta.x * (glm::two_pi<float>() / screenSize.x);
				float pitch = delta.y * (glm::pi<float>() / screenSize.y);

				// limit orbit cam by cam rotation in x
				if (pitch > 0.0f)
				{
					pitch *= glm::smoothstep(0.f, 0.1f, -_targetRotation.x + glm::radians(60.0f));
				}

				auto cameraRotation = _targetRotation;
				cameraRotation.x += pitch;
				cameraRotation.y += yaw;
				_targetRotation = cameraRotation;

				glm::mat4 rotationMatrixX(1.0f);
				rotationMatrixX = glm::rotate(rotationMatrixX, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				auto cameraPosition =
				    static_cast<glm::vec3>(rotationMatrixX * glm::vec4(_targetPosition - handPos, 0.0f)) + handPos;

				glm::mat4 rotationMatrixY(1.0f);
				rotationMatrixY = glm::rotate(rotationMatrixY, pitch, camera.GetRight());
				cameraPosition = (rotationMatrixY * glm::vec4(cameraPosition - handPos, 0.0f)) + glm::vec4(handPos, 0.0f);
				cameraPosition.y = (cameraPosition.y < 15.0f) ? 15.0f : cameraPosition.y;
				_targetPosition = cameraPosition;
			}
		}
		else if (actionSystem.Get(input::BindableActionMap::MOVE))
		{
			_mouseIsMoving = true;
		}
	}

	auto movementSpeed = _movementSpeed * std::max(_targetPosition.y * 0.01f, 0.0f) + 1.0f;

	// ignore all repeated keys
	if (actionSystem.GetRepeatAll(input::BindableActionMap::ROTATE_ON, input::BindableActionMap::MOVE_LEFT,
	                              input::BindableActionMap::MOVE_RIGHT, input::BindableActionMap::MOVE_FORWARDS,
	                              input::BindableActionMap::MOVE_BACKWARDS, input::BindableActionMap::ROTATE_LEFT,
	                              input::BindableActionMap::ROTATE_RIGHT, input::BindableActionMap::TILT_UP,
	                              input::BindableActionMap::TILT_DOWN))
	{
		return;
	}

	// stop Fly if any movement keys are pressed down
	if (actionSystem.GetAny(input::BindableActionMap::ROTATE_ON, input::BindableActionMap::MOVE_LEFT,
	                        input::BindableActionMap::MOVE_RIGHT, input::BindableActionMap::MOVE_FORWARDS,
	                        input::BindableActionMap::MOVE_BACKWARDS, input::BindableActionMap::ROTATE_LEFT,
	                        input::BindableActionMap::ROTATE_RIGHT, input::BindableActionMap::TILT_UP,
	                        input::BindableActionMap::TILT_DOWN))
	{
		_flyInProgress = false;
	}

	if (_mmouseIsDown || _lmouseIsDown)
	{
		ResetVelocities();
	}
	else
	{
		_shiftHeld = actionSystem.Get(input::BindableActionMap::ROTATE_ON);
		if (actionSystem.GetChanged(input::BindableActionMap::MOVE_FORWARDS))
		{
			if (actionSystem.Get(input::BindableActionMap::MOVE_FORWARDS))
			{
				glm::vec3 temp = camera.GetForward() * glm::vec3(1.f, 0.f, 1.f);
				temp = glm::normalize(temp) * static_cast<glm::mat3>(camera.GetRotationMatrix()) * movementSpeed;
				_dv += temp;
				_dwv = temp;
			}
			else
			{
				_dv -= _dwv;
				_dwv = glm::vec3(0.0f, 0.0f, 0.0f);
			}
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::MOVE_BACKWARDS))
		{
			if (actionSystem.Get(input::BindableActionMap::MOVE_BACKWARDS))
			{
				glm::vec3 temp = -camera.GetForward() * glm::vec3(1.f, 0.f, 1.f);
				temp = glm::normalize(temp) * static_cast<glm::mat3>(camera.GetRotationMatrix()) * movementSpeed;
				_dv += temp;
				_dsv = temp;
			}
			else
			{
				_dv -= _dsv;
				_dsv = glm::vec3(0.0f, 0.0f, 0.0f);
			}
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::MOVE_LEFT))
		{
			_dv.x += actionSystem.Get(input::BindableActionMap::MOVE_LEFT) ? -movementSpeed : -_dv.x;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::MOVE_RIGHT))
		{
			_dv.x += actionSystem.Get(input::BindableActionMap::MOVE_RIGHT) ? movementSpeed : -_dv.x;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::ROTATE_LEFT))
		{
			_drv.y += actionSystem.Get(input::BindableActionMap::ROTATE_LEFT) ? _movementSpeed : -_drv.y;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::ROTATE_RIGHT))
		{
			_drv.y += actionSystem.Get(input::BindableActionMap::ROTATE_RIGHT) ? -_movementSpeed : -_drv.y;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::TILT_UP))
		{
			_drv.x += actionSystem.Get(input::BindableActionMap::TILT_UP) ? _movementSpeed : -_drv.x;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::TILT_DOWN))
		{
			_drv.x += actionSystem.Get(input::BindableActionMap::TILT_DOWN) ? -_movementSpeed : -_drv.x;
		}
		else if (actionSystem.GetChanged(input::BindableActionMap::ZOOM_TO_TEMPLE))
		{
			if (actionSystem.Get(input::BindableActionMap::ZOOM_TO_TEMPLE))
			{
				movementSpeed = _movementSpeed * 4 * glm::smoothstep(0.1f, 1.0f, _targetPosition.y * 0.01f) *
				                glm::log(_targetPosition.y + 1);
				glm::vec3 temp = glm::vec3(0.f, 1.f, 0.f);
				const glm::mat3 mRotation = glm::transpose(camera.GetRotationMatrix());
				if (_shiftHeld)
				{
					temp = glm::normalize(-temp) * mRotation * movementSpeed;
					_dv += temp;
					_ddv = temp;
					_flyInProgress = false;
				}
				else
				{
					temp = glm::normalize(temp) * mRotation * movementSpeed;
					_dv += temp;
					_duv = temp;
					_flyInProgress = false;
				}
			}
			else
			{
				_dv -= _ddv;
				_ddv = glm::vec3(0.0f, 0.0f, 0.0f);
				_dv -= _duv;
				_duv = glm::vec3(0.0f, 0.0f, 0.0f);
			}
		}
	}
}

void CameraModel::Update(std::chrono::microseconds dt, const Camera& camera)
{
	auto airResistance = .92f; // reduced to make more floaty
	auto fdt = static_cast<float>(dt.count());
	auto rotation = static_cast<glm::mat3>(camera.GetRotationMatrix());

	// deal with hand pulling camera around
	float worldHandDist = 0.0f;
	const auto windowSize = Locator::windowing::value().GetSize();
	const auto& actionSystem = Locator::gameActionSystem::value();
	const auto handPositions = actionSystem.GetHandPositions();
	if (_lmouseIsDown && (handPositions[0].has_value() || handPositions[1].has_value())) // drag camera using hand
	{
		// get hand transform and project to screen coordinates
		auto handPos = handPositions[0].value_or(handPositions[1].value_or(glm::zero<glm::vec3>()));
		const glm::vec3 handOffset(0, 1.5f, 0);
		glm::vec3 handToScreen;
		const auto viewport = glm::vec4(0, 0, windowSize.x, windowSize.y);
		auto hit = camera.RaycastMouseToLand();
		if (hit)
		{
			handPos -= handOffset * glm::transpose(hit->rotation);
		}
		if (camera.ProjectWorldToScreen(handPos, viewport, handToScreen) && hit)
		{
			// calculate distance between hand and mouse in screen coordinates
			const auto mousePosition = actionSystem.GetMousePosition();
			auto handScreenCoordinates = glm::ivec2(handToScreen);
			handScreenCoordinates.y = windowSize.y - handScreenCoordinates.y;
			_handScreenVec = static_cast<glm::ivec2>(mousePosition) - handScreenCoordinates;
			_handDragMult = glm::length(glm::vec2(_handScreenVec));
			worldHandDist = glm::length(hit->position - handPos);
			_handDragMult /= windowSize.y;
		}
		// still on screen but did not hit land
		else if (!hit)
		{
			_handDragMult -= 0.002f; // slow down movement
		}
		else // if hand is off-screen, culled or behind camera.
		{
			_hVelocity = glm::vec3(0.f);
		}
	}
	else
	{
		_handDragMult = (_handDragMult > 0.0f) ? _handDragMult * 0.96f
		                                       : _handDragMult; // if let go of mouse button slow down hand movement.
		_hVelocity = glm::vec3(0.f);
	}

	if (_handDragMult > 0.0f)
	{
		const auto cameraPosition = camera.GetPosition();
		const auto cameraForward = camera.GetForward();
		auto momentum = cameraPosition.y / 300;
		auto forward =
		    glm::normalize(cameraForward * glm::vec3(1.f, 0.f, 1.f)) * static_cast<float>(_handScreenVec.y * momentum);
		auto right = camera.GetRight() * -static_cast<float>(_handScreenVec.x * momentum);
		auto futurePosition = cameraPosition + forward + right;
		auto logPosY = glm::log(cameraPosition.y + 1.0f);
		glm::vec3 vecTo = futurePosition - cameraPosition;
		vecTo = glm::min(glm::normalize(vecTo) * logPosY * logPosY, glm::vec3(5.0f));
		auto mRotation = static_cast<glm::mat3>(glm::transpose(camera.GetRotationMatrix()));
		_hVelocity += vecTo * mRotation * 0.00005f;
		if (cameraForward.y > 0.0f) // camera is pointing upwards
		{
			if (_handScreenVec.y > 0.0f) // if the move direction is also up
			{
				_hVelocity.y += 0.0005f; // move camera up a little
			}
			else
			{
				_hVelocity.y -= 0.0005f; // move camera down a little
			}
		}
	}

	if (worldHandDist > 2000.0f)
	{
		// speed up movement for really distant hand pulls.
		_handDragMult += 0.001f;
	}
	else if (worldHandDist < 2000.0f && worldHandDist > 0.0f && _handDragMult >= 0.1f)
	{
		// slow movement for less distant hand pulls.
		_handDragMult *= 0.98f;
	}
	else if (worldHandDist < 0.0f)
	{
		_handDragMult = 0.0f;
	}

	auto cameraPosition = camera.GetPosition();
	if (_flyInProgress)
	{
		cameraPosition =
		    glm::hermite(_flyFromPos, _flyFromTan, _flyToPos, _flyToTan, glm::smoothstep(0.0f, 1.0f, _flyProgress));

		// Check if there are obstacles in the way, if there are fly over them
		const auto& dynamicsSystem = Locator::dynamicsSystem::value();
		if (auto obst =
		        dynamicsSystem.RayCastClosestHit(cameraPosition - glm::vec3(0.0f, 20.0f, 0.0f),
		                                         glm::normalize((_flyToPos - glm::vec3(0.0f, 20.0f, 0.0f)) - cameraPosition),
		                                         glm::length(_flyToPos - cameraPosition) + 10.0f))
		{
			auto closest = obst->first;
			auto dist = glm::length(_flyToPos - closest.position);
			if (dist > 60.0f)
			{
				_flyFromTan += glm::vec3(0.0f, 1.0f, 0.0f) * _flyDist * 0.006f;
				_flyToTan += glm::vec3(0.0f, -1.0f, 0.0f) * _flyDist * 0.006f;
			}
		}

		if (_flyPrevPos != cameraPosition && _flyProgress > 0.0f)
		{
			auto tangentY = glm::normalize((cameraPosition - _flyPrevPos) * glm::vec3(1.0f, 0.0f, 1.0f));
			auto currDirY = glm::normalize(camera.GetForward() * glm::vec3(1.0f, 0.0f, 1.0f));
			auto rotY =
			    atan2f(glm::dot(glm::cross(tangentY, currDirY), glm::vec3(0.0f, 1.0f, 1.0f)), glm::dot(currDirY, tangentY));
			_drv.y = rotY * 5.0f * ((_flyProgress * _flyProgress) * -1.0f + 1.0f);
		}
		if (_flyProgress >= 1.0f)
		{
			_flyInProgress = false;
			_drv = glm::vec3(0.0f);
		}
		else if (_flyProgress >= 0.5f && glm::dot(_flyToNorm, glm::vec3(0.0f, 1.0f, 0.0f)) > 0.8f) // rotate cam down
		{
			auto angleDown = -glm::radians(_flyEndAngle) + camera.GetRotation().x;
			_drv.x = angleDown * 2.0f * (((_flyProgress * 2 - 2) * (_flyProgress * 2 - 2)) * -1.0f + 1.0f);
		}
		else if (_flyProgress <= 0.5f && cameraPosition.y < _flyThreshold) // rotate cam up
		{
			auto angleUp = glm::radians(_flyStartAngle) + camera.GetRotation().x;
			_drv.x = angleUp * 0.5f * (((_flyProgress * 2 - 1) * (_flyProgress * 2 - 1)) * -1.0f + 1.0f);
		}
		_flyPrevPos = cameraPosition;
		_flyProgress += _flySpeed * 0.000001f * fdt;
	}
	else if (!_flyInProgress && _flyProgress < 1.0f) // player aborted a Fly early
	{
		_flyProgress = 1.0f;
		_drv = glm::vec3(0.0f);
	}
	else
	{
		_velocity += (((_dv * _maxMovementSpeed) - _velocity) * _accelFactor);
		_hVelocity *= _handDragMult;
		cameraPosition += rotation * (_velocity + _hVelocity) * fdt;
	}
	const auto& land = Locator::terrainSystem::value();

	cameraPosition = camera.GetPosition();
	auto height = land.GetHeightAt(glm::xz(cameraPosition) + 5.0f);
	// stop the camera from going below ground level.
	_targetPosition =
	    glm::vec3(cameraPosition.x, (cameraPosition.y < height + 13.0f) ? height + 13.0f : cameraPosition.y, cameraPosition.z);
	_rotVelocity += -(((_drv * _maxRotationSpeed) - _rotVelocity) * _accelFactor);

	auto orientation = static_cast<glm::quat>(camera.GetRotationMatrix());
	const auto angularVelocity = glm::radians(_rotVelocity * fdt);

	// Convert angular velocity to a quaternion
	const auto velocityQuat = glm::length2(angularVelocity) > 0.0f
	                              ? glm::angleAxis(glm::length(angularVelocity), glm::normalize(angularVelocity))
	                              : glm::quat();

	// Update the orientation
	orientation *= velocityQuat;

	// Extract the yaw, pitch, and roll angles from the rotation matrix
	float yaw, pitch, roll;

	// Use the GLM function `extractEulerAngleYXZ` to get the Euler angles
	glm::extractEulerAngleZYX(glm::mat4(orientation), roll, yaw, pitch);
	_targetRotation = glm::vec3(pitch, yaw, roll);

	_velocity *= airResistance;
	_rotVelocity *= airResistance;
	_mouseIsMoving = false;
}

glm::vec3 CameraModel::GetTargetPosition() const
{
	return _targetPosition;
}

glm::vec3 CameraModel::GetTargetFocus() const
{
	return _targetPosition + glm::quat(_targetRotation) * glm::vec3(0.0f, 0.0f, 1.0f);
}
