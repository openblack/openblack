/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <deque>
#include <memory>
#include <optional>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/GestureSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::gestures
{
class GestureFile;
}

namespace openblack::ecs::systems
{

class GestureSystem final: public GestureSystemInterface
{
public:
	GestureSystem();
	~GestureSystem() override;

	// GestureEventsInterface
	[[nodiscard]] std::vector<GestureEvent> TakeEvents() override;
	void Inject(const GestureEvent& event) override;
	void Reset() override;

	// GestureSystemInterface
	void Update(const Frame& frame) override;
	void DrawPath(std::vector<glm::vec2> path, bool holdingAction) override;
	[[nodiscard]] bool IsDrawingPath() const override { return !_drawing.empty(); }
	[[nodiscard]] std::optional<glm::vec2> GetDrawingPoint() const override
	{
		return _drawing.empty() ? std::nullopt : std::optional(_drawing.front());
	}
	void ForgetPath() override;
	[[nodiscard]] std::span<const gestures::GestureTemplate> GetTemplates() const override;
	[[nodiscard]] const gesture::GestureRecorder& GetPath() const override { return _recorder; }
	[[nodiscard]] float GetScreenAspect() const override { return _screenAspect; }
	[[nodiscard]] const std::vector<gesture::Request>& GetRequests() const override { return _requests; }
	[[nodiscard]] std::optional<Recognised> GetLastRecognised() const override { return _lastRecognised; }
	[[nodiscard]] bool IsLeashPickerOpen() const override { return _picker.open; }
	[[nodiscard]] float GetCircleSecondsLeft() const override { return _circleSeconds; }
	[[nodiscard]] bool IsGesturing() const override { return _gesturing; }

private:
	/// The game's templates, loaded through the resource caches the first time they are wanted
	void LoadTemplates();
	/// What the hand holds and the player's creature, for the requests
	[[nodiscard]] gesture::HandContext ContextOf(const Frame& frame);
	/// Carries out a recognised gesture
	void Act(const gesture::Request& request, const gesture::Match& match, const Frame& frame);
	/// The recognised gesture's trail on the land, for the particles to show
	void LayTrail(GestureType gesture, const gesture::Match& match, const Frame& frame) const;

	std::vector<GestureEvent> _events;
	std::shared_ptr<const gestures::GestureFile> _templates;
	bool _templatesTried {false};
	gesture::GestureRecorder _recorder;
	/// Points of a path being drawn for the testbed, taken in place of the cursor
	std::deque<glm::vec2> _drawing;
	bool _drawingHoldsAction {false};
	float _sinceSample {0.0f};
	/// Where the camera's eye was last frame
	std::optional<glm::vec3> _lastCameraEye;
	/// The rest after a gesture, while nothing is recorded
	float _pause {0.0f};
	float _screenAspect {4.0f / 3.0f};
	std::vector<gesture::Request> _requests;
	std::optional<Recognised> _lastRecognised;
	uint32_t _recognisedCount {0};
	gesture::LeashPicker _picker;
	/// The circle drawn for the seed in the hand, while it is remembered
	float _circleSeconds {0.0f};
	bool _gesturing {false};
	entt::entity _circleSeed {entt::null};
};

} // namespace openblack::ecs::systems
