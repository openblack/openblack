/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/entity/fwd.hpp>

namespace openblack::editor
{
struct EditorContext;

/// The picked thing's sections: its place, turn and size to change, what it is made of, and, for a creature, every
/// section of the creature tools (its looks, body, mind and learning, movement and commands, hands, leash, fight and
/// sounds), and for a villager, its life and states
class InspectorPanel
{
public:
	void Draw(EditorContext& context) noexcept;

private:
	void DrawHeader(EditorContext& context, entt::entity entity) noexcept;
	void DrawTransform(EditorContext& context, entt::entity entity) noexcept;
	void DrawVillager(EditorContext& context, entt::entity entity) noexcept;
	void DrawComponents(EditorContext& context, entt::entity entity) noexcept;
};

} // namespace openblack::editor
