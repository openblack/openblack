/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <chrono>
#include <memory>
#include <optional>

#include <glm/vec3.hpp>

#include "CameraDrag.h"

namespace openblack
{

class Camera;

class CameraModel
{
public:
	enum class Model : uint8_t
	{
		DefaultWorld,
		Old,
	};

	struct CameraInterpolationUpdateInfo
	{
		glm::vec3 origin;
		glm::vec3 focus;
		std::chrono::microseconds duration;
	};

	struct FlightPath
	{
		glm::vec3 origin;
		glm::vec3 focus;
		std::optional<glm::vec3> midpoint;
	};

	/// A projection a model looks through instead of the configured one
	struct Lens
	{
		/// In degrees
		float horizontalFieldOfView;
		float nearClip;
	};

	/// What the camera does with the mouse, for the hand to show
	struct HandCues
	{
		/// The camera hints, as camera_drag::tricon
		uint32_t tricons {0};
		/// The land is being dragged, and what the drag has turned into once decided
		bool dragging {false};
		std::optional<camera_drag::DragMode> dragMode;
	};

	static std::unique_ptr<CameraModel> CreateModel(Model model);

	static FlightPath CharterFlight(glm::vec3 origin, glm::vec3 focus, glm::vec3 currentOrigin, float heightFactor);

	virtual ~CameraModel();

	virtual std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) = 0;
	virtual void HandleActions(std::chrono::microseconds dt) = 0;
	virtual void SetFlight(glm::vec3 origin, glm::vec3 focus) = 0;
	[[nodiscard]] virtual glm::vec3 GetTargetOrigin() const = 0;
	[[nodiscard]] virtual glm::vec3 GetTargetFocus() const = 0;
	[[nodiscard]] virtual std::chrono::seconds GetIdleTime() const = 0;
	[[nodiscard]] virtual std::optional<Lens> GetLens() const { return std::nullopt; }
	/// What the camera does with the mouse, which the hand shows: the hints the cursor's place offers, and what a
	/// drag of the land has turned into
	[[nodiscard]] virtual HandCues GetHandCues() const { return {}; }
};

} // namespace openblack
