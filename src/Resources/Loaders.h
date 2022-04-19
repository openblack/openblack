/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"

#include <PackFile.h>
#include <entt/entt.hpp>

namespace openblack::resources
{

using namespace l3d;

template <typename Loader, typename Resource>
struct BaseLoader: entt::resource_loader<Loader, Resource>
{
public:
	using ResourceType = Resource;
};

struct L3DLoader final: BaseLoader<L3DLoader, L3DMesh>
{
	[[nodiscard]] entt::resource_handle<L3DMesh> load(const std::string& debugName, const std::vector<uint8_t>& data) const;

	[[nodiscard]] entt::resource_handle<L3DMesh> load(const std::filesystem::path& path) const;
};

struct Texture2DLoader final: BaseLoader<Texture2DLoader, graphics::Texture2D>
{
	[[nodiscard]] entt::resource_handle<graphics::Texture2D> load(const std::string& name,
	                                                              const pack::G3DTexture g3dTexture) const;
	[[nodiscard]] entt::resource_handle<graphics::Texture2D> load(const std::filesystem::path& rawTexturePath) const;
};

struct L3DAnimLoader final: BaseLoader<L3DAnimLoader, L3DAnim>
{
	[[nodiscard]] entt::resource_handle<L3DAnim> load(const std::vector<uint8_t>& data) const;

	[[nodiscard]] entt::resource_handle<L3DAnim> load(const std::filesystem::path& path) const;
};
} // namespace openblack::resources
