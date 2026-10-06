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

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreaturePhysiology.h"
#include "Creature/CreatureStatusPanel.h"

/// The rules of Creature Mode, in which the camera locks onto a creature and follows it while the panel shows how
/// damaged, hungry and tired it is. Everything here is pure, so the rules are tested on their own:
/// - C locks onto the player's own creature, and pressed again while on it gives the camera back
/// - a double click on a creature, anyone's, locks onto that one
/// - the cursor keys alone, dragging the land, the temple, the editor or a script's cinema bars end it
/// - a creature passes out when any of the three reaches 100%, and is carried to its pen to come round
namespace openblack::creature_mode
{

/// A creature of size 1 stands about this tall, whatever its species' mesh
constexpr float k_HeightPerSize = 15.0f;
/// How tall a creature of a size stands, which the camera keeps its middle in view by
[[nodiscard]] float CreatureHeight(float size);

/// What C does: lock onto the player's creature, or let go of it when the camera is already on it. Locked onto
/// another god's creature, C moves over to the player's own.
enum class KeyAction : uint8_t
{
	None,
	Enter,
	Leave,
};
[[nodiscard]] KeyAction OnCreatureKey(std::optional<entt::entity> following, std::optional<entt::entity> yours);

/// The Windows double click the game takes: a second press within half a second, within a small box of the first
constexpr uint32_t k_DoubleClickMilliseconds = 500;
constexpr float k_DoubleClickSlop = 2.0f;

/// A press of the left button: when, where on the screen, and the creature under the hand if any
struct Press
{
	uint32_t milliseconds {0};
	glm::vec2 screen {0.0f};
	std::optional<entt::entity> creature;
};

/// Tells double clicks on creatures from the presses of the left button: the second of two quick presses on the same
/// creature, without the mouse moving off the first, is a double click on it
class DoubleClicks
{
public:
	/// The creature double clicked on by this press, if it makes one
	[[nodiscard]] std::optional<entt::entity> OnPress(const Press& press);

private:
	std::optional<Press> _last;
};

/// Whether a creature's status at 100% has it pass out, as the panel shows it: 100% damage is no life left, 100%
/// hunger no energy left and 100% tiredness exhaustion. It agrees with the body's own fainting.
[[nodiscard]] std::optional<creature_physiology::Faint> PassOutFrom(const creature_panel::Values& values);

/// Where a creature that passed out is carried to come round, its pen: where it was given a home, or else by its
/// player's temple, or for a player without a temple (as on the testbed) the spot given, the middle of the land
[[nodiscard]] glm::vec3 PenOf(std::optional<glm::vec3> home, std::optional<glm::vec3> temple, glm::vec3 noPen);

} // namespace openblack::creature_mode
