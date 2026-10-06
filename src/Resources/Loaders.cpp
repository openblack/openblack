/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Resources/Loaders.h"

#include <cmath>

#include <algorithm>
#include <iostream>
#include <ranges>
#include <utility>

#include <GLWFile.h>
#include <L3DFile.h>
#include <MorphFile.h>
#include <PackFile.h>
#include <bgfx/bgfx.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/LandLightTable.h"
#include "3D/Light.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/Bitmap16B.h"
#include "Common/StringUtils.h"
#include "Common/Zip.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::filesystem;
using namespace openblack::resources;

L3DLoader::result_type L3DLoader::operator()(FromBufferTag, const std::string& debugName,
                                             const std::vector<uint8_t>& data) const
{
	auto mesh = std::make_shared<graphics::L3DMesh>(debugName);
	if (!mesh->LoadFromBuffer(data))
	{
		throw std::runtime_error("Unable to load mesh");
	}

	return mesh;
}

namespace
{
/// The bytes of a zipped .zzz file: its size unzipped, then the zipped data
std::vector<uint8_t> ReadZipped(const std::filesystem::path& path)
{
	auto stream = Locator::filesystem::value().Open(path, Stream::Mode::Read);
	uint32_t decompressedSize = 0;
	stream->Read(&decompressedSize);
	auto buffer = std::vector<uint8_t>(stream->Size() - sizeof(decompressedSize));
	stream->Read(buffer.data(), buffer.size());
	return zip::Inflate(buffer, decompressedSize);
}
} // namespace

L3DLoader::result_type L3DLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto mesh = std::make_shared<graphics::L3DMesh>(path.stem().string());
	auto pathExt = string_utils::LowerCase(path.extension().string());

	if (pathExt == ".l3d")
	{
		if (!mesh->LoadFromFilesystem(path))
		{
			throw std::runtime_error("Unable to load mesh");
		}
	}
	else if (pathExt == ".zzz")
	{
		if (!mesh->LoadFromBuffer(ReadZipped(path)))
		{
			throw std::runtime_error("Unable to load decompressed mesh");
		}
	}

	return mesh;
}

L3DLoader::result_type L3DLoader::operator()(FromDynamicFileTag, const std::string& debugName, const l3d::L3DFile& file) const
{
	auto mesh = std::make_shared<graphics::L3DMesh>(debugName, true);
	if (!mesh->Load(file))
	{
		throw std::runtime_error("Unable to load mesh");
	}
	return mesh;
}

L3DFileLoader::result_type L3DFileLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto file = std::make_shared<l3d::L3DFile>();
	const auto result = string_utils::LowerCase(path.extension().string()) == ".zzz"
	                        ? file->Open(ReadZipped(path))
	                        : file->Open(Locator::filesystem::value().ReadAll(path));
	if (result != l3d::L3DResult::Success)
	{
		throw std::runtime_error("Unable to read L3D file: " + std::string(l3d::ResultToStr(result)));
	}
	return file;
}

Bitmap16BLoader::result_type Bitmap16BLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	const auto data = Locator::filesystem::value().ReadAll(path);
	return std::make_shared<Bitmap16B>(data.data());
}

LandLightPaletteLoader::result_type LandLightPaletteLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	return std::make_shared<LandLightPalette>(Locator::filesystem::value().ReadAll(path));
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
	graphics::TextureFormat internalFormat;
	if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT1"))
	{
		internalFormat = graphics::TextureFormat::BlockCompression1;
	}
	else if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT3"))
	{
		internalFormat = graphics::TextureFormat::BlockCompression2;
	}
	else if (g3dTexture.ddsHeader.format.fourCC.data() == std::string("DXT5"))
	{
		internalFormat = graphics::TextureFormat::BlockCompression3;
	}
	else
	{
		throw std::runtime_error("Unsupported compressed texture format");
	}

	texture2D->Create(static_cast<uint16_t>(g3dTexture.ddsHeader.width), static_cast<uint16_t>(g3dTexture.ddsHeader.height), 1,
	                  internalFormat, graphics::Wrapping::Repeat, graphics::Filter::Linear,
	                  bgfx::makeRef(g3dTexture.ddsData.data(), static_cast<uint32_t>(g3dTexture.ddsData.size())));
	return texture2D;
}

Texture2DLoader::result_type Texture2DLoader::operator()(FromDiskTag, const std::filesystem::path& rawTexturePath) const
{
	bool found = false;
	const std::array<uint16_t, 12> resolutions = {{1024, 512, 256, 128, 64, 40, 32, 14, 12, 6}};

	const auto data = Locator::filesystem::value().ReadAll(rawTexturePath);
	graphics::TextureFormat format = graphics::TextureFormat::R8;
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
			format = graphics::TextureFormat::R8;
			found = true;
		}
		if (data.size() == 3 * pixelCount)
		{
			format = graphics::TextureFormat::RGB8;
			found = true;
		}
	}
	if (!found)
	{
		throw std::runtime_error("Unable to load texture: Ambiguous size and format: " + std::to_string(data.size()));
	}

	auto texture = std::make_shared<graphics::Texture2D>(("raw" / rawTexturePath.stem()).string());
	texture->Create(width, height, 1, format, graphics::Wrapping::Repeat, graphics::Filter::Linear,
	                bgfx::makeRef(data.data(), static_cast<uint32_t>(data.size())));

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

	if (!animation->LoadFromFilesystem(path))
	{
		throw std::runtime_error("Unable to load animation");
	}

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

