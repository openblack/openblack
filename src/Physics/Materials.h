/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>

#include <entt/core/hashed_string.hpp>

namespace openblack::physconst
{
struct PhysicsConstantsFile;
}

namespace openblack::physics
{

/// The rows of the physics materials table, one per kind of thing that moves or is hit
enum class MaterialRow : uint8_t
{
	DefaultUnmovable,
	DefaultMovable,
	Football,
	Rock,
	Pot,
	/// The food offering
	Hay,
	Tree,
	Villager,
	Animal,
	OneOffSpell,
	PhysicalShield,
	Fragment,
	Poo,
	Vortex,
	ToyBall,
	ToyDie,
	ToyCuddly,
	MovableScaffold,
	Fence,
	Skittle,
	BowlingBall,
	Champignon,
	MagicMushroom,
	Toadstool,

	_Count
};

/// The materials' id in the resources
inline constexpr entt::hashed_string k_MaterialsId = entt::hashed_string("physics/materials");

inline constexpr std::size_t k_MaterialRows = static_cast<std::size_t>(MaterialRow::_Count);

/// How a material behaves in the physics
struct Material
{
	/// Relative to water: lighter floats, heavier sinks
	float density {0.0f};
	/// Contact stiffness per unit of mass
	float springK {0.0f};
	/// Contact damping per unit of mass
	float dampK {0.0f};
	float friction {0.0f};
	/// The share of a body's spin it keeps each second
	float spinKeptPerSecond {0.0f};
	/// Air drag
	float drag {0.0f};

	bool operator==(const Material&) const = default;
};

/// The material of every row. Each value read from the file is clamped to its range; rows past the ones the file declares
/// copy the first; without a file every row is zero, as the game has no values of its own to fall back on.
class MaterialTable
{
public:
	MaterialTable() = default;
	explicit MaterialTable(const std::optional<physconst::PhysicsConstantsFile>& file);

	[[nodiscard]] const Material& operator[](MaterialRow row) const { return _rows.at(static_cast<std::size_t>(row)); }

	/// The ranges each column is clamped to when read
	static constexpr Material k_Minimum {
	    .density = 0.05f, .springK = 0.0f, .dampK = 0.0f, .friction = 0.0f, .spinKeptPerSecond = 0.2f, .drag = 0.0f};
	static constexpr Material k_Maximum {
	    .density = 3.0f, .springK = 240.0f, .dampK = 10.0f, .friction = 3.0f, .spinKeptPerSecond = 1.0f, .drag = 4.0f};

	/// A value clamped as the file's values are
	[[nodiscard]] static Material Clamp(const Material& material);

private:
	std::array<Material, k_MaterialRows> _rows {};
};

} // namespace openblack::physics
