/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/RenderPass.h"

#include <glm/fwd.hpp>

#include <array>
#include <memory>

namespace openblack
{

class L3DMesh;

namespace graphics
{
class ShaderProgram;
class Texture2D;
} // namespace graphics

class Sky
{
public:
	Sky();
	~Sky() = default;

	void SetDayNightTimes(float nightFull, float duskStart, float duskEnd, float dayFull);

	void Interpolate555Texture(uint16_t* bitmap, uint16_t*, uint16_t*, float);

	void CalculateTextures();
	void SetTime(float time);

	float TimeOfDay;

private:
	friend class Renderer;

	static constexpr std::array<std::string_view, 3> _alignments = {
	    "Ntrl",
	    "good",
	    "evil",
	};
	static constexpr std::array<std::string_view, 3> _times = {
	    "day",
	    "dusk",
	    "night",
	};
	static constexpr std::array<uint16_t, 3> _textureResolution = {
	    256,
	    256,
	    static_cast<uint16_t>(_alignments.size() * _times.size()),
	};

	std::unique_ptr<L3DMesh> _model;
	std::unique_ptr<graphics::Texture2D> _texture; // TODO(bwrsandman): put in a resource manager and store look-up

	std::array<uint16_t, _textureResolution[0] * _textureResolution[1] * _textureResolution[2]> _bitmaps;

	float _timeOfDay;
	float _nightFullTime;
	float _duskStartTime;
	float _duskEndTime;
	float _dayFullTime;
};

} // namespace openblack
