/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "CameraModel.h"
#include "Common/ZoomInterpolator.h"
#include "ECS/Components/Transform.h"

namespace openblack
{

class Camera
{
public:
	enum class Interpolation : uint8_t
	{
		Current,
		Start,
		Target,
	};
	enum class Projection : uint8_t
	{
		Normal,
		ReversedZ,
	};
	explicit Camera(glm::vec3 focus = glm::vec3(1000.0f, 0.0f, 1000.0f));
	virtual ~Camera();

	[[nodiscard]] float GetHorizontalFieldOfView() const;
	[[nodiscard]] virtual glm::mat4 GetViewMatrix(Interpolation interpolation) const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix() const;
	[[nodiscard]] const glm::mat4& GetProjectionMatrix(Projection projection) const;
	[[nodiscard]] glm::mat4 GetViewProjectionMatrix(Interpolation interpolation = Camera::Interpolation::Current) const;
	[[nodiscard]] glm::mat4 GetViewProjectionMatrix(Projection projection,
	                                                Interpolation interpolation = Camera::Interpolation::Current) const;

	[[nodiscard]] std::optional<ecs::components::Transform>
	RaycastMouseToLand(bool includeWater = true, Interpolation interpolation = Camera::Interpolation::Current) const;
	[[nodiscard]] std::optional<ecs::components::Transform>
	RaycastScreenCoordToLand(glm::vec2 screenCoord, bool includeWater,
	                         Interpolation interpolation = Camera::Interpolation::Current) const;

	[[nodiscard]] glm::vec3 GetOrigin(Interpolation interpolation = Interpolation::Current) const;
	[[nodiscard]] glm::vec3 GetOriginVelocity(Interpolation interpolation = Interpolation::Current) const;
	[[nodiscard]] glm::vec3 GetFocus(Interpolation interpolation = Interpolation::Current) const;
	[[nodiscard]] glm::vec3 GetFocusVelocity(Interpolation interpolation = Interpolation::Current) const;

	/// Get rotation as euler angles in radians
	[[nodiscard]] glm::vec3 GetRotation() const;

	Camera& SetOrigin(const glm::vec3& position);
	Camera& SetFocus(const glm::vec3& position);

	Camera& SetOriginInterpolator(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& m0, const glm::vec3& m1);
	Camera& SetFocusInterpolator(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& m0, const glm::vec3& m1);

	[[nodiscard]] std::chrono::microseconds GetInterpolatorTime() const { return _interpolatorTime; }
	[[nodiscard]] float GetInterpolatorT() const
	{
		const auto duration = GetInterpolatorDuration();
		return (duration == decltype(duration)::zero())
		           ? 1.0f
		           : static_cast<float>(GetInterpolatorTime().count()) / GetInterpolatorDuration().count();
	}
	[[nodiscard]] std::chrono::microseconds GetInterpolatorDuration() const { return _interpolatorDuration; }

	Camera& SetInterpolatorTime(std::chrono::microseconds t);
	Camera& AddInterpolatorTime(std::chrono::microseconds t) { return SetInterpolatorTime(t + GetInterpolatorTime()); }
	Camera& SetInterpolatorT(float t)
	{
		return SetInterpolatorTime(std::chrono::duration_cast<std::chrono::microseconds>(t * GetInterpolatorDuration()));
	}
	Camera& SetInterpolatorDuration(std::chrono::microseconds duration);

	Camera& SetProjectionMatrixPerspective(float xFov, float aspect, float nearClip, float farClip);
	Camera& SetProjectionMatrix(const glm::mat4& projection);

	[[nodiscard]] glm::vec3 GetForward() const;
	[[nodiscard]] glm::vec3 GetRight() const;
	[[nodiscard]] glm::vec3 GetUp() const;

	[[nodiscard]] std::unique_ptr<Camera> Reflect() const;

	void DeprojectScreenToWorld(glm::vec2 screenCoord, glm::vec3& outWorldOrigin, glm::vec3& outWorldDirection,
	                            Interpolation interpolation = Camera::Interpolation::Current) const;
	bool ProjectWorldToScreen(glm::vec3 worldPosition, glm::vec4 viewport, glm::vec3& outScreenPosition,
	                          Interpolation interpolation = Camera::Interpolation::Current) const;

	void Update(std::chrono::microseconds dt);
	void HandleActions(std::chrono::microseconds dt);

	[[nodiscard]] glm::mat4 GetRotationMatrix() const;
	[[nodiscard]] Projection GetCameraProjection() const;

	CameraModel& GetModel() { return *_model; }
	[[nodiscard]] const CameraModel& GetModel() const { return *_model; }

protected:
	ZoomInterpolator3f _originInterpolators;
	ZoomInterpolator3f _focusInterpolators;
	// As a value between 0 and _interpolatorDuration
	std::chrono::microseconds _interpolatorTime = std::chrono::microseconds::zero();
	std::chrono::microseconds _interpolatorDuration = std::chrono::microseconds::zero();
	float _xFov = 0.0f; // TODO(#707): This should be a zoomer for animations
	glm::mat4 _projectionMatrix = glm::mat4 {1.0f};
	glm::mat4 _projectionMatrixReversedZ = glm::mat4 {1.0f};
	std::unique_ptr<CameraModel> _model;
	Projection _cameraProjection = Projection::ReversedZ;
};

} // namespace openblack
