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
#include <span>
#include <utility>

#include <GLWFile.h>
#include <GestureFile.h>
#include <L3DFile.h>
#include <MorphFile.h>
#include <PackFile.h>
#include <ParticleFile.h>
#include <RawImage.h>
#include <StackedBitmap.h>
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

CreatureMindLoader::result_type CreatureMindLoader::operator()(FromDiskTag, const std::filesystem::path& creatureMindPath) const
{
	auto mind = std::make_shared<creature::CreatureMind>();
	mind->result = creaturemind::ReadFile(creatureMindPath, mind->data);
	if (!mind->Loaded())
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Creature mind {}: {}", creatureMindPath.generic_string(),
		                   creaturemind::ResultToStr(mind->result));
	}
	return mind;
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
/// A triangle's vertices in every mesh, in the space of the bone that moves each, and those bones. Variants share the
/// base's vertices and their order; a missing one is the base.
struct MeshTriangle
{
	std::array<std::array<glm::vec3, 3>, CreatureRig::k_MeshCount> vertices;
	std::array<uint32_t, 3> bones;
};

std::optional<MeshTriangle> TriangleInMeshes(const morph::MeshIntersect& intersect,
                                             const std::array<std::optional<l3d::L3DFile>, CreatureRig::k_MeshCount>& meshes)
{
	const auto& base = meshes.front();
	const auto baseTriangle = base.has_value() ? FindTriangle(*base, intersect) : std::nullopt;
	if (!baseTriangle)
	{
		return std::nullopt;
	}
	MeshTriangle result {.vertices = {}, .bones = baseTriangle->bones};
	for (size_t m = 0; m < meshes.size(); ++m)
	{
		const auto triangle = meshes.at(m).has_value() ? FindTriangle(*meshes.at(m), intersect) : std::nullopt;
		const auto& from = triangle ? *triangle : *baseTriangle;
		for (size_t i = 0; i < 3; ++i)
		{
			const auto& position = from.vertices.at(i)->position;
			result.vertices.at(m).at(i) = glm::vec3(position.x, position.y, position.z);
		}
	}
	return result;
}

std::vector<CreatureRig::HairGroup> LoadHair(std::span<const morph::HairGroup> groups,
                                             const std::array<std::optional<l3d::L3DFile>, CreatureRig::k_MeshCount>& meshes)
{
	std::vector<CreatureRig::HairGroup> result;
	for (const auto& group : groups)
	{
		// A group without segments has no strands to draw
		if (group.header.segmentCount == 0 || group.hairs.empty())
		{
			continue;
		}
		auto& hair = result.emplace_back();
		hair.segmentCount = group.header.segmentCount;
		hair.textured = group.header.mappingIndex == 1;
		for (size_t v = 0; v < hair.looks.size(); ++v)
		{
			const auto& variant = group.header.variants.at(v);
			hair.looks.at(v) = {
			    .colour = {variant.red, variant.green, variant.blue},
			    .length = variant.length,
			    .damping = variant.damping,
			    .stiffness = variant.stiffness,
			    .thickness = variant.thickness,
			};
		}
		for (const auto& source : group.hairs)
		{
			const auto triangle = TriangleInMeshes(source.intersection, meshes);
			if (!triangle)
			{
				continue;
			}
			CreatureRig::HairStrand strand {
			    .turned = (source.flags & 1u) != 0,
			    .angles = {},
			    .vertices = triangle->vertices,
			    .bones = triangle->bones,
			    .u = source.intersection.u,
			    .v = source.intersection.v,
			};
			for (size_t v = 0; v < strand.angles.size(); ++v)
			{
				const auto& angles = source.angles.at(v);
				strand.angles.at(v) = glm::vec3(angles[0], angles[1], angles[2]);
			}
			hair.strands.push_back(strand);
		}
		if (hair.strands.empty())
		{
			result.pop_back();
		}
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

	rig->meshNames = meshNames;

	// The sounds on moments of the animations
	const auto& extraData = file.GetExtraData();
	rig->soundEvents.resize(extraData.size());
	for (size_t i = 0; i < extraData.size(); ++i)
	{
		for (const auto& data : extraData[i])
		{
			rig->soundEvents[i].push_back({
			    .kind = static_cast<creature_audio::EventKind>(data.type),
			    .timeMs = static_cast<int32_t>(data.frame),
			    .action = static_cast<audio::SoundAction>(data.action),
			    .mode = static_cast<int32_t>(data.mode),
			});
		}
	}
	rig->soundObject = static_cast<int32_t>(file.GetHairHeader().soundObject);
	rig->soundBankName = file.GetSoundBankName();
	rig->leashBone = file.GetLeashBone();

	if (const auto& sites = file.GetTattooSites(); sites.has_value())
	{
		auto& tattooSites = rig->tattooSites.emplace();
		for (size_t i = 0; i < tattooSites.size(); ++i)
		{
			const auto& site = sites->at(i);
			tattooSites.at(i) = {
			    .enabled = site.enabled,
			    .u = site.u,
			    .v = site.v,
			    .skin = site.skin,
			    .size = site.size,
			    .mirror = site.mirror,
			    .rotation = static_cast<uint8_t>(site.rotation & 3u),
			};
		}
	}

	// The bones it acts with and when its object animations take hold or let go
	if (const auto& points = file.GetCreatureActionPoints(); points.has_value())
	{
		const std::array bones {points->rightHand, points->rightFoot, points->rightArmpit,
		                        points->belly,     points->head,      points->groin};
		if (std::ranges::all_of(bones, [](int32_t bone) { return bone >= 0; }))
		{
			const auto bone = [](int32_t value) { return static_cast<uint32_t>(value); };
			const auto ms = [](int32_t value) { return static_cast<float>(std::max(value, 0)); };
			rig->actionPoints = CreatureRig::ActionPoints {
			    .rightHand = bone(points->rightHand),
			    .rightFoot = bone(points->rightFoot),
			    .rightArmpit = bone(points->rightArmpit),
			    .belly = bone(points->belly),
			    .head = bone(points->head),
			    .groin = bone(points->groin),
			    .pickUpMs = ms(points->pickUpTime),
			    .destroyMs = ms(points->destroyTime),
			    .discardMs = ms(points->discardTime),
			    .eatMs = ms(points->eatTime),
			    .throwMs = ms(points->throwTime),
			    .putDownMs = ms(points->putDownTime),
			};
		}
	}

	// The eyes and the hair sit on triangles of the meshes
	const auto& eyes = file.GetCreatureEyes();
	if (eyes.has_value() || !file.GetHairGroups().empty())
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
		if (eyes.has_value())
		{
			rig->eyes = LoadEyes(*eyes, meshes);
		}
		rig->hairGroups = LoadHair(file.GetHairGroups(), meshes);
	}
	return rig;
}

