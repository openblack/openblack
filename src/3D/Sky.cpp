/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Sky.h"

#include <Common/StringUtils.h>
#include <glm/vec3.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "Common/Bitmap16B.h"
#include "Common/FileSystem.h"
#include "Game.h"

using namespace openblack::graphics;

namespace openblack
{

Sky::Sky()
{
	SetDayNightTimes(4.5, 7.0, 7.5, 8.25);

	// load in the mesh
	auto& filesystem = Game::instance()->GetFileSystem();
	_model = std::make_unique<L3DMesh>("Sky");
	_model->LoadFromFile(filesystem.WeatherSystemPath() / "sky.l3d");

	for (uint32_t i = 0; i < _alignments.size(); i++)
	{
		for (uint32_t j = 0; j < _times.size(); j++)
		{
			auto time = std::string(_times[j]);
			if (i == 1)
			{
				time = string_utils::Capitalise(time);
			}
			std::string filename = fmt::format("sky_{}_{}.555", _alignments[i], time);
			if (i == 1)
			{
				filename = string_utils::Capitalise(filename);
			}
			auto path = filesystem.WeatherSystemPath() / filename;
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading sky texture: {}", path.generic_string());

			Bitmap16B* bitmap = Bitmap16B::LoadFromFile(path);
			memcpy(&_bitmaps[(i * 3 + j) * _textureResolution[0] * _textureResolution[1]], bitmap->Data(), bitmap->Size());
			delete bitmap;
		}
	}

	_texture = std::make_unique<Texture2D>("Sky");
	_timeOfDay = 1.0f;

	_texture->Create(_textureResolution[0], _textureResolution[1], _textureResolution[2], Format::RGB5A1, Wrapping::ClampEdge,
	                 _bitmaps.data(), static_cast<uint32_t>(_bitmaps.size() * sizeof(_bitmaps[0])));
}

void Sky::SetDayNightTimes(float nightFull, float duskStart, float duskEnd, float dayFull)
{
	_nightFullTime = nightFull;
	_duskStartTime = duskStart;
	_duskEndTime = duskEnd;
	_dayFullTime = dayFull;
}

void Sky::SetTime(float time)
{
	assert(time <= 24.0f);
	_timeOfDay = time;
}

float Sky::GetCurrentSkyType() const
{
	assert(_timeOfDay <= 24.0f);

	// Reflect time at 12
	float time = _timeOfDay > 12.0f ? 24.0f - _timeOfDay : _timeOfDay;

	if (time <= _nightFullTime) // In full night
	{
		return 0.0f; // Index for night texture
	}
	else if (time < _duskStartTime) // Between night and dusk
	{
		return (time - _nightFullTime) / (_duskStartTime - _nightFullTime); // 0 - 1 lerp between night and dusk
	}
	else if (time <= _duskEndTime) // In full dusk
	{
		return 1.0f; // Index for dusk texture
	}
	else if (time < _dayFullTime) // Between dusk and day
	{
		return 1.0f + (time - _duskEndTime) / (_dayFullTime - _duskEndTime); // 1 - 2 lerp between dusk and day
	}
	else // In full day
	{
		return 2.0f; // Index for day texture
	}
}

} // namespace openblack
