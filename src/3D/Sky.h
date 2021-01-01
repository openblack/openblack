/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
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
	std::unique_ptr<L3DMesh> _model;
	std::unique_ptr<graphics::Texture2D> _texture; // TODO(bwrsandman): put in a resource manager and store look-up

	std::array<std::array<uint16_t, 256 * 256>, 9> _bitmaps;

	float _timeOfDay;
	float _nightFullTime;
	float _duskStartTime;
	float _duskEndTime;
	float _dayFullTime;
};

} // namespace openblack