CreatureSkinArtLoader::result_type CreatureSkinArtLoader::operator()(FromDiskTag, const Paths& paths) const
{
	auto& fileSystem = Locator::filesystem::value();
	constexpr uint32_t k_Size = creature_marks::k_AtlasSize;
	const auto rgb = [&fileSystem](const std::filesystem::path& path, uint32_t width, uint32_t height) {
		auto image = rawimage::DecodeRgb(fileSystem.ReadAll(path), width, height);
		if (!image)
		{
			throw std::runtime_error("Unexpected size of " + path.string());
		}
		return std::move(image->pixels);
	};
	const auto grey = [&fileSystem](const std::filesystem::path& path, uint32_t width, uint32_t height) {
		auto image = rawimage::DecodeGrey(fileSystem.ReadAll(path), width, height);
		if (!image)
		{
			throw std::runtime_error("Unexpected size of " + path.string());
		}
		return std::move(image->pixels);
	};
	auto art = std::make_shared<creature_skin::Art>();
	const auto symbols =
	    fileSystem.Exists(paths.symbols) ? rgb(paths.symbols, k_Size, k_Size) : std::vector<std::array<uint8_t, 3>> {};
	const auto defaults = rgb(paths.defaultSymbols, k_Size, k_Size);
	for (uint32_t design = 0; design < art->designs.size(); ++design)
	{
		auto written = creature_tattoo::DesignFromAtlas(symbols, k_Size, design);
		const bool blank = std::ranges::all_of(written.front().levels, [](uint8_t level) { return level == 0; });
		art->designs.at(design) = blank ? creature_tattoo::DesignFromAtlas(defaults, k_Size, design) : std::move(written);
	}
	art->damage.fresh = {.colours = rgb(paths.freshDamage, k_Size, k_Size),
	                     .alpha = grey(paths.freshDamageAlpha, k_Size, k_Size)};
	art->damage.old = {.colours = rgb(paths.oldDamage, k_Size, k_Size), .alpha = grey(paths.oldDamageAlpha, k_Size, k_Size)};
	art->palette = rgb(paths.palette, creature_tattoo::k_PaletteColumns, creature_tattoo::k_PaletteRows);
	return art;
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

GestureTemplatesLoader::result_type GestureTemplatesLoader::operator()(FromDiskTag, const std::filesystem::path& path) const
{
	auto file = std::make_shared<gestures::GestureFile>();
	if (const auto result = file->Open(Locator::filesystem::value().ReadAll(path));
	    result != gestures::GestureFileResult::Success)
	{
		throw std::runtime_error("Unable to load gesture templates " + path.string() + ": " +
		                         std::string(gestures::ResultToStr(result)));
	}
	return file;
}

ParticleFileLoader::result_type ParticleFileLoader::operator()(FromDiskTag, const std::filesystem::path& directory,
                                                               const std::string& name) const
{
	auto& fileSystem = Locator::filesystem::value();
	std::string text;
	if (const auto loose = directory / (name + ".txt"); fileSystem.Exists(loose))
	{
		const auto bytes = fileSystem.ReadAll(loose);
		text.assign(bytes.begin(), bytes.end());
	}
	else
	{
		const auto bytes = fileSystem.ReadAll(directory / (name + "_txt.zzz"));
		const auto compressed = psys::SplitCompressed(bytes);
		if (!compressed.has_value())
		{
			throw std::runtime_error("Particle file " + name + " is too short");
		}
		const auto inflated =
		    zip::Inflate(std::vector<uint8_t>(compressed->deflated.begin(), compressed->deflated.end()), compressed->textSize);
		text.assign(inflated.begin(), inflated.end());
	}
	auto file = psys::ParticleFile::Parse(text);
	if (!file.has_value())
	{
		throw std::runtime_error("Particle file " + name + " cannot be read");
	}
	return std::make_shared<psys::ParticleFile>(std::move(*file));
}

ParticleBitmapLoader::result_type ParticleBitmapLoader::operator()(FromDiskTag, const std::filesystem::path& path,
                                                                   const Layout& layout) const
{
	const auto bytes = Locator::filesystem::value().ReadAll(path);
	auto bitmap = psys::LoadStackedBitmap(bytes, layout.pitch, layout.channels, layout.framesInFile, layout.framesInUse);
	if (!bitmap.has_value())
	{
		throw std::runtime_error("Light map " + path.string() + " is not the size its effect says");
	}
	return std::make_shared<psys::StackedBitmap>(std::move(*bitmap));
}