namespace
{
using CreatureRig = creature::CreatureRig;

/// A triangle of a mesh's first submesh, as the creature file names it: its primitive and the vertices and vertex
/// groups counted in it
struct TriangleRef
{
	std::array<const l3d::L3DVertex*, 3> vertices;
	std::array<uint32_t, 3> bones;
	uint32_t skinId;
};

std::optional<TriangleRef> FindTriangle(const l3d::L3DFile& file, const morph::MeshIntersect& intersect)
{
	if (file.GetSubmeshHeaders().empty())
	{
		return std::nullopt;
	}
	const auto primitives = file.GetPrimitiveSpan(0);
	if (intersect.primitive >= primitives.size())
	{
		return std::nullopt;
	}
	uint32_t vertexOffset = 0;
	uint32_t groupOffset = 0;
	for (uint32_t i = 0; i < intersect.primitive; ++i)
	{
		vertexOffset += primitives[i].numVertices;
		groupOffset += primitives[i].numGroups;
	}
	const auto& primitive = primitives[intersect.primitive];
	const auto vertices = file.GetVertexSpan(0);
	const auto groups = file.GetVertexGroupSpan(0);
	TriangleRef triangle {.vertices = {}, .bones = {}, .skinId = primitive.material.skinID};
	for (size_t i = 0; i < 3; ++i)
	{
		const auto vertex = intersect.vertices.at(i);
		const auto group = intersect.vertexGroups.at(i);
		if (vertex >= primitive.numVertices || vertexOffset + vertex >= vertices.size())
		{
			return std::nullopt;
		}
		triangle.vertices.at(i) = &vertices[vertexOffset + vertex];
		// Meshes without bones place every vertex with the first
		triangle.bones.at(i) =
		    group < primitive.numGroups && groupOffset + group < groups.size() ? groups[groupOffset + group].boneIndex : 0;
	}
	return triangle;
}

/// The colour of a skin of the mesh at a texture coordinate, as the game reads it for the eyelids
glm::vec3 SkinColourAt(const l3d::L3DFile& file, uint32_t skinId, glm::vec2 uv)
{
	const auto skin = std::ranges::find(file.GetSkins(), skinId, &l3d::L3DTexture::id);
	if (skin == file.GetSkins().end())
	{
		return glm::vec3(1.0f);
	}
	constexpr auto k_Width = static_cast<int>(l3d::L3DTexture::k_Width);
	constexpr auto k_Height = static_cast<int>(l3d::L3DTexture::k_Height);
	const auto x = static_cast<int>(std::floor(uv.x * k_Width)) & (k_Width - 1);
	const auto y = static_cast<int>(std::floor(uv.y * k_Height)) & (k_Height - 1);
	const auto& texel = skin->texels.at(static_cast<size_t>((y * k_Width) + x));
	constexpr float k_Max = 15.0f;
	return {static_cast<float>(texel.r) / k_Max, static_cast<float>(texel.g) / k_Max, static_cast<float>(texel.b) / k_Max};
}

std::optional<CreatureRig::Eyes> LoadEyes(const morph::CreatureEyes& eyes,
                                          const std::array<std::optional<l3d::L3DFile>, CreatureRig::k_MeshCount>& meshes)
{
	const auto& base = meshes.front();
	if (!base.has_value())
	{
		return std::nullopt;
	}
	CreatureRig::Eyes result {
	    .scale = eyes.scale,
	    .points = {},
	    .lidAngles = {.open = eyes.lidAngles[0][1], .closed = eyes.lidAngles[1][1], .calm = eyes.lidAngles[2][1]},
	};
	for (size_t p = 0; p < eyes.points.size(); ++p)
	{
		const auto& source = eyes.points.at(p);
		auto& point = result.points.at(p);
		point = {.enabled = false,
		         .depth = source.depth,
		         .vertices = {},
		         .bones = {},
		         .u = source.intersect.u,
		         .v = source.intersect.v,
		         .skinColour = glm::vec3(1.0f)};
		if (!source.enabled)
		{
			continue;
		}
		const auto baseTriangle = FindTriangle(*base, source.intersect);
		if (!baseTriangle)
		{
			continue;
		}
		point.enabled = true;
		point.bones = baseTriangle->bones;
		for (size_t m = 0; m < meshes.size(); ++m)
		{
			// Variants share the base's vertices and their order; a missing one is the base
			const auto triangle = meshes.at(m).has_value() ? FindTriangle(*meshes.at(m), source.intersect) : std::nullopt;
			const auto& from = triangle ? *triangle : *baseTriangle;
			for (size_t i = 0; i < 3; ++i)
			{
				const auto& position = from.vertices.at(i)->position;
				point.vertices.at(m).at(i) = glm::vec3(position.x, position.y, position.z);
			}
		}
		const auto uvOf = [&baseTriangle](size_t i) {
			const auto& coordinate = baseTriangle->vertices.at(i)->texCoord;
			return glm::vec2(coordinate.x, coordinate.y);
		};
		const auto uv = uvOf(0) + ((uvOf(1) - uvOf(0)) * point.u) + ((uvOf(2) - uvOf(0)) * point.v);
		point.skinColour = SkinColourAt(*base, baseTriangle->skinId, uv);
	}
	return result;
}
} // namespace

