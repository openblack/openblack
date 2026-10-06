/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack
{

namespace graphics
{
class L3DMesh;
class Texture2D;
} // namespace graphics

class SkyInterface
{
public:
	/// Hours of the morning at which the sky reaches each state, mirrored around midday for the evening
	struct DayNightTimes
	{
		float nightFull;
		float duskStart;
		float duskEnd;
		float dayFull;
	};

	[[nodiscard]] virtual float GetCurrentSkyType() const noexcept = 0;
	[[nodiscard]] virtual graphics::L3DMesh& GetMesh() const noexcept = 0;
	[[nodiscard]] virtual graphics::Texture2D& GetTexture() const noexcept = 0;
	virtual void SetTime(float time) noexcept = 0;
};

} // namespace openblack
