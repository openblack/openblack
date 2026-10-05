/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <memory>

#include <glm/fwd.hpp>

#include "3D/DayNightClock.h"
#include "3D/SkyInterface.h"
#include "Graphics/RenderPass.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{

namespace graphics
{
class L3DMesh;
class ShaderProgram;
class Texture2D;
} // namespace graphics

class Sky final: public SkyInterface
{
public:
	Sky() noexcept;
	~Sky() noexcept;

	void SetTime(float time) noexcept override;
	[[nodiscard]] float GetCurrentSkyType() const noexcept override;
	[[nodiscard]] float GetTime() const noexcept override { return _clock.GetVisualTime(); }
	[[nodiscard]] DayNightTimes GetDayNightTimes() const noexcept override
	{
		const auto& times = _clock.GetVisualTimes();
		return {.nightFull = times[0], .duskStart = times[1], .duskEnd = times[2], .dayFull = times[3]};
	}
	[[nodiscard]] DayNightClock& GetClock() noexcept override { return _clock; }
	[[nodiscard]] const DayNightClock& GetClock() const noexcept override { return _clock; }
	[[nodiscard]] graphics::L3DMesh& GetMesh() const noexcept override { return *_mesh; }
	[[nodiscard]] graphics::Texture2D& GetTexture() const noexcept override { return *_texture; }

private:
	static constexpr std::array<std::string_view, 3> k_Alignments = {
	    "evil",
	    "Ntrl",
	    "good",
	};
	static constexpr std::array<std::string_view, 3> k_Times = {
	    "night",
	    "dusk",
	    "day",
	};
	static constexpr std::array<uint16_t, 3> k_TextureResolution = {
	    256,
	    256,
	    static_cast<uint16_t>(k_Alignments.size() * k_Times.size()),
	};

	std::unique_ptr<graphics::L3DMesh> _mesh;
	std::unique_ptr<graphics::Texture2D> _texture; // TODO(bwrsandman): put in a resource manager and store look-up

	std::array<uint16_t, k_TextureResolution[0] * k_TextureResolution[1] * k_TextureResolution[2]> _bitmaps;

	DayNightClock _clock;
};

} // namespace openblack
