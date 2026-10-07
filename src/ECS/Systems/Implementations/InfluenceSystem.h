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
#include <vector>

#include <entt/entity/fwd.hpp>

#include "ECS/Systems/InfluenceSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class InfluenceSystem final: public InfluenceSystemInterface
{
public:
	void Reset() override;
	void ProcessTurn(uint32_t turn) override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;

	using InfluenceSystemInterface::PlayerInfluence;
	[[nodiscard]] float PlayerInfluence(PlayerNames player, const map_coords::MapCoords& position) const override;

	[[nodiscard]] std::span<const influence::Circle> GetCircles() const override { return _circles; }
	[[nodiscard]] bool IsBorderShown(PlayerNames player) const override;
	[[nodiscard]] glm::vec2 GetScrollOffset() const override { return influence::ScrollOffset(_scrollClock); }
	[[nodiscard]] std::span<const influence::Ripple> GetRipples() const override { return _ripples; }

private:
	static constexpr size_t k_Players = 8;

	void ProcessTowns();
	void ProcessCitadels();
	void DrawBorders();
	/// A reach that has moved since its border was drawn wants the border drawn again
	void NoteReach(float reach, float drawn);
	/// Whether the hand crossed a border shown since the last frame, sending out a ripple where it did
	bool CrossBorders(const glm::vec3& hand);

	std::vector<influence::Circle> _circles;
	std::array<bool, k_Players> _borderShown {};
	bool _bordersDirty {true};
	int32_t _scrollClock {0};
	float _scrollRemainder {0.0f};
	/// Newest first
	std::vector<influence::Ripple> _ripples;
	/// Whether the hand was inside each player's border at the last frame, once it has been somewhere, and where it was
	std::array<bool, k_Players> _handWasInside {};
	bool _handSeen {false};
	glm::vec3 _handBefore {0.0f};
};

} // namespace openblack::ecs::systems
