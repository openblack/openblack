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

class DayNightClock;

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

	/// The sky of the visual time: 0 at night, 1 at dusk and 2 by day, between them as it turns
	[[nodiscard]] virtual float GetCurrentSkyType() const noexcept = 0;
	/// The visual time, between 0 and 24 in hours
	[[nodiscard]] virtual float GetTime() const noexcept = 0;
	/// The hours of the visual time the sky turns at
	[[nodiscard]] virtual DayNightTimes GetDayNightTimes() const noexcept = 0;
	/// The clock of day and night the sky follows
	[[nodiscard]] virtual DayNightClock& GetClock() noexcept = 0;
	[[nodiscard]] virtual const DayNightClock& GetClock() const noexcept = 0;
	[[nodiscard]] virtual graphics::L3DMesh& GetMesh() const noexcept = 0;
	[[nodiscard]] virtual graphics::Texture2D& GetTexture() const noexcept = 0;
	/// Jumps to an hour of script time
	virtual void SetTime(float time) noexcept = 0;
};

} // namespace openblack
