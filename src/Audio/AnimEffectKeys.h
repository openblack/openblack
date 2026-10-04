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

namespace openblack::audio
{

// Values of the keys of Black & White's animation effects (see AnimEffectTable), named as the bank's editor text names
// them after the game's SoundSize.h, SoundAlignment.h, SoundObject.h, SoundSurface.h and SoundAction.h. Only the
// values the game plays so far are here. 0 leaves a key unset.

enum class SoundSize : int32_t
{
	None = 0,
	Large = 1,
	Medium = 2,
	Small = 3,
};

enum class SoundObject : int32_t
{
	None = 0,
	Tree = 20,
};

enum class SoundSurface : int32_t
{
	None = 0,
	Tree = 10,
};

enum class SoundAction : int32_t
{
	None = 0,
	Tree = 70,
	Collide = 75,
};

/// What an animation effect is for, in the order of the keys of the banks
struct AnimEffectKeys
{
	SoundSize size {SoundSize::None};
	int32_t alignment {0};
	SoundObject object {SoundObject::None};
	SoundSurface surface {SoundSurface::None};
	SoundAction action {SoundAction::None};

	[[nodiscard]] constexpr std::array<int32_t, 5> ToArray() const noexcept
	{
		return {static_cast<int32_t>(size), alignment, static_cast<int32_t>(object), static_cast<int32_t>(surface),
		        static_cast<int32_t>(action)};
	}

	constexpr bool operator==(const AnimEffectKeys&) const noexcept = default;
};

} // namespace openblack::audio
