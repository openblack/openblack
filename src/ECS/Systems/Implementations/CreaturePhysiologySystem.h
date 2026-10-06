/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <random>

#include "ECS/Systems/CreaturePhysiologySystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreaturePhysiologySystem final: public CreaturePhysiologySystemInterface
{
public:
	void ProcessTurn() override;
	void Update(float seconds) override;

	void Eat(entt::entity creature, float foodValue) override;
	void Drink(entt::entity creature) override;
	void Poo(entt::entity creature) override;
	void Puke(entt::entity creature) override;
	void WakeFromFaint(entt::entity creature) override;
	void FinishAction(entt::entity creature, std::string_view action) override;
	void ModifyStrength(entt::entity creature, float amount) override;

	void SetTimeScale(float scale) override;
	[[nodiscard]] float GetTimeScale() const override;
	void SetFaintingEnabled(bool enabled) override;
	[[nodiscard]] bool IsFaintingEnabled() const override;

private:
	float _timeScale {1.0f};
	/// Turns of the body owed from a fractional time scale
	float _owedTurns {0.0f};
	bool _fainting {true};
	std::mt19937 _random {std::random_device {}()};
};

} // namespace openblack::ecs::systems
