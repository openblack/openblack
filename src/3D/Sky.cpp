/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Sky.h"

#include <glm/vec3.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "Common/Bitmap16B.h"
#include "Common/StringUtils.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

using namespace openblack::filesystem;
using namespace openblack::graphics;

namespace openblack
{

Sky::Sky()
{
	auto& fileSystem = Locator::filesystem::value();

	SetDayNightTimes(4.5, 7.0, 7.5, 8.25);

	// load in the mesh
	_model = std::make_unique<L3DMesh>("Sky");
#if __ANDROID__
	//  Android has a complicated permissions API, must call java code to read contents.
	_model->LoadFromBuffer(
	    Locator::filesystem::value().ReadAll(fileSystem.GetPath<filesystem::Path::WeatherSystem>() / "sky.l3d"));
#else
	_model->LoadFromFile(fileSystem.GetPath<filesystem::Path::WeatherSystem>() / "sky.l3d");
#endif

	for (uint32_t idx = 0; const auto& alignment : k_Alignments)
	{
		for (const auto& timeView : k_Times)
		{
			auto time = std::string(timeView);
			auto prefix = std::string("sky");
			if (idx >= k_Times.size() && idx < 2 * k_Times.size())
			{
				time = string_utils::Capitalise(time);
				prefix = string_utils::Capitalise(prefix);
			}
			const auto filename = fmt::format("{}_{}_{}.555", prefix, alignment, time);
			const auto path = fileSystem.GetPath<filesystem::Path::WeatherSystem>() / filename;
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading sky texture: {}", path.generic_string());

			Bitmap16B* bitmap = Bitmap16B::LoadFromFile(path);
			memcpy(&_bitmaps.at(idx * k_TextureResolution[0] * k_TextureResolution[1]), bitmap->Data(), bitmap->Size());
			delete bitmap;
			++idx;
		}
	}

	_texture = std::make_unique<Texture2D>("Sky");
	_timeOfDay = 1.0f;

	_texture->Create(k_TextureResolution[0], k_TextureResolution[1], k_TextureResolution[2], Format::BGR5A1,
	                 Wrapping::ClampEdge, Filter::Linear, _bitmaps.data(),
	                 static_cast<uint32_t>(_bitmaps.size() * sizeof(_bitmaps[0])));
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
	if (time < _duskStartTime) // Between night and dusk
	{
		return (time - _nightFullTime) / (_duskStartTime - _nightFullTime); // 0 - 1 lerp between night and dusk
	}
	if (time <= _duskEndTime) // In full dusk
	{
		return 1.0f; // Index for dusk texture
	}
	if (time < _dayFullTime) // Between dusk and day
	{
		return 1.0f + (time - _duskEndTime) / (_dayFullTime - _duskEndTime); // 1 - 2 lerp between dusk and day
	}
	// In full day
	return 2.0f; // Index for day texture
}

} // namespace openblack
