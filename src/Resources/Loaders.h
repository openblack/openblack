/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <PackFile.h>
#include <entt/entt.hpp>

#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "Level.h"

namespace openblack::resources
{

using namespace l3d;

template <typename Loader, typename Resource>
struct BaseLoader: entt::resource_loader<Loader, Resource>
{
public:
	using result_type = entt::resource_handle<Resource>;
	using ResourceType = Resource;
	struct FromBufferTag
	{
	};
	struct FromDiskTag
	{
	};
};

struct L3DLoader final: BaseLoader<L3DLoader, L3DMesh>
{
	[[nodiscard]] result_type load(FromBufferTag t, const std::string& debugName, const std::vector<uint8_t>& data) const
	{
		return operator()(t, debugName, data);
	}
	[[nodiscard]] result_type load(FromDiskTag t, const std::filesystem::path& path) const {
		return operator()(t, path);
	}

	[[nodiscard]] result_type operator()(FromBufferTag, const std::string& debugName, const std::vector<uint8_t>& data) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

struct Texture2DLoader final: BaseLoader<Texture2DLoader, graphics::Texture2D>
{
	struct FromPackTag
	{
	};

	[[nodiscard]] result_type load(FromPackTag t, const std::string& name, const pack::G3DTexture& g3dTexture) const {
		return operator()(t, name, g3dTexture);
	}
	[[nodiscard]] result_type load(FromDiskTag t, const std::filesystem::path& rawTexturePath) const {
		return operator()(t, rawTexturePath);
	}

	[[nodiscard]] result_type operator()(FromPackTag, const std::string& name, const pack::G3DTexture& g3dTexture) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& rawTexturePath) const;
};

struct L3DAnimLoader final: BaseLoader<L3DAnimLoader, L3DAnim>
{
	[[nodiscard]] result_type load(FromBufferTag t, const std::vector<uint8_t>& data) const {
		return operator()(t, data);
	}
	[[nodiscard]] result_type load(FromDiskTag t, const std::filesystem::path& path) const {
		return operator()(t, path);
	}

	[[nodiscard]] result_type operator()(FromBufferTag, const std::vector<uint8_t>& data) const;
	[[nodiscard]] result_type operator()(FromDiskTag, const std::filesystem::path& path) const;
};

struct LevelLoader final: BaseLoader<LevelLoader, Level>
{
	[[nodiscard]] result_type load(FromDiskTag t, const std::string& name, const std::filesystem::path& path,
	                               bool isCampaign) const {
		return operator()(t, name, path, isCampaign);
	}

	[[nodiscard]] result_type operator()(FromDiskTag, const std::string& name, const std::filesystem::path& path,
	                                     bool isCampaign) const;
};
} // namespace openblack::resources
