/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>

#include "3D/SkyDome.h"

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
	/// The sun: a square far out in the sky, and its texture
	static constexpr entt::hashed_string k_SunMeshId = entt::hashed_string("weather/sun");
	static constexpr entt::hashed_string k_SunTextureId = entt::hashed_string("raw/sun");
	/// The moon: a half sphere beside the camera, its texture and that texture's alpha
	static constexpr entt::hashed_string k_MoonMeshId = entt::hashed_string("weather/moon");
	static constexpr entt::hashed_string k_MoonTextureId = entt::hashed_string("raw/weather");
	static constexpr entt::hashed_string k_MoonAlphaTextureId = entt::hashed_string("raw/weathera");

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
	/// The pictures of the sky's dome, a layer for each alignment from evil to good and, in each, for each time of day
	/// from night to day
	[[nodiscard]] virtual graphics::Texture2D& GetTexture() const noexcept = 0;
	/// Once a frame: the rows of the dome to blend again for the sky as it is now
	[[nodiscard]] virtual sky_dome::FrameRows AdvanceDome() noexcept = 0;
	/// Jumps to an hour of script time
	virtual void SetTime(float time) noexcept = 0;
};

} // namespace openblack
