/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <memory>
#include <optional>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "3D/CameraPath.h"
#include "Camera/CameraZoomer.h"
#include "ECS/Systems/CameraPathSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

enum class CameraPathState : uint8_t
{
	PLAYING,
	PAUSED,
	STOPPED
};

class CameraPathSystem final: public CameraPathSystemInterface
{
public:
	void Start(entt::id_type id) override;
	void Stop() override;
	void Play() override { _state = CameraPathState::PLAYING; }
	void Pause() override { _state = CameraPathState::PAUSED; }
	void Update(const std::chrono::microseconds& dt) override;
	bool IsPathing() override
	{
		return _placed.has_value() || _state == CameraPathState::PLAYING || _state == CameraPathState::PAUSED;
	}
	bool IsPaused() override { return _state == CameraPathState::PAUSED; }
	void FollowPlaced(std::unique_ptr<CameraPath> path, const glm::mat4& placement, float pauseSeconds, float speedUp,
	                  float animationMs, entt::entity spell) override;
	[[nodiscard]] bool HoldsCamera() const override { return _placed.has_value(); }
	void HandlePlayerControl(const PlayerControl& control) override;

private:
	/// A path placed in the world that the camera follows
	struct Placed
	{
		std::unique_ptr<CameraPath> path;
		glm::mat4 placement;
		float pauseSeconds;
		float speedUp;
		float animationMs;
		/// The miracle whose effect placed it, which it goes with
		entt::entity spell {entt::null};
		/// Seconds since it began
		float seconds {0.0f};
		/// The camera's origin and focus, each coordinate gliding
		std::array<camera::Zoomer, 3> origin;
		std::array<camera::Zoomer, 3> focus;
	};
	void UpdatePlaced(float seconds);
	std::optional<Placed> _placed;

	entt::resource<CameraPath> _path;
	/// How far along the path the camera is
	std::chrono::microseconds _elapsed {0};
	CameraPathState _state {CameraPathState::STOPPED};
};
} // namespace openblack::ecs::systems
