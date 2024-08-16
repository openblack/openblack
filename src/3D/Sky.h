/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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

#include "Graphics/RenderPass.h"

namespace openblack
{

namespace graphics
{
class L3DMesh;
class ShaderProgram;
class Texture2D;
} // namespace graphics

class Sky
{
public:
	Sky();
	~Sky() = default;

	void SetDayNightTimes(float nightFull, float duskStart, float duskEnd, float dayFull);
	/// Time between 0 and 24 in hours
	void SetTime(float time);
	/// Return index in times as a float for interpolation between adjacent times
	/// Will use _nightFullTime, _duskStartTime, _duskEndTime and _dayFullTime to determine value
	/// 0 -> Night (min value)
	/// 1 -> Dawn/Dusk
	/// 2 -> Day (max value)
	[[nodiscard]] float GetCurrentSkyType() const;
	[[nodiscard]] graphics::L3DMesh& GetMesh() const noexcept { return *_mesh; }
	[[nodiscard]] graphics::Texture2D& GetTexture() const noexcept { return *_texture; }

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

	float _timeOfDay;
	float _nightFullTime;
	float _duskStartTime;
	float _duskEndTime;
	float _dayFullTime;
};

} // namespace openblack