CreatureRigLoader::result_type CreatureRigLoader::operator()(FromBufferTag, const std::vector<uint8_t>& block,
                                                             const std::filesystem::path& specDirectory,
                                                             const std::filesystem::path& meshDirectory) const
{
	morph::MorphFile file;
	const auto result = file.Open(block, specDirectory);
	if (result != morph::MorphResult::Success)
	{
		throw std::runtime_error("Unable to read creature animations: " + std::string(morph::ResultToStr(result)));
	}

	auto rig = std::make_shared<CreatureRig>();
	const auto& header = file.GetHeader();
	rig->baseMeshName = header.baseMeshName.data();
	std::array<std::string, CreatureRig::k_MeshCount> meshNames;
	meshNames.front() = rig->baseMeshName;
	rig->hasMesh.front() = true;
	for (size_t i = 0; i < header.variantMeshNames.size(); ++i)
	{
		meshNames.at(i + 1) = header.variantMeshNames.at(i).data();
		rig->hasMesh.at(i + 1) = !meshNames.at(i + 1).empty();
	}

	size_t animationCount = 0;
	for (const auto& set : file.GetAnimationSpecs().animationSets)
	{
		animationCount += set.animations.size();
	}
	for (size_t mesh = 0; mesh < CreatureRig::k_AnimatedMeshCount; ++mesh)
	{
		auto& animations = rig->animations.at(mesh);
		animations.resize(animationCount);
		for (size_t i = 0; i < animationCount; ++i)
		{
			const auto* source =
			    mesh == 0 ? file.GetBaseAnimation(i) : file.GetVariantAnimation(static_cast<uint32_t>(mesh - 1), i);
			if (source != nullptr && !source->keyframes.empty())
			{
				animations[i] = skeletal_animation::FromMorph(*source);
			}
		}
	}

	if (const auto& eyes = file.GetCreatureEyes())
	{
		auto& fileSystem = Locator::filesystem::value();
		std::array<std::optional<l3d::L3DFile>, CreatureRig::k_MeshCount> meshes;
		for (size_t i = 0; i < meshNames.size(); ++i)
		{
			const auto path = meshDirectory / (meshNames.at(i) + ".l3d");
			if (meshNames.at(i).empty() || !fileSystem.Exists(path))
			{
				continue;
			}
			auto& mesh = meshes.at(i).emplace();
			if (mesh.Open(fileSystem.ReadAll(path)) != l3d::L3DResult::Success)
			{
				meshes.at(i).reset();
			}
		}
		rig->eyes = LoadEyes(*eyes, meshes);
	}
	return rig;
}

SoundLoader::result_type SoundLoader::operator()(BaseLoader<audio::Sound>::FromBufferTag,
                                                 const pack::AudioBankSampleHeader& header,
                                                 const std::vector<std::vector<uint8_t>>& buffer) const
{
	auto sound = std::make_shared<audio::Sound>();
	// Let's clean up the names as they're very difficult to read from the debug GUI
	sound->name = std::filesystem::path(header.name.data()).filename().string();
	sound->id = header.id;
	sound->priority = header.priority;
	sound->sampleRate = static_cast<int>(header.sampleRate);
	sound->bitRate = 0;
	sound->volume = 1.f;
	sound->pitch = header.pitch;
	sound->pitchDeviation = header.pitchDeviation;
	sound->overrideFlags = header.overrideFlags;
	sound->headerVolume = header.volume;
	sound->loop = header.loop;
	sound->minDistance = header.minDist;
	sound->maxDistance = header.maxDist;
	sound->distanceScale = header.scale;
	sound->loopType = header.loopType;
	sound->group = static_cast<uint16_t>(header.group);
	sound->buffer = buffer;
	return sound;
}

LightLoader::result_type LightLoader::operator()(BaseLoader<Lights>::FromDiskTag, const std::filesystem::path& path) const
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading lights from file: {}", path.string());
	glw::GLWFile glw;

	const auto result = glw.ReadFile(*Locator::filesystem::value().GetData(path));
	if (result != glw::GLWResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open glw file from filesystem {}: {}", path.string(),
		                    glw::ResultToStr(result));
		throw glw::ResultToStr(result);
	}
	auto lights = std::make_shared<Lights>();
	for (const auto& entry : glw.GetGlows())
	{
		lights->emitters.emplace_back(MakeLightEmitter(entry));
	}
	return lights;
}

CameraPathLoader::result_type CameraPathLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto cameraPath = std::make_shared<CameraPath>(path.stem().string());
	if (!cameraPath->LoadFromFile(path))
	{
		throw std::runtime_error("Unable to load camera path");
	}

	return cameraPath;
}
