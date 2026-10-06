/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CreatureCaveSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureCaveSystem final: public CreatureCaveSystemInterface
{
public:
	void Update() override;
	void SetInterface(const gui::GameInterface* interface) override { _interface = interface; }
	[[nodiscard]] const gui::GameInterface* GetInterface() const override { return _interface; }
	void Open() override;
	void Close() override;
	[[nodiscard]] bool IsOpen() const override { return _screen.open; }
	[[nodiscard]] bool InTemple() const override;
	bool Escape() override;
	[[nodiscard]] creature_cave::Screen& GetScreen() override { return _screen; }
	[[nodiscard]] std::optional<entt::entity> GetCreature() const override;
	[[nodiscard]] std::optional<creature_cave::Snapshot> Snapshot() const override;
	bool ApplyTattoo(uint8_t site, uint8_t design, glm::u8vec3 colour) override;
	bool RemoveTattoo(uint8_t site) override;

private:
	const gui::GameInterface* _interface {nullptr};
	creature_cave::Screen _screen;
};

} // namespace openblack::ecs::systems
