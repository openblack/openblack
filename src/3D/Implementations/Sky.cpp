/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "Sky.h"

#include <bgfx/bgfx.h>
#include <glm/vec3.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "Common/Bitmap16B.h"
#include "Common/StringUtils.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack::filesystem;
using namespace openblack::graphics;

namespace openblack
{

Sky::Sky() noexcept
{
	auto& fileSystem = Locator::filesystem::value();

	_clock.Reset();
	_dome = sky_dome::Follow(GetCurrentSkyType());

	// load in the mesh
	_mesh = std::make_unique<graphics::L3DMesh>("Sky");
	_mesh->LoadFromFilesystem(fileSystem.GetPath<filesystem::Path::WeatherSystem>() / "sky.l3d");

	// TODO (#749) Maybe use std::views::enumerate
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

	_texture->Create(k_TextureResolution[0], k_TextureResolution[1], k_TextureResolution[2], TextureFormat::BGR5A1,
	                 Wrapping::ClampEdge, Filter::Linear,
	                 bgfx::makeRef(_bitmaps.data(), static_cast<uint32_t>(_bitmaps.size() * sizeof(_bitmaps[0]))));
}

Sky::~Sky() noexcept = default;

void Sky::SetTime(float time) noexcept
{
	_clock.SetScriptTime(time);
	_dome.Jump(GetCurrentSkyType());
}

float Sky::GetCurrentSkyType() const noexcept
{
	return _clock.SkyType(_clock.GetVisualTime());
}

} // namespace openblack
