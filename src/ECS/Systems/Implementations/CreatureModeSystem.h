/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include "ECS/Systems/CreatureModeSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack
{
class CameraModel;
class CreatureCameraModel;
} // namespace openblack

namespace openblack::ecs::systems
{

class CreatureModeSystem final: public CreatureModeSystemInterface
{
public:
	CreatureModeSystem();
	~CreatureModeSystem() override;
	CreatureModeSystem(const CreatureModeSystem&) = delete;
	CreatureModeSystem& operator=(const CreatureModeSystem&) = delete;

	void Update(std::chrono::microseconds dt, const Frame& frame) override;
	bool Press(const creature_mode::Press& press) override;
	void PressCreatureKey() override;
	bool Enter(entt::entity creature) override;
	void Leave() override;
	[[nodiscard]] bool IsActive() const override { return _creature.has_value(); }
	[[nodiscard]] std::optional<entt::entity> GetCreature() const override { return _creature; }
	[[nodiscard]] std::optional<entt::entity> PlayersCreature() const override;
	[[nodiscard]] std::optional<creature_follow::View> GetView() const override;
	void HoldKeys(int across, int along, bool shift, bool ctrl, float seconds) override;
	void ClearView() override;

private:
	[[nodiscard]] bool OwnsCamera() const;
	/// Hands the player's camera back, if the camera is with this mode's
	void ReleaseCamera();
	/// Whether the creature can still be followed: it is there, and nothing else needs the camera
	[[nodiscard]] bool StillValid() const;

	std::optional<entt::entity> _creature;
	/// The camera's model while locked on, and the player's own, given back on leaving
	CreatureCameraModel* _model {nullptr};
	std::unique_ptr<CameraModel> _playerModel;
	/// Leaving was asked for while another had the camera; it is given back once this mode has it again
	bool _leaving {false};
	/// Whether the hand gripped the land last frame, so that only a fresh grip counts
	bool _wasGripping {false};
	creature_mode::DoubleClicks _doubleClicks;
	/// The keys the testbed holds down
	struct HeldKeys
	{
		int across {0};
		int along {0};
		bool shift {false};
		bool ctrl {false};
		float seconds {0.0f};
	};
	HeldKeys _held;
};

} // namespace openblack::ecs::systems
