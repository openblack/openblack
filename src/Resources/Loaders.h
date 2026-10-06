/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <queue>

#include <PackFile.h>

#include "3D/CameraPath.h"
#include "3D/L3DAnim.h"
#include "3D/Light.h"
#include "Audio/Sound.h"
#include "Creature/CreatureMind.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureSkin.h"
#include "Level.h"

namespace openblack
{
class Bitmap16B;
class LandLightPalette;
} // namespace openblack

namespace openblack::graphics
{
class L3DMesh;
class Texture2D;
} // namespace openblack::graphics

namespace openblack::l3d
{
class L3DFile;
}

namespace openblack::psys
{
struct ParticleFile;
struct StackedBitmap;
} // namespace openblack::psys

namespace openblack::pack
{
struct AudioBankSampleHeader;
struct G3DTexture;
} // namespace openblack::pack

namespace openblack::resources
{

template <typename Resource>
struct BaseLoader
{
	using result_type = std::shared_ptr<Resource>;
	using ResourceType = Resource;
	struct FromBufferTag
	{
	};
	struct FromDiskTag
	{
	};
};

struct L3DLoader final: BaseLoader<graphics::L3DMesh>
{
	/// From a file already read, as a mesh whose vertices and skins can be changed after
	struct FromDynamicFileTag
	{
	};

	[[nodiscard]] result_type operator()(FromBufferTag, const std::string& debugName, const std::vector<uint8_t>& data) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
	[[nodiscard]] result_type operator()(FromDynamicFileTag, const std::string& debugName, const l3d::L3DFile& file) const;
};

/// The data of an L3D file, .l3d or zipped .zzz, for what changes meshes on the CPU
struct L3DFileLoader final: BaseLoader<l3d::L3DFile>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

/// A 16 bit image, .16B
struct Bitmap16BLoader final: BaseLoader<Bitmap16B>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

struct LandLightPaletteLoader final: BaseLoader<LandLightPalette>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

struct Texture2DLoader final: BaseLoader<graphics::Texture2D>
{
	struct FromPackTag
	{
	};

	[[nodiscard]] result_type operator()(FromPackTag, const std::string& name, const pack::G3DTexture& g3dTexture) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& rawTexturePath) const;
};

struct L3DAnimLoader final: BaseLoader<L3DAnim>
{
	[[nodiscard]] result_type operator()(FromBufferTag, const std::vector<uint8_t>& data) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

struct LevelLoader final: BaseLoader<Level>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path, Level::LandType landType) const;
};

struct CreatureMindLoader final: BaseLoader<creature::CreatureMind>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& creatureMindPath) const;
};

/// What moves a species' body, from the Creature block of its .cbn file. The species' meshes are read from
/// meshDirectory for where its eyes sit; the animations are named by the creature spec file in specDirectory.
struct CreatureRigLoader final: BaseLoader<creature::CreatureRig>
{
	[[nodiscard]] result_type operator()(FromBufferTag, const std::vector<uint8_t>& block,
	                                     const std::filesystem::path& specDirectory,
	                                     const std::filesystem::path& meshDirectory) const;
};

/// What creatures' tattoos and marks are painted with, read from raw images: the tattoo designs from the atlas of
/// players' symbols, the fresh and old damage atlases with their alphas, and the tattoo palette
struct CreatureSkinArtLoader final: BaseLoader<creature_skin::Art>
{
	struct Paths
	{
		/// The players' symbols as the game last wrote them, and the symbols it ships with, for the cells no player's
		/// symbol has been written into
		std::filesystem::path symbols;
		std::filesystem::path defaultSymbols;
		std::filesystem::path freshDamage;
		std::filesystem::path freshDamageAlpha;
		std::filesystem::path oldDamage;
		std::filesystem::path oldDamageAlpha;
		std::filesystem::path palette;
	};
	[[nodiscard]] result_type operator()(FromDiskTag, const Paths& paths) const;
};

struct SoundLoader final: BaseLoader<audio::Sound>
{
	[[nodiscard]] result_type operator()(FromBufferTag, const pack::AudioBankSampleHeader& header,
	                                     const std::vector<std::vector<uint8_t>>& buffer) const;
};

struct LightLoader final: BaseLoader<Lights>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

/// A particle effect file by its name in a folder: <name>.txt when there is one, else the compressed <name>_txt.zzz
struct ParticleFileLoader final: BaseLoader<psys::ParticleFile>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& directory, const std::string& name) const;
};

/// A particle light map: a file of frames of pitch by pitch texels of `channels` bytes, laid out in a grid
struct ParticleBitmapLoader final: BaseLoader<psys::StackedBitmap>
{
	struct Layout
	{
		int pitch;
		int channels;
		int framesInFile;
		int framesInUse;
	};
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path, const Layout& layout) const;
};

struct CameraPathLoader final: BaseLoader<CameraPath>
{
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};
} // namespace openblack::resources
