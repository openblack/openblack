/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <entt/core/fwd.hpp>

namespace openblack
{

/// The main room's five buttons that choose what the map in its pool shows: the temples, the creatures, miracles being
/// cast, the players' influence and the challenges.
///
/// Each is a pair of submeshes of the main room's mesh, one pressed in and one out, of which only the one for whether
/// its things are shown is drawn. A press on the one drawn takes the mouse, and letting go over it turns the button
/// over with a click.
class TempleToggles
{
public:
	enum class Display : uint8_t
	{
		Temples,
		Creatures,
		MiracleActivity,
		Influence,
		Challenges,
	};
	static constexpr size_t k_Count = 5;

	using PlaySound = std::function<void(entt::id_type)>;
	explicit TempleToggles(PlaySound playSound);

	/// Finds the buttons among the main room's submeshes, by their names
	void Find(std::span<const std::string> subMeshNames);

	/// A press on a button takes the mouse from the camera, and letting go over the same button
	/// turns it over. True while a button has the press.
	bool Hold(bool pressed, std::optional<uint32_t> hoveredSubMesh);
	[[nodiscard]] bool IsHeld() const { return _held.has_value(); }

	/// Whether the map shows a kind of thing. All are shown to start with.
	[[nodiscard]] bool IsShown(Display display) const { return _shown.at(static_cast<size_t>(display)); }
	void SetShown(Display display, bool shown) { _shown.at(static_cast<size_t>(display)) = shown; }

	/// A button drawn, as the tooltips see it: its submesh and its tooltip, of the 170 from text 0xE73
	struct Control
	{
		uint32_t subMesh;
		uint32_t toolTip;
	};
	/// The buttons drawn, one of each pair
	[[nodiscard]] std::vector<Control> GetControls() const;
	/// The submeshes of the buttons not drawn
	[[nodiscard]] std::vector<uint32_t> GetHidden() const;
	/// Whether a submesh of the main room's mesh is a button drawn, a control the cursor makes glow
	[[nodiscard]] bool IsControl(uint32_t subMesh) const;

private:
	/// The submeshes of a button, pressed in while its things are shown and out while they aren't
	struct Button
	{
		std::optional<uint32_t> checked;
		std::optional<uint32_t> unchecked;
	};
	/// The submesh drawn of a button
	[[nodiscard]] std::optional<uint32_t> Drawn(size_t button) const;

	PlaySound _playSound;
	std::array<Button, k_Count> _buttons {};
	std::array<bool, k_Count> _shown {true, true, true, true, true};
	std::optional<size_t> _held;
	bool _wasPressed {false};
};

} // namespace openblack
