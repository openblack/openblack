/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Sky.h"

#include <glm/vec3.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "Common/Bitmap16B.h"
#include "Common/FileSystem.h"
#include "Game.h"
#include "Graphics/Texture2D.h"

constexpr std::array<std::string_view, 3> alignments = {"Ntrl", "good", "evil"};
constexpr std::array<std::string_view, 3> times = {"day", "dusk", "night"};

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

	for (uint32_t i = 0; i < alignments.size(); i++)
	{
		for (uint32_t j = 0; j < times.size(); j++)
		{
			auto time = std::string(times[j]);
			if (i == 0)
			{
				time[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(time[0])));
			}
			std::string filename = fmt::format("sky_{}_{}.555", alignments[i], time);
			if (i == 0)
			{
				filename[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(filename[0])));
			}
			auto path = filesystem.WeatherSystemPath() / filename;
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading sky texture: {}", path.generic_string());

			Bitmap16B* bitmap = Bitmap16B::LoadFromFile(path);
			memcpy(_bitmaps[i * 3 + j].data(), bitmap->Data(), bitmap->Size());
			delete bitmap;
		}
	}

	_texture = std::make_unique<Texture2D>("Sky");
	_timeOfDay = 1.0f;
	CalculateTextures();
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
	_timeOfDay = time;
	CalculateTextures();
}

void Sky::Interpolate555Texture(uint16_t* bitmap, uint16_t* bitmap1, uint16_t* bitmap2, float interpolate)
{
	for (int i = 0; i < 256 * 256; i++)
	{
		const auto color1 = glm::u16vec3((bitmap1[i] & 0x7C00) >> 10, (bitmap1[i] & 0x3E0) >> 5, (bitmap1[i] & 0x1F));
		const auto color2 = glm::u16vec3((bitmap2[i] & 0x7C00) >> 10, (bitmap2[i] & 0x3E0) >> 5, (bitmap2[i] & 0x1F));
		const auto color = glm::mix(color1, color2, interpolate);
		bitmap[i] = (color.r << 10) | (color.g << 5) | color.b;
	}
}

void Sky::CalculateTextures()
{
	std::array<uint16_t, 256 * 256> bitmap;

	uint16_t* day = _bitmaps[0].data();
	uint16_t* dusk = _bitmaps[1].data();
	uint16_t* night = _bitmaps[2].data();

	// 0.00: day
	// 0.25: dusk
	// 0.50: night
	// 0.75: dusk
	// 1.00: day
	if (_timeOfDay < 0.25f)
		Interpolate555Texture(bitmap.data(), day, dusk, _timeOfDay * 4.0f);
	else if (_timeOfDay < 0.5f)
		Interpolate555Texture(bitmap.data(), dusk, night, (_timeOfDay - 0.25f) * 4.0f);
	else if (_timeOfDay < 0.75f)
		Interpolate555Texture(bitmap.data(), night, dusk, (_timeOfDay - 0.5f) * 4.0f);
	else
		Interpolate555Texture(bitmap.data(), dusk, day, (_timeOfDay - 0.75f) * 4.0f);

	// set alpha=1
	for (unsigned int i = 0; i < 256 * 256; i++) bitmap[i] = bitmap[i] | 0x8000;

	_texture->Create(256, 256, 1, Format::RGB5A1, Wrapping::ClampEdge, bitmap.data(),
	                 static_cast<uint32_t>(bitmap.size() * sizeof(bitmap[0])));
}

} // namespace openblack
