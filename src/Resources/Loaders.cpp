/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Resources/Loaders.h"

#include <iostream>

#include <GLWFile.h>
#include <spdlog/spdlog.h>

#include "Audio/AudioManagerInterface.h"
#include "Common/StringUtils.h"
#include "Common/Zip.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::filesystem;
using namespace openblack::resources;

L3DLoader::result_type L3DLoader::operator()(FromBufferTag, const std::string& debugName,
                                             const std::vector<uint8_t>& data) const
{
	auto mesh = std::make_shared<L3DMesh>(debugName);
	if (!mesh->LoadFromBuffer(data))
	{
		throw std::runtime_error("Unable to load mesh");
	}

	return mesh;
}

L3DLoader::result_type L3DLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto mesh = std::make_shared<L3DMesh>(path.stem().string());
	auto pathExt = string_utils::LowerCase(path.extension().string());

	if (pathExt == ".l3d")
	{
#if __ANDROID__
		mesh->LoadFromBuffer(Locator::filesystem::value().ReadAll(path));
#else
		if (!mesh->LoadFromFile(path))
		{
			throw std::runtime_error("Unable to load mesh");
		}
#endif
	}
	else if (pathExt == ".zzz")
	{
		auto stream = Locator::filesystem::value().Open(path, Stream::Mode::Read);
		uint32_t decompressedSize = 0;
		stream->Read(&decompressedSize);
		auto buffer = std::vector<uint8_t>(stream->Size() - sizeof(decompressedSize));
		stream->Read(buffer.data(), buffer.size());
		auto decompressedBuffer = zip::Inflate(buffer, decompressedSize);
		if (!mesh->LoadFromBuffer(decompressedBuffer))
		{
			throw std::runtime_error("Unable to load decompressed mesh");
		}
	}

	return mesh;
}

Texture2DLoader::result_type Texture2DLoader::operator()(FromPackTag, const std::string& name,
                                                         const pack::G3DTexture& g3dTexture) const
{
	// some assumptions:
	// - no mipmaps
	// - no cubemap or volume textures
	// - always dxt1 or dxt3
	// - all are compressed
	auto texture2D = std::make_shared<graphics::Texture2D>(name);
	graphics::Format internalFormat;
	if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT1"))
	{
		internalFormat = graphics::Format::BlockCompression1;
	}
	else if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT3"))
	{
		internalFormat = graphics::Format::BlockCompression2;
	}
	else if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT5"))
	{
		internalFormat = graphics::Format::BlockCompression3;
	}
	else
	{
		throw std::runtime_error("Unsupported compressed texture format");
	}

	texture2D->Create(static_cast<uint16_t>(g3dTexture.ddsHeader.width), static_cast<uint16_t>(g3dTexture.ddsHeader.height), 1,
	                  internalFormat, graphics::Wrapping::Repeat, graphics::Filter::Linear, g3dTexture.ddsData.data(),
	                  static_cast<uint32_t>(g3dTexture.ddsData.size()));
	return texture2D;
}

Texture2DLoader::result_type Texture2DLoader::operator()(FromDiskTag, const std::filesystem::path& rawTexturePath) const
{
	bool found = false;
	const std::array<uint16_t, 6> resolutions = {{256, 40, 32, 14, 12, 6}};

	const auto data = Locator::filesystem::value().ReadAll(rawTexturePath);
	graphics::Format format = graphics::Format::R8;
	uint16_t width = 0;
	uint16_t height = 0;
	for (auto res : resolutions)
	{
		if (found)
		{
			break;
		}

		width = res;
		height = res;

		const typename decltype(data)::size_type pixelCount = width * height;

		if (data.size() == pixelCount)
		{
			format = graphics::Format::R8;
			found = true;
		}
		if (data.size() == 3 * pixelCount)
		{
			format = graphics::Format::RGB8;
			found = true;
		}
	}
	if (!found)
	{
		throw std::runtime_error("Unable to load texture: Ambiguous size and format: " + std::to_string(data.size()));
	}

	auto texture = std::make_shared<graphics::Texture2D>(("raw" / rawTexturePath.stem()).string());
	texture->Create(width, height, 1, format, graphics::Wrapping::Repeat, graphics::Filter::Linear, data.data(),
	                static_cast<uint32_t>(data.size()));

	return texture;
}

L3DAnimLoader::result_type L3DAnimLoader::operator()(FromBufferTag, const std::vector<uint8_t>& data) const
{
	auto animation = std::make_shared<L3DAnim>();
	animation->LoadFromBuffer(data);
	return animation;
}

L3DAnimLoader::result_type L3DAnimLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto animation = std::make_shared<L3DAnim>();
#if __ANDROID__
	animation->LoadFromBuffer(Locator::filesystem::value().ReadAll(path));
#else
	if (!animation->LoadFromFile(path))
	{
		throw std::runtime_error("Unable to load animation");
	}
#endif

	return animation;
}

LevelLoader::result_type LevelLoader::operator()(FromDiskTag, const std::filesystem::path& path, Level::LandType landType) const
{
	return std::make_shared<Level>(Level::ParseLevel(path, landType));
}

CreatureMindLoader::result_type CreatureMindLoader::operator()(FromDiskTag, const std::filesystem::path& /*unused*/) const
{
	return std::make_shared<creature::CreatureMind>();
}

SoundLoader::result_type SoundLoader::operator()(BaseLoader<audio::Sound>::FromBufferTag,
                                                 const pack::AudioBankSampleHeader& header,
                                                 const std::vector<std::vector<uint8_t>>& buffer) const
{
	auto sound = std::make_shared<audio::Sound>();
	// Let's clean up the names as they're very difficult to read from the debug GUI
	sound->name = std::filesystem::path(header.name.data()).filename().string();
	sound->priority = header.priority;
	sound->sampleRate = static_cast<int>(header.sampleRate);
	sound->bitRate = 0;
	sound->volume = 1.f;
	sound->pitch = header.pitch;
	sound->pitchDeviation = header.pitchDeviation;
	sound->playType = static_cast<audio::PlayType>(header.loopType);
	sound->buffer = buffer;
	return sound;
}

GlowLoader::result_type GlowLoader::operator()(BaseLoader<Glows>::FromDiskTag, const std::filesystem::path& path) const
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading glows from file: {}", path.string());
	glw::GLWFile glw;

	try
	{
#if __ANDROID__
		//  Android has a complicated permissions API, must call java code to read contents.
		lnd.Open(Locator::filesystem::value().ReadAll(path));
#else
		glw.Open(path);
#endif
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open glw file from filesystem {}: {}", path.string(), err.what());
		throw err;
	}
	auto glows = std::make_shared<Glows>();
	auto newPath = path;
	for (auto entry : glw.GetGlows())
	{
		auto glow = Glow();
		glow.name = entry.name;
		glow.colour = glm::vec4(entry.red, entry.green, entry.blue, 1.f);
		glow.scale = glm::vec3(entry.red, entry.green, entry.blue);
		auto transform = ecs::components::Transform();
		transform.scale = glm::vec3(1.f);
		transform.position = glm::vec3(entry.glowX, entry.glowY, entry.dirZ);
		glows->entries.emplace_back(glow, transform);
	}

	//	if (newPath.filename() == "main.glw")
	//	{
	//		glw.Write(newPath);
	//	}
	return glows;
}
