/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Resources/Loaders.h"

#include <fstream>
#include <iostream>

#include "Common/FileSystem.h"
#include "Common/StringUtils.h"
#include "Common/Zip.h"
#include "Game.h"

using namespace openblack;
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
		if (!mesh->LoadFromFile(path))
		{
			throw std::runtime_error("Unable to load mesh");
		}
	}
	else if (pathExt == ".zzz")
	{
		auto stream = Game::Instance()->GetFileSystem().Open(path, FileMode::Read);
		uint32_t decompressedSize = 0;
		stream->Read(reinterpret_cast<uint32_t*>(&decompressedSize), sizeof(decompressedSize));
		auto buffer = std::vector<uint8_t>(stream->Size() - sizeof(decompressedSize));
		stream->Read(reinterpret_cast<char*>(buffer.data()), buffer.size());
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

	const auto data = Game::Instance()->GetFileSystem().ReadAll(rawTexturePath);
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
	if (!animation->LoadFromFile(path))
	{
		throw std::runtime_error("Unable to load animation");
	}

	return animation;
}

LevelLoader::result_type LevelLoader::operator()(FromDiskTag, const std::string& name, const std::filesystem::path& path,
                                                 LandType isCampaign) const
{
	std::string description;
	std::string levelName(name);
	bool isValid(false);

	{
		std::string loadLandscapeLine("LOAD_LANDSCAPE");
		std::string startMessageLine("START_GAME_MESSAGE");
		std::string gameMessageLine("ADD_GAME_MESSAGE_LINE");

		std::fstream levelFile;
		levelFile.open(path, std::ios::in);
		while (!levelFile.eof())
		{
			std::string line;
			std::getline(levelFile, line);
			if (!isValid && line.find(loadLandscapeLine) != std::string::npos)
			{
				isValid = true;
			}
			if (line.find(startMessageLine) != std::string::npos)
			{
				size_t first(line.find('\"'));
				size_t second(line.find('\"', first + 1));
				levelName = line.substr(first + 1, second - first - 1);
			}
			if (line.find(gameMessageLine) != std::string::npos)
			{
				size_t first(line.find('\"'));
				size_t second(line.find('\"', first + 1));
				description = line.substr(first + 1, second - first - 1);
			}
		}
	}

	return std::make_shared<Level>(levelName, path, description, isCampaign, isValid);
}

CreatureMindLoader::result_type CreatureMindLoader::operator()(FromDiskTag, const std::filesystem::path& /*unused*/) const
{
	return std::make_shared<creature::CreatureMind>();
}
